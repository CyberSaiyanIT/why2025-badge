#include "httpd.h"

#include "file.h"
#include "api.h"
#include "session.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"

#define REST_TAG __FILE__

httpd_handle_t httpd_server = NULL;

static esp_err_t httpd_start_webserver(void) {
  if (httpd_server != NULL)
    return ESP_FAIL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.uri_match_fn   = httpd_uri_match_wildcard;

  // Start the httpd server
  ESP_LOGI(__FILE__, "Starting HTTPD server on port: '%d'", config.server_port);
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());
  esp_err_t ret = httpd_start(&httpd_server, &config);
  if (ret == ESP_OK) {
    // Registering the ws handler
    ESP_LOGI(__FILE__, "Registering URI handlers");
    /* URI handler for fetching system info */
    httpd_uri_t common_post_uri = {
        .uri      = API_ENDPOINT_WILDCARD,
        .method   = HTTP_POST,
        .handler  = httpd_api_handler,
        .user_ctx = NULL};
    ESP_ERROR_CHECK(httpd_register_uri_handler(httpd_server, &common_post_uri));

    /* URI handler for getting web server files */
    httpd_uri_t common_get_uri = {
        .uri      = "/*",
        .method   = HTTP_GET,
        .handler  = httpd_file_handler,
        .user_ctx = NULL};
    ESP_ERROR_CHECK(httpd_register_uri_handler(httpd_server, &common_get_uri));
  } else {
    ESP_LOGI(__FILE__, "Error starting server! %s", esp_err_to_name(ret));
    return ret;
  }
  return ESP_OK;
}

static esp_err_t httpd_stop_webserver(void) {
  if (httpd_server == NULL)
    return ESP_FAIL;
  session_destroy();
  httpd_stop(httpd_server);
  httpd_server = NULL;
  return ESP_OK;
}

static void httpd_start_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());
  ESP_ERROR_CHECK(httpd_start_webserver());
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());
}

static void httpd_stop_event(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());
  ESP_ERROR_CHECK(httpd_stop_webserver());
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());
}

void httpd_init() {
  httpd_server = NULL;

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_START, &httpd_start_event, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STOP, &httpd_stop_event, NULL));
}