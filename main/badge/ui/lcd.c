#include "lcd.h"
#include <lvgl.h>
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

uint8_t *buf1 = NULL;
uint8_t *buf2 = NULL;
/***** LCD INIT *****/


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
  
  // because SPI LCD is big-endian, we need to swap the RGB bytes order
  lv_draw_sw_rgb565_swap(px_map,
                         (offsetx2 + 1 - offsetx1) * (offsety2 + 1 - offsety1));
  // copy a buffer's content to a specific area of the display
  ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(
      (esp_lcd_panel_handle_t)lv_display_get_user_data(disp),
      offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map));
}

void spi_init() {
  ESP_LOGI(__FILE__, "Initialize SPI bus");
  spi_bus_config_t buscfg = {
      .sclk_io_num     = PIN_NUM_SCLK,
      .mosi_io_num     = PIN_NUM_MOSI,
      .miso_io_num     = PIN_NUM_MISO,
      .quadwp_io_num   = -1,
      .quadhd_io_num   = -1,
      .max_transfer_sz = LCD_H_RES * LVGL_DRAW_BUF_LINES * sizeof(uint16_t),
  };
  ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));
}

void panel_init() {
  ESP_LOGI(__FILE__, "Initialize panel");
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
}

void lcd_init() {
  spi_init();
  panel_init();

  ESP_LOGI(__FILE__, "LVGL Initialization");
  lv_init();
#if LV_USE_LOG
  lv_log_register_print_cb(log_to_serial);
#endif
  // create a lvgl display
  ESP_LOGI(__FILE__, "Display Initialization");
  lv_display_t *display = lv_display_create(LCD_H_RES, LCD_V_RES);

  ESP_LOGI(__FILE__, "Buffers Initialization");
  buf1 = heap_caps_malloc(LVGL_BUF_SIZE, MALLOC_CAP_DMA);
  buf2 = heap_caps_malloc(LVGL_BUF_SIZE, MALLOC_CAP_DMA);
  lv_display_set_buffers(display, buf1, buf2, LVGL_BUF_SIZE * sizeof(uint8_t),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);
  // associate the mipi panel handle to the display
  lv_display_set_user_data(display, panel_handle);
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
}
