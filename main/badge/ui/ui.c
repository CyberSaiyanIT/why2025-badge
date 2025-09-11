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

static lv_obj_t *screens[NUM_SCREENS];
static int8_t current_screen;

int8_t ui_get_current_screen() { return current_screen; }

void ui_resume_current_screen() {
  if (screen_config[current_screen].resume_screen)
    screen_config[current_screen].resume_screen();
}

void ui_pause_current_screen() {
  if (screen_config[current_screen].pause_screen)
    screen_config[current_screen].pause_screen();
}

static void ui_scroll(int dy) {
  lv_obj_t *object = lv_obj_get_child(screens[current_screen], 0);
  lv_obj_scroll_by(object, 0, dy, LV_ANIM_ON);
}

void ui_scroll_up() { ui_scroll(SCROLL_UP); }
void ui_scroll_down() { ui_scroll(SCROLL_DOWN); }

void ui_load_current_screen() {
  if (screen_config[current_screen].load_screen)
    screen_config[current_screen].load_screen();
}

void ui_unload_current_screen() {
  if (screen_config[current_screen].unload_screen)
    screen_config[current_screen].unload_screen();
}

static void ui_switch_page(lv_screen_load_anim_t anim_type) {
  ui_backlight_update(true);
  ESP_LOGI(__FILE__, "DISPLAY COUNTER: %d/%d", current_screen + 1, NUM_SCREENS);
  lv_screen_load_anim(screens[current_screen], anim_type, 100, 0, false);
  ui_load_current_screen();
}

void ui_switch_page_down() {
  ui_unload_current_screen();
  current_screen = current_screen + 1 >= NUM_SCREENS ? 0 : current_screen + 1;
  ui_switch_page(LV_SCR_LOAD_ANIM_OVER_BOTTOM);
}

void ui_switch_page_up() {
  ui_unload_current_screen();
  current_screen = current_screen - 1 < 0 ? NUM_SCREENS - 1 : current_screen - 1;
  ui_switch_page(LV_SCR_LOAD_ANIM_OVER_TOP);
}
#if LV_USE_LOG
void log_to_serial(lv_log_level_t level, const char *buf) {
  switch (level) {
    case LV_LOG_LEVEL_TRACE:
      ESP_LOGV("LVGL", "%s", buf);
      break;
    case LV_LOG_LEVEL_INFO:
      ESP_LOGI("LVGL", "%s", buf);
      break;
    case LV_LOG_LEVEL_WARN:
      ESP_LOGW("LVGL", "%s", buf);
      break;
    case LV_LOG_LEVEL_ERROR:
      ESP_LOGE("LVGL", "%s", buf);
      break;
    case LV_LOG_LEVEL_USER:
      ESP_LOGI("LVGL User", "%s", buf);
      break;
    case LV_LOG_LEVEL_NONE:
      ESP_LOGI("LVGL None", "%s", buf);
      break;
  }
}
#endif

static void ui_init(void) {
  current_screen = FIRST_SCREEN;
  for (uint8_t i = 0; i < NUM_SCREENS; i++)
    screens[i] = screen_config[i].screen_init();
  lv_screen_load(screens[current_screen]);
}

/*****  TASK *****/

static void ui_tick_task(void *arg) { lv_tick_inc(1); }

void ui_task(void *arg) {
  ESP_LOGI(__FILE__, "Starting UI task");

  ui_lcd_init();
#if LV_USE_LOG
  lv_log_register_print_cb(log_to_serial);
#endif
  ui_backlight_update(true);
  ui_backlight_init();
  buttons_init();
  ui_touchscreen_init();

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
