#include "httpd.h"

#include "file.h"
#include "api.h"
#include "session.h"

#define REST_TAG __FILE__

static uint8_t client_count;
static httpd_handle_t *server;

static httpd_handle_t start_webserver(void) {
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.uri_match_fn   = httpd_uri_match_wildcard;

  client_count = 0;

  // Start the httpd server
  ESP_LOGI(__FILE__, "Starting server on port: '%d'", config.server_port);
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());
  esp_err_t ret = httpd_start(&server, &config);
  if (ret == ESP_OK) {
    // Registering the ws handler
    ESP_LOGI(__FILE__, "Registering URI handlers");
    /* URI handler for fetching system info */
    httpd_uri_t common_post_uri = {
        .uri      = API_ENDPOINT_WILDCARD,
        .method   = HTTP_POST,
        .handler  = post_handler,
        .user_ctx = NULL};
    httpd_register_uri_handler(server, &common_post_uri);

    /* URI handler for getting web server files */
    httpd_uri_t common_get_uri = {
        .uri      = "/*",
        .method   = HTTP_GET,
        .handler  = get_handler,
        .user_ctx = NULL};
    httpd_register_uri_handler(server, &common_get_uri);
    return server;
  } else {
    ESP_LOGI(__FILE__, "Error starting server! %s", esp_err_to_name(ret));
    return NULL;
  }
}

static esp_err_t stop_webserver(httpd_handle_t server) {
  // Stop the httpd server
  session_destroy();
  return httpd_stop(server);
}

void connect_handler(void *arg, esp_event_base_t event_base,
                     int32_t event_id, void *event_data) {
  client_count++;
  ESP_LOGI(__FILE__, "Number of clients: %d", client_count);

  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());
  if (server == NULL) {
    ESP_LOGI(__FILE__, "Starting webserver");
    server = start_webserver();
  }
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());
}

void disconnect_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data) {
  if (client_count > 0)
    client_count--;
  ESP_LOGI(__FILE__, "Number of clients: %d", client_count);

  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());
  if (server != NULL && !client_count) {
    ESP_LOGI(__FILE__, "Stopping webserver");
    if (stop_webserver(*server) == ESP_OK) {
      server = NULL;
    } else {
      ESP_LOGE(__FILE__, "Failed to stop http server");
    }
  }
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());
}

void httpd_init() {
  server = NULL;

  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &connect_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &disconnect_handler, NULL));
}