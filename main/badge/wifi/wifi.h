#ifndef WIFI__H
#define WIFI__H

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"
#include "esp_wifi.h"

enum badge_wifi_event_t {
  EVENT_AP_START,
  EVENT_STA_START,
  EVENT_SYNC,
  EVENT_STOP
} ;

void stop_wifi(void);
void wifi_init(void);
void wifi_task(void*);

void start_ap();
void start_sta();
void start_sync();
void stop_all();

#endif