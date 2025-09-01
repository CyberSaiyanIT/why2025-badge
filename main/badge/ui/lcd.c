#include "lcd.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include <lvgl.h>
#include <lvgl_helpers.h>
#include <stdio.h>
#include <stdlib.h>

void lcd_init() {
  ESP_LOGI(__FILE__, "LVGL INIT");
  lv_init();

  ESP_LOGI(__FILE__, "DRIVERS INIT");
  lvgl_driver_init();
}

void lcd_free() {
}