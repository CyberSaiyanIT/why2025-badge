#include "ui.h"
#include "../led.h"

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

  lv_scr_load_anim(screens[current_screen], LV_SCR_LOAD_ANIM_OVER_TOP, 300, 0,
                   false);

  restore_current_timer();
}

void ui_switch_page_up() {
  ui_update_backlight(true);

  current_screen--;
  current_screen = (NUM_SCREENS + (current_screen % NUM_SCREENS)) % NUM_SCREENS;
  ESP_LOGI("DISPLAY", "DISPLAY COUNTER: %d/%d", current_screen + 1,
           NUM_SCREENS);

  lv_scr_load_anim(screens[current_screen], LV_SCR_LOAD_ANIM_OVER_BOTTOM, 300,
                   0, false);

  restore_current_timer();
}

static void ui_init(void) {
  current_screen = SCREEN_LOGO;

  screens[SCREEN_LOGO] = ui_screen_splash_init();
  screens[SCREEN_PERSON] = ui_screen_person_init();
  screens[SCREEN_SOCIALENERGY] = ui_screen_socialenergy_init();
  screens[SCREEN_EVENT] = ui_screen_event_init();
  screens[SCREEN_RADAR] = ui_screen_radar_init();
  screens[SCREEN_RSSI] = ui_screen_rssi_init();
  screens[SCREEN_ADMIN] = ui_screen_admin_init();
  screens[SCREEN_SNAKE] = ui_screen_snake_init();

  // show first page.
  lv_scr_load(screens[current_screen]);

  // Turn on backlight and run backlight management task
  ui_update_backlight(true);
  backlight_init();
}

static void ui_tick_task(void *arg) { lv_tick_inc(1); }

void ui_task(void *arg) {
  SemaphoreHandle_t xGuiSemaphore;
  xGuiSemaphore = xSemaphoreCreateMutex();

  ESP_LOGI(__FILE__, "LVGL INIT");
  lv_init();

  ESP_LOGI(__FILE__, "DRIVERS INIT");
  lvgl_driver_init();

  ESP_LOGI(__FILE__, "BUFFERS INIT");
  lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(
      DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
  lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(
      DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);

  static lv_disp_draw_buf_t disp_buf;
  uint32_t size_in_px = DISP_BUF_SIZE;

  lv_disp_draw_buf_init(&disp_buf, buf1, buf2, size_in_px);
  lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.flush_cb = disp_driver_flush;
  disp_drv.draw_buf = &disp_buf;
  lv_disp_drv_register(&disp_drv);


  ESP_LOGI(__FILE__, "LVGL READY");
  const esp_timer_create_args_t periodic_timer_args = {
      .callback = &ui_tick_task,
      .name = "ui_tick_task",
  };
  esp_timer_handle_t periodic_timer;
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

  free(buf1);
  free(buf2);
  vTaskDelete(NULL);
}
