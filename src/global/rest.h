#ifndef REST_H_INCLUDED
#define REST_H_INCLUDED

//extern static CURL *curl_handler;
//extern char *var_queue_metadata_url;
//extern char *var_queue_metadata_path;

struct inmem_s {
    const char *ptr;
    size_t left;
};

void restlog_change_queue( const char *queue_from, const char *queue_to, const char *queue_id );

#endif // REST_H_INCLUDED
