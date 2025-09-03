#include "ui.h"
#include "../led.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sys/param.h>
#include <unistd.h>

static esp_lcd_panel_handle_t panel_handle;
static esp_lcd_panel_io_handle_t io_handle;

lv_obj_t *screens[NUM_SCREENS];
int8_t current_screen;

uint8_t buf1[LCD_H_RES * LCD_V_RES / 10 * BYTES_PER_PIXEL];
uint8_t buf2[LCD_H_RES * LCD_V_RES / 10 * BYTES_PER_PIXEL];

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

/***** LCD INIT *****/
static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                    esp_lcd_panel_io_event_data_t *edata,
                                    void *user_ctx) {
  lv_display_t *disp = (lv_display_t *)user_ctx;
  lv_display_flush_ready(disp);
  return false;
}

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area,
                          uint8_t *px_map) {
  int offsetx1 = area->x1;
  int offsetx2 = area->x2;
  int offsety1 = area->y1;
  int offsety2 = area->y2;

  ESP_LOGI(__FILE__, "LVGL_FLUSH %d %d - %d %d", offsetx1, offsety1, offsetx2, offsety2);

  // because SPI LCD is big-endian, we need to swap the RGB bytes order
  lv_draw_sw_rgb565_swap(px_map,
                         (offsetx2 + 1 - offsetx1) * (offsety2 + 1 - offsety1));
  // copy a buffer's content to a specific area of the display
  ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(
      (esp_lcd_panel_handle_t)lv_display_get_user_data(disp),
      offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map));
}

void panel_init() {
  ESP_LOGI(__FILE__, "Initialize SPI bus");
  spi_bus_config_t buscfg = {
      .sclk_io_num     = PIN_NUM_SCLK,
      .mosi_io_num     = PIN_NUM_MOSI,
      .miso_io_num     = PIN_NUM_MISO,
      .quadwp_io_num   = -1,
      .quadhd_io_num   = -1,
      .max_transfer_sz = LCD_H_RES * 80 * sizeof(uint16_t),
  };
  ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

  ESP_LOGI(__FILE__, "Install panel IO");
  esp_lcd_panel_io_spi_config_t io_config = {
      .dc_gpio_num       = PIN_NUM_LCD_DC,
      .cs_gpio_num       = PIN_NUM_LCD_CS,
      .pclk_hz           = LCD_PIXEL_CLOCK_HZ,
      .lcd_cmd_bits      = LCD_CMD_BITS,
      .lcd_param_bits    = LCD_PARAM_BITS,
      .spi_mode          = 0,
      .trans_queue_depth = 10,
  };
  // Attach the LCD to the SPI bus
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST,
                                           &io_config, &io_handle));

  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = PIN_NUM_LCD_RST,
      .color_space    = LCD_RGB_ENDIAN_RGB,
      .bits_per_pixel = 16,
  };
  ESP_LOGI(__FILE__, "Install ST7789 panel driver");
  ESP_ERROR_CHECK(
      esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_disp_sleep(panel_handle, false));
  ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, true));
  ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));

  // user can flush pre-defined pattern to the screen before we turn on the
  // screen or backlight
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
  ESP_LOGI(__FILE__, "Panel ready");
}

/***** LV LOG *****/
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

/*****  TASK *****/

static void ui_tick_task(void *arg) { lv_tick_inc(1); }

void ui_task(void *arg) {
  ESP_LOGI(__FILE__, "Starting UI task");
  SemaphoreHandle_t xGuiSemaphore;
  xGuiSemaphore = xSemaphoreCreateMutex();

  panel_init();
  ESP_LOGI(__FILE__, "LVGL Initialization");
  lv_init();
  lv_log_register_print_cb(log_to_serial);

  // create a lvgl display
  ESP_LOGI(__FILE__, "Display Initialization");
  lv_display_t *display = lv_display_create(LCD_H_RES, LCD_V_RES);

  // alloc draw buffers used by LVGL
  // it's recommended to choose the size of the draw buffer(s) to be at least
  // 1/10 screen sized
  ESP_LOGI(__FILE__, "Buffers Initialization");

  // initialize LVGL draw buffers

  lv_display_set_buffers(display, buf1, buf2, sizeof(buf1),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);
  ESP_LOGI(__FILE__, "Associate the mipi panel ");
  // associate the mipi panel handle to the display
  lv_display_set_user_data(display, panel_handle);
  // set color depth
  ESP_LOGI(__FILE__, "Setting color depth");
  lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
  // set the callback which can copy the rendered image to an area of the
  // display
  ESP_LOGI(__FILE__, "Setting flush callback");
  lv_display_set_flush_cb(display, lvgl_flush_cb);

  ESP_LOGI(
      __FILE__,
      "Register io panel event callback for LVGL flush ready notification");
  const esp_lcd_panel_io_callbacks_t cbs = {
      .on_color_trans_done = notify_lvgl_flush_ready,
  };
  /* Register done callback */
  ESP_ERROR_CHECK(
      esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, display));

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
