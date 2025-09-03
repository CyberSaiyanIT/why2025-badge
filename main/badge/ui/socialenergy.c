#include "socialenergy.h"

static lv_obj_t *socialenergy_meter;


lv_obj_t *ui_screen_socialenergy_init() {
  lv_obj_t *screen_socialenergy = lv_obj_create(NULL);
  lv_obj_clear_flag(screen_socialenergy, LV_OBJ_FLAG_SCROLLABLE);

  socialenergy_meter = lv_scale_create(screen_socialenergy);
  lv_obj_set_align(socialenergy_meter, LV_ALIGN_TOP_MID);
  lv_obj_set_pos(socialenergy_meter, 0, 10);
  lv_obj_set_size(socialenergy_meter, 180, 180);

  // TODO  

  /* Label */
  static lv_style_t style_label;
  lv_style_init(&style_label);
  lv_style_set_text_font(&style_label, &lv_font_montserrat_22);

  lv_obj_t *label = lv_label_create(screen_socialenergy);
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP); /*Break the long lines*/
  lv_label_set_recolor(label,
                       true); /*Enable re-coloring by commands in the text*/
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER,
                              0); /*Center aligned lines*/
  lv_obj_set_width(label, NAME_LABEL_SIZE);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 90);
  lv_obj_add_style(label, &style_label, LV_PART_MAIN);

  lv_label_set_text(label, "Social Energy");

  return(screen_socialenergy);
}


void socialenergy_button_up() {
  // TODO
  /*needle_value = lv_meter_get_value(socialenergy_meter, 0) - 5;
  needle_value = needle_value < 0 ? 0 : needle_value; // Low limit to 0
  lv_meter_set_value(socialenergy_meter, 0, needle_value);*/
}
void socialenergy_button_down() {
  // TODO
  /* needle_value = lv_meter_get_value(socialenergy_meter, 0) + 5;
  needle_value = needle_value > 100 ? 100 : needle_value; // TOP limit to 100
  lv_meter_set_value(socialenergy_meter, 0, needle_value); */
}