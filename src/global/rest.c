/* Std */
#include <sys_defs.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include <msg.h>
#include <events.h>
#include <vstream.h>
#include <dict.h>
#include <bounce_log.h>

/* Service */
#include <curl/curl.h>
#include <json/json.h>

/* Own */
#include <string.h>
#include <vstring.h>
#include "rest.h"

static CURL *curl_handler;
static char *server_name = 0;

static size_t read_callback( void *out, const size_t size, const size_t nmemb, const void *in) {
    struct inmem_s *in_data = (struct inmem_s *)in;
    size_t in_size = 0;

    if ( size*nmemb < 1 || in_data->left == 0 )
        return 0;
    if ( in_data->left <= size*nmemb )
        in_size = in_data->left;
    else
        in_size = size*nmemb - 1;
    memcpy( out, in_data->ptr, in_size );
    in_data->left = in_data->left - in_size;
    in_data->ptr = in_data->ptr + in_size;
    return in_size;
}

/* Perform HTTP PUT to given url with json data,
/* adding hostname to it */

CURLcode perform_put( const char *url, const char *queue_id, json_object *json ) {
    struct inmem_s json_data;
    CURLcode res;
    int retval;
    struct curl_slist *headers = NULL;
    VSTRING *vstr_queue_id_hdr = vstring_alloc(100);
    char *queue_id_copy = (char *)alloca( strlen(queue_id) + 1 );

    strcpy(queue_id_copy, queue_id);
    if ( server_name == 0 ) {
        server_name = (char *) mymalloc( (ssize_t) sizeof(char) * 1024 );
        if ( gethostname(server_name, (size_t) 1024) )
            server_name = (char *) mystrdup("undetected");
    }
    json_object_object_add( json, "server_name", json_object_new_string( server_name ) );
    json_data.ptr = json_object_get_string( json );
    json_data.left = strlen( json_data.ptr );
    if ( msg_verbose )
        msg_info("json_data: %s", json_data.ptr );
    curl_handler = curl_easy_init();
    if ( curl_handler ) {
        vstr_queue_id_hdr = vstring_sprintf( vstr_queue_id_hdr, "Queue-Id: %s",
                queue_id_copy );
        headers = curl_slist_append( headers, "Content-Type: application/json");
        headers = curl_slist_append( headers, vstring_str( vstr_queue_id_hdr ) );
        curl_easy_setopt( curl_handler, CURLOPT_HTTPHEADER, headers );
        curl_easy_setopt( curl_handler, CURLOPT_URL, url );
        curl_easy_setopt( curl_handler, CURLOPT_NOPROGRESS, 1L );
        curl_easy_setopt( curl_handler, CURLOPT_TIMEOUT, 5L );
        curl_easy_setopt( curl_handler, CURLOPT_VERBOSE, 0L );
        curl_easy_setopt( curl_handler, CURLOPT_UPLOAD, 1L );
        curl_easy_setopt( curl_handler, CURLOPT_READDATA, &json_data );
        curl_easy_setopt( curl_handler, CURLOPT_READFUNCTION, (curl_read_callback) read_callback );
        curl_easy_setopt( curl_handler, CURLOPT_INFILESIZE, json_data.left );
        res = curl_easy_perform( curl_handler );
        curl_easy_cleanup( curl_handler );
    } else {
        msg_error("curl_handler BROKEN!");
    }
    vstring_free( vstr_queue_id_hdr );
    curl_slist_free_all( headers );
    myfree( server_name );
    return res;
}

/*
/* queue_from -- initial postfix queue
/* queue_to -- target postfix queue
/* queue_id -- internal message id
/* action = "mesage_queue_changed"
*/
void restlog_change_queue( const char *url, const char *queue_from, const char *queue_to, const char *queue_id ) {
    json_object *root, *reason_list;
    CURLcode    res;
    BOUNCE_LOG  *bounce_log;
    RCPT_BUF    *rcpt_buf;
    DSN_BUF     *dsn_buf;
    RECIPIENT   *rcpt = &rcpt_buf->rcpt;
    DSN         *dsn = &dsn_buf->dsn;

    /* Create JSON object to transfer to index */
    root = json_object_new_object( );
    json_object_object_add( root, "queue_from", json_object_new_string( queue_from ) );
    json_object_object_add( root, "queue_id", json_object_new_string( queue_id ) );
    json_object_object_add( root, "queue_to", json_object_new_string( queue_to ) );
    json_object_object_add( root, "action", json_object_new_string( "message_queue_changed" ) );
    /* Grab reasons from bounce log */
    reason_list = json_object_new_object();
    if ( ( bounce_log = bounce_log_open( "defer", queue_id, O_RDONLY, 0) ) != 0 ) {
        dsn_buf = dsb_create();
        rcpt_buf = rcpb_create();
        while ( bounce_log_read( bounce_log, rcpt_buf, dsn_buf ) != 0 ) {
            rcpt = &rcpt_buf->rcpt;
            dsn = &dsn_buf->dsn;
            json_object_object_add( reason_list, rcpt->address, json_object_new_string( dsn->reason ));
        }
        rcpb_free( rcpt_buf );
        dsb_free( dsn_buf );
        if (bounce_log_close( bounce_log ))
            msg_warn("close %s %s: %m", queue_to, queue_id);
    } else {
        msg_warn("Error opening bounce log %s: %m", queue_to, queue_id);
    }
    json_object_object_add( root, "delay_reason", reason_list );
    res = perform_put( url, queue_id, root );
    if ( res != CURLE_OK ) {
            msg_warn("curl_easy_perform() failed: %s %m", curl_easy_strerror(res) );
    }
}

