#include "ui.h"
#include "../led.h"
#include "lcd.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sys/param.h>
#include <unistd.h>



lv_obj_t *screens[NUM_SCREENS];
int8_t current_screen;


void restore_current_timer() {
  if (current_screen == SCREEN_RSSI)
    lv_timer_resume(rssi_timer_handle);
  else if (current_screen == SCREEN_RADAR)
    lv_timer_resume(radar_timer_handle);
}

void pause_current_timer() {
  if (current_screen == SCREEN_RSSI)
    lv_timer_pause(rssi_timer_handle);
  else if (current_screen == SCREEN_RADAR)
    lv_timer_pause(radar_timer_handle);
}

void scroll_up() {
  lv_obj_t *object = lv_obj_get_child(screens[current_screen], 0);
  lv_obj_scroll_by(object, 0, 80, LV_ANIM_ON);
}

void scroll_down() {
  lv_obj_t *object = lv_obj_get_child(screens[current_screen], 0);
  lv_obj_scroll_by(object, 0, -80, LV_ANIM_ON);
}

void ui_switch_page_down() {
  ui_update_backlight(true);

  current_screen++;
  current_screen %= NUM_SCREENS;
  ESP_LOGI("DISPLAY", "DISPLAY COUNTER: %d/%d", current_screen + 1,
           NUM_SCREENS);

  lv_screen_load_anim(screens[current_screen], LV_SCR_LOAD_ANIM_OVER_TOP, 300,
                      0, false);

  restore_current_timer();
}

void ui_switch_page_up() {
  ui_update_backlight(true);

  current_screen--;
  current_screen = (NUM_SCREENS + (current_screen % NUM_SCREENS)) % NUM_SCREENS;
  ESP_LOGI("DISPLAY", "DISPLAY COUNTER: %d/%d", current_screen + 1,
           NUM_SCREENS);

  lv_screen_load_anim(screens[current_screen], LV_SCR_LOAD_ANIM_OVER_BOTTOM,
                      300, 0, false);

  restore_current_timer();
}

static void ui_init(void) {
  current_screen = SCREEN_PERSON;

  screens[SCREEN_LOGO]         = ui_screen_splash_init();
  screens[SCREEN_PERSON]       = ui_screen_person_init();
  screens[SCREEN_SOCIALENERGY] = ui_screen_socialenergy_init();
  screens[SCREEN_EVENT]        = ui_screen_event_init();
  screens[SCREEN_RADAR]        = ui_screen_radar_init();
  screens[SCREEN_RSSI]         = ui_screen_rssi_init();
  screens[SCREEN_ADMIN]        = ui_screen_admin_init();
  screens[SCREEN_SNAKE]        = ui_screen_snake_init();

  // show first page.
  lv_screen_load(screens[current_screen]);

  // Turn on backlight and run backlight management task
  ui_update_backlight(true);
  backlight_init();
}

/*****  TASK *****/

static void ui_tick_task(void *arg) { lv_tick_inc(1); }

void ui_task(void *arg) {
  ESP_LOGI(__FILE__, "Starting UI task");
  SemaphoreHandle_t xGuiSemaphore;
  xGuiSemaphore = xSemaphoreCreateMutex();

  lcd_init();

  ESP_LOGI(__FILE__, "LVGL READY");
  const esp_timer_create_args_t periodic_timer_args = {
      .callback = &ui_tick_task,
      .name     = "ui_tick_task",
  };
  esp_timer_handle_t periodic_timer = NULL;
  ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 1000));

  ui_init();

  while (1) {
    /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Try to take the semaphore, call lvgl related function on success */
    if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {
      lv_timer_handler();
      xSemaphoreGive(xGuiSemaphore);
    }
  }

  vTaskDelete(NULL);
}
