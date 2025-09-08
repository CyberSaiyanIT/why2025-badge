#include <stdio.h>
#include <string.h>

#include "badge/badge.h"
#include "badge/common/i2c.h"
#include "badge/http/httpd.h"

void app_main() {
  usleep(10 * 1000000UL);
  ESP_LOGI(__FILE__, "---------- MAIN START(1): free_heap_size = %lu\n", esp_get_free_heap_size());
  badge_init();
  i2c_master_init();
  // led_init();
  ESP_LOGI(__FILE__, "---------- MAIN START(2): free_heap_size = %lu\n", esp_get_free_heap_size());
  // start bluetooth
  bt_init();
  xTaskCreate(bt_task, "bt_task", 4096, NULL, 6, NULL);
  ESP_LOGI(__FILE__, "---------- MAIN START(3): free_heap_size = %lu\n", esp_get_free_heap_size());
  // start wifi management
  wifi_init();
  xTaskCreate(wifi_task, "wifi_task", 4096, NULL, 5, NULL);
  ESP_LOGI(__FILE__, "---------- MAIN START(4): free_heap_size = %lu\n", esp_get_free_heap_size());
  // start ui.
  xTaskCreate(ui_task, "ui_task", 8192, NULL, 0, NULL);
  ESP_LOGI(__FILE__, "---------- MAIN START(5): free_heap_size = %lu\n", esp_get_free_heap_size());
  // xTaskCreate(led_task, "led_task", 2048, NULL, 10, NULL);

  // Handle HTTP webserver start/stop
  httpd_init();
  ESP_LOGI(__FILE__, "---------- MAIN END: free_heap_size = %lu\n", esp_get_free_heap_size());
}
