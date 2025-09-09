#ifndef _AP_WIFI_H
#define _AP_WIFI_H

#include <stdbool.h>

#define AP_INACTIVITY_TIMEOUT_S 60
#define AP_MAX_STA_CONN         5

bool start_wifi_ap();
void stop_wifi_ap();
void wifi_ap_init();

#endif