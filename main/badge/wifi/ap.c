#include "ap.h"
#include "wifi.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "../badge.h"

esp_timer_handle_t inactivity_timer;
static int ap_clients_num = 0;

static void inactivity_timer_callback(void* arg) {
  if (!ap_clients_num) {
    ESP_LOGI(__FILE__, "Inactivity detected");
    stop_wifi();
  } else
    ESP_LOGE(__FILE__, "Timer should not be running...");
}

static void ap_staconnected_event(void* arg, esp_event_base_t event_base,
                                  int32_t event_id, void* event_data) {
  wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*)event_data;
  ESP_LOGI(__FILE__, "station " MACSTR " join, AID=%d",
           MAC2STR(event->mac), event->aid);
  ap_clients_num++;
  esp_timer_stop(inactivity_timer);
}
static void ap_stadisconnected_event(void* arg, esp_event_base_t event_base,
                                     int32_t event_id, void* event_data) {
  wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*)event_data;
  ESP_LOGI(__FILE__, "station " MACSTR " leave, AID=%d",
           MAC2STR(event->mac), event->aid);
  ap_clients_num--;
  if (ap_clients_num < 0) ap_clients_num = 0;
  if (!ap_clients_num) esp_timer_start_once(inactivity_timer, AP_INACTIVITY_TIMEOUT_S * 10000000);
}

static void ap_wifi_wrong_password_event(void* arg, esp_event_base_t event_base,
                                         int32_t event_id, void* event_data) {
  wifi_event_ap_wrong_password_t* event = (wifi_event_ap_wrong_password_t*)event_data;
  ESP_LOGI(__FILE__, "Wrong password received: %s", MAC2STR(event->mac));
}

bool start_wifi_ap() {
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());

  wifi_config_t wifi_config = {0};
  snprintf((char*)wifi_config.ap.ssid, SIZEOF(wifi_config.ap.ssid), "%s", badge_obj.ap_ssid);
  snprintf((char*)wifi_config.ap.password, SIZEOF(wifi_config.ap.password), "%s", badge_obj.ap_password);
  wifi_config.ap.authmode       = WIFI_AUTH_WPA_WPA2_PSK;
  wifi_config.ap.ssid_len       = strlen(badge_obj.ap_ssid);
  wifi_config.ap.max_connection = AP_MAX_STA_CONN;

  if (strlen(badge_obj.ap_password) == 0) {
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_set_inactive_time(WIFI_IF_AP, AP_INACTIVITY_TIMEOUT_S));

  const esp_timer_create_args_t timer_args = {
      .callback = &inactivity_timer_callback,
      .name     = "inactivity-timer"};

  ESP_ERROR_CHECK(esp_timer_create(&timer_args, &inactivity_timer));

  ESP_LOGI(__FILE__, "WIFI_MODE_AP started. SSID:%s password:|%s|",
           badge_obj.ap_ssid, badge_obj.ap_password);

  curr_mode = WIFI_MODE_AP;
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());
  esp_timer_start_once(inactivity_timer, AP_INACTIVITY_TIMEOUT_S * 10000000);
  return ESP_OK;
}

void stop_wifi_ap() {
  esp_timer_stop(inactivity_timer);
}

void wifi_ap_init() {
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &ap_staconnected_event, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &ap_stadisconnected_event, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_WRONG_PASSWORD, &ap_wifi_wrong_password_event, NULL));
}