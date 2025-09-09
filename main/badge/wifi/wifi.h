#ifndef WIFI__H
#define WIFI__H

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"
#include "esp_wifi.h"
 

extern QueueHandle_t wifi_queue;
enum enum_badge_event {
  EVENT_HOTSPOT_START,
  EVENT_HOTSPOT_STOP,
  EVENT_STA_START,
  EVENT_STA_STOP,
  EVENT_SYNC_START,
  EVENT_SYNC_STOP
};

extern wifi_mode_t curr_mode;

void stop_wifi(void);
void wifi_init(void);
void wifi_task(void*);

#endif