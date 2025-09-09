#ifndef _STA_WIFI_H
#define _STA_WIFI_H

#include <stdbool.h>

#define STA_TIMEOUT_MS    20000
#define STA_MAXIMUM_RETRY 5

bool start_wifi_sta();
void stop_wifi_sta();
void wifi_sta_init();

#endif