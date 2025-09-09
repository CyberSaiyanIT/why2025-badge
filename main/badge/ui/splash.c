#include "splash.h"

lv_obj_t *ui_screen_splash_init() {
  LV_IMG_DECLARE(img_logo);

  lv_obj_t *screen_logo = lv_obj_create(NULL);
  lv_obj_clear_flag(screen_logo, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *logo = lv_img_create(screen_logo);
  lv_img_set_src(logo, &img_logo);
  lv_obj_align(logo, LV_ALIGN_CENTER, 0, 0);

  /*Change the logo's background color*/
  static lv_style_t style;
  lv_style_init(&style);
  lv_style_set_bg_opa(&style, LV_OPA_COVER);
  lv_style_set_bg_color(&style, lv_color_hex(0x343a40));
  lv_obj_add_style(logo, &style, LV_PART_MAIN);

  return (screen_logo);
}