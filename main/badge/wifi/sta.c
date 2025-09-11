#include "sta.h"

#include "wifi.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "../ui/admin.h"
#include "../schedule.h"

static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;
const int FAIL_BIT      = BIT1;

static int retry_num = 0;

static void wifi_sta_got_ip_event(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
  ESP_LOGI(__FILE__, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
  retry_num = 0;
  xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
  ui_admin_sta_connected_event();
}

static void wifi_sta_disconnect_event(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  if (retry_num < STA_MAXIMUM_RETRY) {
    retry_num++;
    ui_admin_connection_progress(retry_num, STA_MAXIMUM_RETRY);
    esp_wifi_connect();
    ESP_LOGI(__FILE__, "retry to connect to the AP (%d/%d)", retry_num, STA_MAXIMUM_RETRY);
    xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
  } else
    xEventGroupSetBits(wifi_event_group, FAIL_BIT);
  ui_admin_sta_disconnected_event();
}

static void wifi_sta_stop_event(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  ui_admin_sta_stop_event();
}

bool wifi_sta_start() {
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());

  wifi_config_t wifi_config = {0};
  snprintf((char*)wifi_config.sta.ssid, SIZEOF(wifi_config.sta.ssid), "%s", badge_obj.sta_ssid);

  // Only set password if it's not empty (for open networks)
  if (strlen(badge_obj.sta_password) > 0)
    snprintf((char*)wifi_config.sta.password, SIZEOF(wifi_config.sta.password), "%s", badge_obj.sta_password);

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_sta_got_ip_event, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &wifi_sta_disconnect_event, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_STOP, &wifi_sta_stop_event, NULL));

  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_connect());

  int bits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, pdFALSE, pdTRUE, STA_TIMEOUT_MS / portTICK_PERIOD_MS);
  ESP_LOGI(__FILE__, "bits=%x", bits);
  retry_num = 0;
  if (bits & CONNECTED_BIT) {
    ESP_LOGI(__FILE__, "WIFI_MODE_STA connected. SSID:%s password:%s", badge_obj.sta_ssid, badge_obj.sta_password);
    return true;
  }
  ESP_LOGI(__FILE__, "WIFI_MODE_STA can't connected. SSID:%s password:%s", badge_obj.sta_ssid, badge_obj.sta_password);
  return false;
}

void wifi_sta_stop() {
  ESP_LOGI(__FILE__, "Stopping wifi STA");
  esp_wifi_disconnect();
  ESP_LOGI(__FILE__, "Unregister events");
  esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_sta_got_ip_event);
  esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &wifi_sta_disconnect_event);
  esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_STOP, &wifi_sta_stop_event);
  ESP_LOGI(__FILE__, "Wifi STA stopped");
}

void wifi_sta_init() {
  wifi_event_group = xEventGroupCreate();
}