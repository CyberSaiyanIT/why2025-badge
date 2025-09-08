#include "wifi.h"
#include "ui/admin.h"

static const char* TAG       = "WIFI";
static wifi_mode_t curr_mode = WIFI_MODE_NULL;
QueueHandle_t wifi_queue;

static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;
const int FAIL_BIT      = BIT1;

static int retry_num      = 0;
static int ap_clients_num = 0;
static esp_timer_handle_t inactivity_timer;


static void sta_disconnect_event(void* arg, esp_event_base_t event_base,
                                 int32_t event_id, void* event_data) {
  if (retry_num < STA_MAXIMUM_RETRY) {
    ui_connection_progress(retry_num + 1, STA_MAXIMUM_RETRY);
    esp_wifi_connect();
    retry_num++;
    ESP_LOGI(TAG, "retry to connect to the AP (%d/%d)", retry_num, STA_MAXIMUM_RETRY);
    xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
  } else
    xEventGroupSetBits(wifi_event_group, FAIL_BIT);
}

static void sta_got_ip_event(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data) {
  ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
  ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
  retry_num = 0;
  xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
}
static void ap_staconnected_event(void* arg, esp_event_base_t event_base,
                                  int32_t event_id, void* event_data) {
  wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*)event_data;
  ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
           MAC2STR(event->mac), event->aid);
  ap_clients_num++;
  esp_timer_stop(inactivity_timer);
}
static void ap_stadisconnected_event(void* arg, esp_event_base_t event_base,
                                     int32_t event_id, void* event_data) {
  wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*)event_data;
  ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
           MAC2STR(event->mac), event->aid);
  ap_clients_num--;
  if (ap_clients_num < 0) ap_clients_num = 0;
  if (!ap_clients_num) esp_timer_start_once(inactivity_timer, AP_INACTIVITY_TIMEOUT_S * 10000000);
}

static void ap_wifi_wrong_password_event(void* arg, esp_event_base_t event_base,
                                         int32_t event_id, void* event_data) {
  wifi_event_ap_wrong_password_t* event = (wifi_event_ap_wrong_password_t*)event_data;
  ESP_LOGI(TAG, "Wrong password received: %s", MAC2STR(event->mac));
}

void wifi_init(void) {
  esp_log_level_set("wifi", ESP_LOG_WARN);
  static bool initialized = false;
  if (initialized)
    return;

  ESP_ERROR_CHECK(esp_netif_init());
  wifi_event_group = xEventGroupCreate();

  esp_netif_t* ap_netif = esp_netif_create_default_wifi_ap();
  assert(ap_netif);
  esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
  assert(sta_netif);
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &sta_disconnect_event, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &sta_got_ip_event, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &ap_staconnected_event, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &ap_stadisconnected_event, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_WRONG_PASSWORD, &ap_wifi_wrong_password_event, NULL));
  // ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &event_handler, NULL) );

  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
  ESP_ERROR_CHECK(esp_wifi_start());

  curr_mode = WIFI_MODE_NULL;
  initialized = true;
}

static void inactivity_timer_callback(void* arg) {
  if (!ap_clients_num) {
    ESP_LOGI(__FILE__, "Inactivity detected");
    stop_wifi();
  } else 
    ESP_LOGE(__FILE__, "Timer should not be running...");
}

bool start_wifi_ap(void) {
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

  ESP_LOGI(TAG, "WIFI_MODE_AP started. SSID:%s password:|%s|",
           badge_obj.ap_ssid, badge_obj.ap_password);

  curr_mode = WIFI_MODE_AP;
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());

  return ESP_OK;
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
  ESP_LOGI(TAG, "bits=%x", bits);
  if (bits & CONNECTED_BIT) {
    ESP_LOGI(TAG, "WIFI_MODE_STA connected. SSID:%s password:%s",
             badge_obj.sta_ssid, badge_obj.sta_password);
    schedule_sync_handler(true);
  } else {
    ESP_LOGI(TAG, "WIFI_MODE_STA can't connected. SSID:%s password:%s",
             badge_obj.sta_ssid, badge_obj.sta_password);
    stop_wifi();
  }

  curr_mode = WIFI_MODE_STA;
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());

  return (bits & CONNECTED_BIT) != 0;
}

void stop_wifi() {
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());

  esp_wifi_disconnect();
  esp_wifi_stop();
  esp_wifi_set_mode(WIFI_MODE_NULL);
  ESP_LOGI(TAG, "WIFI disabled");
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());
}

void wifi_task(void* arg) {
  ESP_LOGI(__FILE__, "Starting Wifi task");

  wifi_queue = xQueueCreate(4, sizeof(uint32_t));
  uint32_t wifi_event;

  while (1) {
    if (!xQueueReceive(wifi_queue, &wifi_event, portMAX_DELAY))
      continue;

    switch (wifi_event) {
      case EVENT_HOTSPOT_START:
        ESP_LOGI(__FILE__, "EVENT_HOTSPOT_START received");
        stop_wifi();
        start_wifi_ap();
        esp_timer_start_once(inactivity_timer, AP_INACTIVITY_TIMEOUT_S * 10000000);
        break;
      case EVENT_STA_START:
        ESP_LOGI(__FILE__, "EVENT_STA_START received");
        stop_wifi();
        start_wifi_sta();
        retry_num = 0;
        break;
      case EVENT_SYNC_START:
        ESP_LOGI(__FILE__, "EVENT_SYNC_START received");
        schedule_sync_handler(true);
        break;
      case EVENT_HOTSPOT_STOP:
        ESP_LOGI(__FILE__, "EVENT_HOTSPOT_STOP received");
        stop_wifi();
        esp_timer_stop(inactivity_timer);
        break;
      case EVENT_STA_STOP:
        ESP_LOGI(__FILE__, "EVENT_STA_STOP received");
        stop_wifi();
        esp_timer_stop(inactivity_timer);
        break;
      default:
        ESP_LOGI(__FILE__, "not exists event 0x%04" PRIx32, wifi_event);
        break;
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}