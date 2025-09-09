#include "wifi.h"
#include "../badge.h"
#include "../badge.h"
#include "../schedule.h"

#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"

#include "ap.h"
#include "sta.h"

wifi_mode_t curr_mode = WIFI_MODE_NULL;
QueueHandle_t wifi_queue;

void wifi_init(void) {
  static bool initialized = false;
  if (initialized)
    return;

  ESP_ERROR_CHECK(esp_netif_init());
  esp_netif_create_default_wifi_ap();
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  wifi_ap_init();
  wifi_sta_init();

  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
  ESP_ERROR_CHECK(esp_wifi_start());

  curr_mode   = WIFI_MODE_NULL;
  initialized = true;
}

void stop_wifi() {
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());

  esp_wifi_disconnect();
  esp_wifi_stop();
  esp_wifi_set_mode(WIFI_MODE_NULL);
  ESP_LOGI(__FILE__, "WIFI disabled");
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());
}

void wifi_task(void* arg) {
  ESP_LOGI(__FILE__, "Starting Wifi task");

  wifi_queue = xQueueCreate(4, sizeof(uint32_t));
  uint32_t wifi_event;

  while (1) {
    if (xQueueReceive(wifi_queue, &wifi_event, portMAX_DELAY))
      switch (wifi_event) {
        case EVENT_HOTSPOT_START:
          ESP_LOGI(__FILE__, "EVENT_HOTSPOT_START received");
          stop_wifi();
          start_wifi_ap();
          break;
        case EVENT_STA_START:
          ESP_LOGI(__FILE__, "EVENT_STA_START received");
          stop_wifi();
          start_wifi_sta();
          break;
        case EVENT_SYNC_START:
          ESP_LOGI(__FILE__, "EVENT_SYNC_START received");
          schedule_sync_handler(true);
          break;
        case EVENT_HOTSPOT_STOP:
          ESP_LOGI(__FILE__, "EVENT_HOTSPOT_STOP received");
          stop_wifi_ap();
          stop_wifi();
          break;
        case EVENT_STA_STOP:
          ESP_LOGI(__FILE__, "EVENT_STA_STOP received");
          stop_wifi_sta();
          stop_wifi();
          break;
        default:
          ESP_LOGI(__FILE__, "not exists event 0x%04" PRIx32, wifi_event);
          break;
      }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}