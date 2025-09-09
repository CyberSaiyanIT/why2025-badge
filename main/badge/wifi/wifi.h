#ifndef WIFI__H
#define WIFI__H

#include <string.h>
#include "freertos/FreeRTOS.h"

#define AP_INACTIVITY_TIMEOUT_S 60
#define AP_MAX_STA_CONN         5
#define STA_TIMEOUT_MS          20000
#define STA_MAXIMUM_RETRY       5

extern QueueHandle_t wifi_queue;
enum enum_badge_event {
  EVENT_HOTSPOT_START,
  EVENT_HOTSPOT_STOP,
  EVENT_STA_START,
  EVENT_STA_STOP,
  EVENT_SYNC_START,
  EVENT_SYNC_STOP
};

void wifi_init(void);

bool start_wifi_ap(void);
bool start_wifi_sta(void);
bool start_wifi_apsta(void);

void stop_wifi(void);

void wifi_task(void *);

#endif