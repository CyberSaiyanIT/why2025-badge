#ifndef WIFI__H
#define WIFI__H

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"
#include "esp_wifi.h"

enum badge_wifi_event_t {
  EVENT_AP_START,
  EVENT_STA_START,
  EVENT_STOP
};

void wifi_init(void);
void wifi_task(void*);

void wifi_start_ap();
void wifi_start_sta();
void wifi_stop_all();

#endif