/* Std */
#include <sys_defs.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include <msg.h>
#include <events.h>
#include <vstream.h>
#include <dict.h>

/* Service */
#include <curl/curl.h>
#include "cJSON.h"

/* Own */
#include <string.h>
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

CURLcode perform_put( const char *url, cJSON *json ) {
    struct inmem_s json_data;
    CURLcode res;
    int retval;
    struct curl_slist *headers = NULL;

    if ( ! server_name ) {
        server_name = (char *) mymalloc( (ssize_t) 1024 );
        if ( gethostname(server_name, (size_t) 1024) )
            server_name = (char *) mystrdup("undetected");
    }
    cJSON_AddItemToObject( json, "server_name", cJSON_CreateString( server_name ) );
    json_data.ptr = cJSON_PrintUnformatted( json );
    json_data.left = strlen( json_data.ptr );
    if ( msg_verbose )
        msg_info("json_data: %s", json_data.ptr );
    curl_handler = curl_easy_init();
    if ( curl_handler ) {
        headers = curl_slist_append( headers, "Content-Type: application/json");
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
        curl_slist_free_all( headers );
        curl_easy_cleanup( curl_handler );
    } else {
        msg_error("curl_handler BROKEN!");
    }
    return res;
}

/*
/* queue_from -- initial postfix queue
/* queue_to -- target postfix queue
/* queue_id -- internal message id
/* action = "mesage_queue_changed"
*/
void restlog_change_queue( const char *url, const char *queue_from, const char *queue_to, const char *queue_id ) {
    cJSON *root;
    CURLcode res;

    /* Create JSON object to transfer to index */
    root = cJSON_CreateObject();
    cJSON_AddItemToObject( root, "queue_from", cJSON_CreateString( queue_from ) );
    cJSON_AddItemToObject( root, "queue_id", cJSON_CreateString( queue_id ) );
    cJSON_AddItemToObject( root, "queue_to", cJSON_CreateString( queue_to ) );
    cJSON_AddItemToObject( root, "action", cJSON_CreateString( "message_queue_changed" ) );

    res = perform_put( url, root );
    if ( res != CURLE_OK ) {
            msg_warn("curl_easy_perform() failed: %s", curl_easy_strerror(res) );
    }

    /* Cleanup JSON object*/
    cJSON_Delete( root );
}

/*
/* queue_id -- internal message id
/* wait_time -- seconds until next retry
/* action = "message_next_retry"
*/
void restlog_change_wait_time( const char *url, const char *queue_id, size_t wait_time) {
    cJSON *root;
    CURLcode res;

    /* Create JSON object to transfer to index */
    root = cJSON_CreateObject();
    cJSON_AddItemToObject( root, "queue_id", cJSON_CreateString( queue_id ) );
    cJSON_AddItemToObject( root, "wait_time", cJSON_CreateNumber( wait_time ) );
    cJSON_AddItemToObject( root, "action", cJSON_CreateString( "message_next_retry" ) );

    res = perform_put( url, root );
    if ( res != CURLE_OK ) {
            msg_warn("curl_easy_perform() failed: %s", curl_easy_strerror(res) );
    }

    /* Cleanup JSON object*/
    cJSON_Delete( root );
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
    cJSON *root;
    CURLcode res;

    /* Create JSON object to transfer to index */
    root = cJSON_CreateObject();
    cJSON_AddItemToObject( root, "queue_id", cJSON_CreateString( queue_id ) );
    cJSON_AddItemToObject( root, "queue_name", cJSON_CreateString( queue_name ) );
    cJSON_AddItemToObject( root, "sender", cJSON_CreateString( env_sender ) );
    cJSON_AddItemToObject( root, "recipient", cJSON_CreateString( recip ) );
    cJSON_AddItemToObject( root, "rcpt_count", cJSON_CreateNumber( rcpt_count ) );
    cJSON_AddItemToObject( root, "msg_size", cJSON_CreateNumber( msg_size ) );
    cJSON_AddItemToObject( root, "subject", cJSON_CreateString( subject ) );
    cJSON_AddItemToObject( root, "action", cJSON_CreateString( "message_added" ) );

    res = perform_put( url, root );
    if ( res != CURLE_OK ) {
            msg_warn("curl_easy_perform() failed: %s", curl_easy_strerror(res) );
    }

    /* Cleanup JSON object*/
    cJSON_Delete( root );
}

/*
/* queue -- name of postfix queue
/* queue_id -- internal message id
/* action = "message_sent"
*/
void restlog_message_sent( const char *url, const char *queue_name, const char *queue_id ) {
    cJSON *root;
    CURLcode res;

    /* Create JSON object to transfer to index */
    root = cJSON_CreateObject();
    cJSON_AddItemToObject( root, "queue_id", cJSON_CreateString( queue_id ) );
    cJSON_AddItemToObject( root, "queue", cJSON_CreateString( queue_name ) );
    cJSON_AddItemToObject( root, "action", cJSON_CreateString( "message_sent" ) );

    res = perform_put( url, root );
    if ( res != CURLE_OK ) {
            msg_warn("curl_easy_perform() failed: %s", curl_easy_strerror(res) );
    }

    /* Cleanup JSON object*/
    cJSON_Delete( root );
}
