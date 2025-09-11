#ifndef _FILE_H
#define _FILE_H

#include "esp_http_server.h"
#include "esp_vfs.h"
#include "esp_err.h"

esp_err_t httpd_file_handler(httpd_req_t *req);

#endif