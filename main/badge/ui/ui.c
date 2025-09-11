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

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "../badge.h"
#include "admin.h"
#include "event.h"
#include "person.h"
#include "dice.h"
#include "radar.h"
#include "rssi.h"
#include "snake.h"
#include "socialenergy.h"
#include "splash.h"

#include "backlight.h"
#include "buttons.h"

#include "touchscreen.h"

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

static void scroll(int dy) {
  lv_obj_t *object = lv_obj_get_child(screens[current_screen], 0);
  lv_obj_scroll_by(object, 0, dy, LV_ANIM_ON);
}

void scroll_up() { scroll(SCROLL_UP); }
void scroll_down() { scroll(SCROLL_DOWN); }

static void ui_switch_page(lv_screen_load_anim_t anim_type) {
  ui_update_backlight(true);
  ESP_LOGI(__FILE__, "DISPLAY COUNTER: %d/%d", current_screen + 1, NUM_SCREENS);
  lv_screen_load_anim(screens[current_screen], anim_type, 100, 0, false);
  restore_current_timer();
}

void ui_switch_page_down() {
  current_screen = current_screen + 1 >= NUM_SCREENS ? 0 : current_screen + 1;
  ui_switch_page(LV_SCR_LOAD_ANIM_OVER_BOTTOM);
}

void ui_switch_page_up() {
  current_screen = current_screen - 1 < 0 ? NUM_SCREENS - 1 : current_screen - 1;
  ui_switch_page(LV_SCR_LOAD_ANIM_OVER_TOP);
}

static void ui_init(void) {
  current_screen = SCREEN_LOGO;
  for (uint8_t i=0; i < NUM_SCREENS; i++)
    screens[i] = screen_config[i].screen_init();
  lv_screen_load(screens[current_screen]);
}

/*****  TASK *****/

static void ui_tick_task(void *arg) { lv_tick_inc(1); }

void ui_task(void *arg) {
  ESP_LOGI(__FILE__, "Starting UI task");

  lcd_init();
  ui_update_backlight(true);
  backlight_init();
  buttons_init();
  touchscreen_init();

  const esp_timer_create_args_t periodic_timer_args = {
      .callback = &ui_tick_task,
      .name     = "ui_tick_task",
  };
  esp_timer_handle_t periodic_timer = NULL;
  ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 1000));

  ui_init();

  ESP_LOGI(__FILE__, "Starting main ui loop");
  while (1) {
    /* Delay 1 tick (assumes FreeRTOS tick is 10ms) */
    vTaskDelay(pdMS_TO_TICKS(10));
    lv_timer_handler();
  }

  vTaskDelete(NULL);
}
