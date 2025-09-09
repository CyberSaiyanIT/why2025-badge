#include "sta.h"

#include "wifi.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "../ui/admin.h"
#include "../schedule.h"

EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;
const int FAIL_BIT      = BIT1;

static int retry_num = 0;

static void sta_disconnect_event(void* arg, esp_event_base_t event_base,
                                 int32_t event_id, void* event_data) {
  if (retry_num < STA_MAXIMUM_RETRY) {
    ui_connection_progress(retry_num + 1, STA_MAXIMUM_RETRY);
    esp_wifi_connect();
    retry_num++;
    ESP_LOGI(__FILE__, "retry to connect to the AP (%d/%d)", retry_num, STA_MAXIMUM_RETRY);
    xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
  } else
    xEventGroupSetBits(wifi_event_group, FAIL_BIT);
}

static void sta_got_ip_event(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data) {
  ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
  ESP_LOGI(__FILE__, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
  retry_num = 0;
  xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
}

bool start_wifi_sta() {
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());

  wifi_config_t wifi_config = {0};
  snprintf((char*)wifi_config.sta.ssid, SIZEOF(wifi_config.sta.ssid), "%s", badge_obj.sta_ssid);

  // Only set password if it's not empty (for open networks)
  if (strlen(badge_obj.sta_password) > 0)
    snprintf((char*)wifi_config.sta.password, SIZEOF(wifi_config.sta.password), "%s", badge_obj.sta_password);

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_connect());

  int bits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                                 pdFALSE, pdTRUE, STA_TIMEOUT_MS / portTICK_PERIOD_MS);
  ESP_LOGI(__FILE__, "bits=%x", bits);
  if (bits & CONNECTED_BIT) {
    ESP_LOGI(__FILE__, "WIFI_MODE_STA connected. SSID:%s password:%s",
             badge_obj.sta_ssid, badge_obj.sta_password);
    schedule_sync_handler(true);
  } else {
    ESP_LOGI(__FILE__, "WIFI_MODE_STA can't connected. SSID:%s password:%s",
             badge_obj.sta_ssid, badge_obj.sta_password);
    stop_wifi();
  }

  curr_mode = WIFI_MODE_STA;
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());

  retry_num = 0;
  return (bits & CONNECTED_BIT) != 0;
}

void stop_wifi_sta() {}

void wifi_sta_init() {
  wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &sta_disconnect_event, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &sta_got_ip_event, NULL));
}