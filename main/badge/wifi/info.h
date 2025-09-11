#ifndef _WIFI_INFO_H
#define _WIFI_INFO_H
#include "esp_err.h"
#include "esp_wifi.h"

#define IP_BUFF_SIZE 20

typedef struct 
{
  char ip[IP_BUFF_SIZE];
  char gateway[IP_BUFF_SIZE];
  char netmask[IP_BUFF_SIZE];
  wifi_mode_t wifi_mode;
} wifi_info_t;

esp_err_t wifi_update_ip_info(wifi_info_t *wifi_info);
#endif