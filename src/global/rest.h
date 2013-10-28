#ifndef REST_H_INCLUDED
#define REST_H_INCLUDED

#include <curl/curl.h>
#include <json/json.h>

#define QMSG_SENT 1
#define QMSG_EXPIRED 2

struct inmem_s {
    const char *ptr;
    size_t left;
};

CURLcode perform_put( const char *url, json_object *json );
void restlog_change_queue( const char *url, const char *queue_from,
    const char *queue_to, const char *queue_id );
void restlog_change_wait_time( const char *url, const char *queue_id,
    size_t wait_time);
void restlog_queued( const char *url, const char *queue_id,
    const char *queue_name, const char *env_sender,
    const char* recip, const int rcpt_count, const char *subject,
    const unsigned long msg_size );
void restlog_message_discarded( const char *url, const char *queue_id,
    const char *queue_name);
void restlog_message_sent( const char *url, const char *queue_name,
    const char *queue_id, const int flag );

#endif // REST_H_INCLUDED
