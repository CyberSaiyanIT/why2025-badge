#include "splash.h"
#include "ui.h"
#include "esp_log.h"

#define STD_LOGO   "A:/data/img/img_logo.bin"
#define MILLI_LOGO "A:/data/img/img_milli.bin"

static lv_obj_t *logo;
static bool milli_logo = false;

static void ui_screen_splash_single_click_event(lv_event_t *event) {
  // lv_event_code_t event_code = lv_event_get_code(event);
  //  ESP_LOGI(__FILE__, "EVENT ui_screen_splash_event : %s", lv_event_code_get_name(event_code));
  ui_switch_page_down();
}
static void ui_screen_splash_triple_click_event(lv_event_t *event) {
  lv_image_set_src(logo, milli_logo ? MILLI_LOGO : STD_LOGO);
  milli_logo = !milli_logo;
}

lv_obj_t *ui_screen_splash_init() {
  lv_obj_t *screen_logo = lv_obj_create(NULL);
  lv_obj_clear_flag(screen_logo, LV_OBJ_FLAG_SCROLLABLE);

  logo = lv_image_create(screen_logo);
  //lv_image_set_src(logo, STD_LOGO);
  lv_obj_align(logo, LV_ALIGN_CENTER, 0, 0);

  /*Change the logo's background color*/
  static lv_style_t style;
  lv_style_init(&style);
  lv_style_set_bg_opa(&style, LV_OPA_COVER);
  lv_style_set_bg_color(&style, lv_color_hex(0x343a40));
  lv_obj_add_style(logo, &style, LV_PART_MAIN);

  lv_obj_add_flag(logo, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(logo, ui_screen_splash_single_click_event, LV_EVENT_DOUBLE_CLICKED, NULL);
  lv_obj_add_event_cb(logo, ui_screen_splash_triple_click_event, LV_EVENT_LONG_PRESSED, NULL);

  return (screen_logo);
}
