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

static size_t read_callback( void *out, size_t size, size_t nmemb, void *in) {
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

void restlog_change_queue( const char *queue_from, const char *queue_to, const char *queue_id ) {
    cJSON *root;
    CURLcode res;
    struct curl_slist *hdrs = NULL;
    struct inmem_s json_data;

    /* Create JSON object to transfer to index */
    root = cJSON_CreateObject();
    cJSON_AddItemToObject( root, "queue_from", cJSON_CreateString( queue_from ) );
    cJSON_AddItemToObject( root, "queue_id", cJSON_CreateString( queue_id ) );
    cJSON_AddItemToObject( root, "queue_to", cJSON_CreateString( queue_to ) );
    json_data.ptr = cJSON_PrintUnformatted( root );
    json_data.left = strlen( json_data.ptr );
    if ( msg_verbose )
        msg_info("json_data: %s", json_data.ptr );

    /* Use libcurl to send JSON to server */
    //if ( !curl_handler )
        curl_handler = curl_easy_init();
    if ( curl_handler ) {
        //curl_slist_append( hdrs, "Transfer-Encoding: chunked");
        curl_easy_setopt( curl_handler, CURLOPT_URL, "http://127.0.0.1:9300/postfix/message" );
        curl_easy_setopt( curl_handler, CURLOPT_NOPROGRESS, 1L );
        curl_easy_setopt( curl_handler, CURLOPT_TIMEOUT, 5L );
        curl_easy_setopt( curl_handler, CURLOPT_VERBOSE, 0L );
        curl_easy_setopt( curl_handler, CURLOPT_UPLOAD, 1L );
        //curl_easy_setopt( curl_handler, CURLOPT_PUT, 1L );
        curl_easy_setopt( curl_handler, CURLOPT_READDATA, &json_data );
        curl_easy_setopt( curl_handler, CURLOPT_READFUNCTION, read_callback );
        curl_easy_setopt( curl_handler, CURLOPT_INFILESIZE, json_data.left );
        // Still OK until this moment
        res = curl_easy_perform( curl_handler );
        if ( res != CURLE_OK ) {
            msg_warn("curl_easy_perform() failed: %s", curl_easy_strerror(res) );
        }
        curl_easy_cleanup( curl_handler );
    } else {
        msg_error("curl_handler BROKEN!");
    }
    /* Cleanup JSON object*/
    cJSON_Delete( root );
}
