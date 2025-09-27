#ifndef _SESSION_H
#define _SESSION_H

#include "esp_err.h"
#include "esp_http_server.h"

#define SESSION_KEY_LEN 8

const char *get_session_key();
void session_destroy();
esp_err_t session_init(httpd_req_t *req);
bool session_check(httpd_req_t *req, const char *client_data);

#endif