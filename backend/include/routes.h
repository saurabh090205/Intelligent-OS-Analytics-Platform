#ifndef ROUTES_H
#define ROUTES_H

#include "json_utils.h"

typedef struct {
    int status_code;
    const char *status_text;
    const char *content_type;
    char *body;
    size_t body_len;
} http_response_t;

void http_response_init(http_response_t *res);
void http_response_free(http_response_t *res);
void http_response_set_json(http_response_t *res, int status_code, json_builder_t *jb);
void http_response_set_error(http_response_t *res, int status_code, const char *error_msg);

void handle_api_request(const char *method, const char *path, const char *query,
                        const char *req_body, http_response_t *res);

#endif /* ROUTES_H */
