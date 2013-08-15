#ifndef REST_H_INCLUDED
#define REST_H_INCLUDED

#include <curl/curl.h>
#include "cJSON.h"

//extern static CURL *curl_handler;
//extern char *var_queue_metadata_url;
//extern char *var_queue_metadata_path;

struct inmem_s {
    const char *ptr;
    size_t left;
};

CURLcode perform_put( char *url, cJSON *json );
void restlog_change_queue( const char *queue_from, const char *queue_to, const char *queue_id );
void restlog_change_wait_time( const char *queue_id, size_t wait_time);
//const char*orig_rcpt
//const char *reason
//const char *smtp_reply
void restlog_queued( const char *queue_id, const char *queue_name,
    const char *env_sender, const int rcpt_count,
    const unsigned long msg_size );
void restlog_message_sent( const char *queue_name, const char *queue_id );

#endif // REST_H_INCLUDED
