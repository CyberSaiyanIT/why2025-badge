#ifndef __SPIFFS_H__
#define __SPIFFS_H__

#include <stdio.h>
#include <string.h>

#include "nvs_flash.h"
#include "esp_spiffs.h"
#include "esp_err.h"
#include "esp_log.h"

esp_err_t nvs_init();
esp_err_t spiffs_size();
esp_err_t spiffs_init();

#endif