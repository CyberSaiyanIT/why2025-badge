#ifndef _STA_WIFI_H
#define _STA_WIFI_H

#include <stdbool.h>

#define STA_TIMEOUT_MS    20000
#define STA_MAXIMUM_RETRY 5

bool wifi_sta_start();
void wifi_sta_stop();
void wifi_sta_init();

#endif