/*
/* queue_id -- internal message id
/* wait_time -- seconds until next retry
/* action = "message_next_retry"
*/
void restlog_change_wait_time( const char *url, const char *queue_id, size_t wait_time) {
    json_object *root;
    CURLcode res;

    /* Create JSON object to transfer to index */
    root = json_object_new_object();
    json_object_object_add( root, "queue_id", json_object_new_string( queue_id ) );
    json_object_object_add( root, "wait_time", json_object_new_int( wait_time ) );
    json_object_object_add( root, "action", json_object_new_string( "message_next_retry" ) );

    res = perform_put( url, queue_id, root );
    if ( res != CURLE_OK ) {
            msg_warn("curl_easy_perform() failed: %s", curl_easy_strerror(res) );
    }
}

/*
/* queue_name -- postfix queue name
/* queue_id -- internal message id
/* sender -- mailaddress of the sender
/* recipient -- mailaddress of recipient
/* rcpt_count -- number of recipients
/* msg_size -- message size in bytes
/* subject -- message subject
/* action = "message_added"
*/
void restlog_queued( const char *url, const char *queue_id,
    const char *queue_name, const char *env_sender,
    const char* recip, const int rcpt_count, const char *subject,
    const unsigned long msg_size ) {
    json_object *root;
    CURLcode res;

    /* Create JSON object to transfer to index */
    root = json_object_new_object();
    json_object_object_add( root, "queue_id", json_object_new_string( queue_id ) );
    json_object_object_add( root, "queue_name", json_object_new_string( queue_name ) );
    json_object_object_add( root, "sender", json_object_new_string( env_sender ) );
    json_object_object_add( root, "recipient", json_object_new_string( recip ) );
    json_object_object_add( root, "rcpt_count", json_object_new_int( rcpt_count ) );
    json_object_object_add( root, "msg_size", json_object_new_int( msg_size ) );
    json_object_object_add( root, "subject", json_object_new_string( subject ) );
    json_object_object_add( root, "action", json_object_new_string( "message_added" ) );

    res = perform_put( url, queue_id, root );
    if ( res != CURLE_OK ) {
            msg_warn("curl_easy_perform() failed: %s", curl_easy_strerror(res) );
    }
}

/*
/* queue -- name of postfix queue
/* queue_id -- internal message id
/* action = "message_sent"
*/
void restlog_message_sent( const char *url, const char *queue_name,
    const char *queue_id, const double msg_delay, const int flag ) {
    json_object *root;
    CURLcode res;

    /* Create JSON object to transfer to index */
    root = json_object_new_object();
    json_object_object_add( root, "queue_id", json_object_new_string( queue_id ) );
    json_object_object_add( root, "queue", json_object_new_string( queue_name ) );
    json_object_object_add( root, "delay", json_object_new_double( msg_delay ) ) ;
    if ( flag == QMSG_SENT )
        json_object_object_add( root, "action", json_object_new_string( "message_sent" ) );
    else
        json_object_object_add( root, "action", json_object_new_string( "message_expired" ) );

    res = perform_put( url, queue_id, root );
    if ( res != CURLE_OK ) {
            msg_warn("curl_easy_perform() failed: %s", curl_easy_strerror(res) );
    }
}

/*
/* queue -- name of postfix queue
/* queue_id -- internal message id
/* action = "message_discarded"
*/
void restlog_message_discarded( const char *url, const char *queue_id, const char *queue_name ) {
    json_object *root;
    CURLcode res;

    /* Create JSON object to transfer to index */
    root = json_object_new_object();
    json_object_object_add( root, "queue_id", json_object_new_string( queue_id ) );
    json_object_object_add( root, "queue", json_object_new_string( queue_name ) );
    json_object_object_add( root, "action", json_object_new_string( "message_discarded" ) );

    res = perform_put( url, queue_id, root );
    if ( res != CURLE_OK ) {
            msg_warn("curl_easy_perform() failed: %s", curl_easy_strerror(res) );
    }
}

/*
/* queue -- name of postfix queue
/* queue_id -- internal message id
/* action = "message_expired"
*/
void restlog_message_expired( const char *url, const char *queue_name, const char *queue_id ) {
    json_object *root;
    CURLcode res;

    /* Create JSON object to transfer to index */
    root = json_object_new_object();
    json_object_object_add( root, "queue_id", json_object_new_string( queue_id ) );
    json_object_object_add( root, "queue", json_object_new_string( queue_name ) );
    json_object_object_add( root, "action", json_object_new_string( "message_expired" ) );

    res = perform_put( url, queue_id, root );
    if ( res != CURLE_OK ) {
            msg_warn("curl_easy_perform() failed: %s", curl_easy_strerror(res) );
    }
}
