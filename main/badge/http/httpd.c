#include "httpd.h"

#include "file.h"
#include "api.h"
#include "session.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"

#define REST_TAG __FILE__

static uint8_t client_count;
httpd_handle_t httpd_server = NULL;

static esp_err_t start_webserver(void) {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.uri_match_fn   = httpd_uri_match_wildcard;

  client_count = 0;

  // Start the httpd server
  ESP_LOGI(__FILE__, "Starting server on port: '%d'", config.server_port);
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());
  esp_err_t ret = httpd_start(&httpd_server, &config);
  if (ret == ESP_OK) {
    // Registering the ws handler
    ESP_LOGI(__FILE__, "Registering URI handlers");
    /* URI handler for fetching system info */
    httpd_uri_t common_post_uri = {
        .uri      = API_ENDPOINT_WILDCARD,
        .method   = HTTP_POST,
        .handler  = post_handler,
        .user_ctx = NULL};
    ESP_ERROR_CHECK(httpd_register_uri_handler(httpd_server, &common_post_uri));

    /* URI handler for getting web server files */
    httpd_uri_t common_get_uri = {
        .uri      = "/*",
        .method   = HTTP_GET,
        .handler  = get_handler,
        .user_ctx = NULL};
    ESP_ERROR_CHECK(httpd_register_uri_handler(httpd_server, &common_get_uri));
  } else {
    ESP_LOGI(__FILE__, "Error starting server! %s", esp_err_to_name(ret));
    return ret;
  }
  return ESP_OK;
}

static void httpd_connect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  client_count++;
  ESP_LOGI(__FILE__, "Number of clients: %d", client_count);

  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());
  if (httpd_server == NULL) {
    ESP_LOGI(__FILE__, "Starting webserver");
    ESP_ERROR_CHECK(start_webserver());
  }
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());
}

static void httpd_disconnect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  if (client_count > 0)
    client_count--;
  ESP_LOGI(__FILE__, "Number of clients: %d", client_count);

  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());
  if (httpd_server != NULL && !client_count) {
    session_destroy();
    ESP_LOGI(__FILE__, "Stopping webserver");

    if (httpd_stop(httpd_server) == ESP_OK)
      httpd_server = NULL;
    else
      ESP_LOGE(__FILE__, "Failed to stop http server");
  }
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());
}

void httpd_init() {
  httpd_server = NULL;

  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &httpd_connect_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &httpd_disconnect_handler, NULL));
}