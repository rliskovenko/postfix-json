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

static size_t read_callback( const void *out, const size_t size, const size_t nmemb, const void *in) {
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

CURLcode perform_put( const char *url, const cJSON *json ) {
    struct inmem_s json_data;
    CURLcode res;

    json_data.ptr = cJSON_PrintUnformatted( json );
    json_data.left = strlen( json_data.ptr );
    if ( msg_verbose )
        msg_info("json_data: %s", json_data.ptr );
    curl_handler = curl_easy_init();
    if ( curl_handler ) {
        curl_easy_setopt( curl_handler, CURLOPT_URL, url );
        curl_easy_setopt( curl_handler, CURLOPT_NOPROGRESS, 1L );
        curl_easy_setopt( curl_handler, CURLOPT_TIMEOUT, 5L );
        curl_easy_setopt( curl_handler, CURLOPT_VERBOSE, 0L );
        curl_easy_setopt( curl_handler, CURLOPT_UPLOAD, 1L );
        curl_easy_setopt( curl_handler, CURLOPT_READDATA, &json_data );
        curl_easy_setopt( curl_handler, CURLOPT_READFUNCTION, read_callback );
        curl_easy_setopt( curl_handler, CURLOPT_INFILESIZE, json_data.left );
        // Still OK until this moment
        res = curl_easy_perform( curl_handler );
        curl_easy_cleanup( curl_handler );
    } else {
        msg_error("curl_handler BROKEN!");
    }
    return res;
}

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

void restlog_message_sent( const char *url, const char *queue_name, const char *queue_id ) {
    cJSON *root;
    CURLcode res;

    /* Create JSON object to transfer to index */
    root = cJSON_CreateObject();
    cJSON_AddItemToObject( root, "queue_id", cJSON_CreateString( queue_id ) );
    cJSON_AddItemToObject( root, "queue", cJSON_CreateString( queue_name ) );
    cJSON_AddItemToObject( root, "finished", cJSON_CreateNumber( 1 ) );
    cJSON_AddItemToObject( root, "action", cJSON_CreateString( "message_sent" ) );

    res = perform_put( url, root );
    if ( res != CURLE_OK ) {
            msg_warn("curl_easy_perform() failed: %s", curl_easy_strerror(res) );
    }

    /* Cleanup JSON object*/
    cJSON_Delete( root );
}
