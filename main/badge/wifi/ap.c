#include "ap.h"
#include "wifi.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"
#include "../ui/admin.h"

#include "../badge.h"

esp_timer_handle_t inactivity_timer;

int wifi_ap_get_client_count() {
  wifi_sta_list_t stations;
  esp_wifi_ap_get_sta_list(&stations);
  return stations.num;
}

static void wifi_ap_inactivity_timer_cb(void* arg) {
  if (!wifi_ap_get_client_count()) {
    ESP_LOGI(__FILE__, "Inactivity detected");
    wifi_stop_all();
  } else
    ESP_LOGE(__FILE__, "Timer should not be running...");
}

static inline void wifi_ap_start_inactivity_timer() { esp_timer_start_once(inactivity_timer, AP_INACTIVITY_TIMEOUT_S * 10000000); }

static void wifi_ap_staconnected_event(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*)event_data;
  ESP_LOGI(__FILE__, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
  esp_timer_stop(inactivity_timer);
  ui_admin_update_info();
}
static void wifi_ap_stadisconnected_event(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*)event_data;
  ESP_LOGI(__FILE__, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
  if (!wifi_ap_get_client_count()) wifi_ap_start_inactivity_timer();
  ui_admin_update_info();
}

static void wifi_ap_start_event(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  ui_admin_ap_start_event();
}

static void wifi_ap_stop_event(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  ui_admin_ap_stop_event();
}

bool wifi_ap_start() {
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());

  wifi_config_t wifi_config = {0};
  snprintf((char*)wifi_config.ap.ssid, SIZEOF(wifi_config.ap.ssid), "%s", badge_obj.ap_ssid);
  snprintf((char*)wifi_config.ap.password, SIZEOF(wifi_config.ap.password), "%s", badge_obj.ap_password);
  wifi_config.ap.authmode       = WIFI_AUTH_WPA_WPA2_PSK;
  wifi_config.ap.ssid_len       = strlen(badge_obj.ap_ssid);
  wifi_config.ap.max_connection = AP_MAX_STA_CONN;

  if (strlen(badge_obj.ap_password) == 0)
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &wifi_ap_staconnected_event, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &wifi_ap_stadisconnected_event, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_START, &wifi_ap_start_event, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STOP, &wifi_ap_stop_event, NULL));

  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_set_inactive_time(WIFI_IF_AP, AP_INACTIVITY_TIMEOUT_S));

  wifi_ap_start_inactivity_timer();

  ESP_LOGI(__FILE__, "WIFI_MODE_AP started. SSID:%s password:|%s|", badge_obj.ap_ssid, badge_obj.ap_password);
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());

  return ESP_OK;
}

void wifi_ap_stop() {
  ESP_LOGI(__FILE__, "Stopping wifi AP");
  ESP_LOGI(__FILE__, "Stopping Inactivity timer");
  esp_timer_stop(inactivity_timer);
  ESP_LOGI(__FILE__, "Unregister events");
  esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &wifi_ap_staconnected_event);
  esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &wifi_ap_stadisconnected_event);
  esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_AP_START, &wifi_ap_start_event);
  esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_AP_STOP, &wifi_ap_stop_event);
  ESP_LOGI(__FILE__, "Wifi AP stopped");
}

void wifi_ap_init() {
  const esp_timer_create_args_t timer_args = {
      .callback = &wifi_ap_inactivity_timer_cb,
      .name     = "inactivity-timer"};

  ESP_ERROR_CHECK(esp_timer_create(&timer_args, &inactivity_timer));
}