#include <stdio.h>
#include <string.h>
#include "esp_event.h"
#include "badge/badge.h"
#include "badge/common/i2c.h"
#include "badge/http/httpd.h"
#include "badge/wifi/wifi.h"
#include "badge/ui/ui.h"
#include "badge/common/storage.h"
#include "badge/led.h"
#include "badge/bt.h"

void app_main() {
  ESP_LOGI(__FILE__, "---------- MAIN START(1): free_heap_size = %lu\n", esp_get_free_heap_size());
  // Init storage
  nvs_init();
  spiffs_init();

  // Init event loop
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  badge_init();
  i2c_master_init();
  led_init();
  ESP_LOGI(__FILE__, "---------- MAIN START(2): free_heap_size = %lu\n", esp_get_free_heap_size());
  // start bluetooth
  // bt_init();
  // xTaskCreate(bt_task, "bt_task", 4096, NULL, 6, NULL);
  ESP_LOGI(__FILE__, "---------- MAIN START(3): free_heap_size = %lu\n", esp_get_free_heap_size());
  // start wifi management
  wifi_init();
  xTaskCreate(wifi_task, "wifi_task", 4096, NULL, 5, NULL);
  ESP_LOGI(__FILE__, "---------- MAIN START(4): free_heap_size = %lu\n", esp_get_free_heap_size());
  // start ui.
  xTaskCreate(ui_task, "ui_task", 8192, NULL, 0, NULL);
  ESP_LOGI(__FILE__, "---------- MAIN START(5): free_heap_size = %lu\n", esp_get_free_heap_size());
  xTaskCreate(led_task, "led_task", 2048, NULL, 10, NULL);

  // Handle HTTP webserver start/stop
  httpd_init();
  ESP_LOGI(__FILE__, "---------- MAIN END: free_heap_size = %lu\n", esp_get_free_heap_size());
}
