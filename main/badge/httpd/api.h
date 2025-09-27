#ifndef _API_H
#define _API_H

#include "esp_http_server.h"
#include "esp_err.h"

esp_err_t httpd_api_handler(httpd_req_t *req);
#endif