#include "socialenergy.h"

static lv_obj_t *socialenergy_meter;


lv_obj_t *ui_screen_socialenergy_init() {
  lv_obj_t *screen_socialenergy = lv_obj_create(NULL);
  lv_obj_clear_flag(screen_socialenergy, LV_OBJ_FLAG_SCROLLABLE);

  socialenergy_meter = lv_meter_create(screen_socialenergy);
  lv_obj_set_align(socialenergy_meter, LV_ALIGN_TOP_MID);
  lv_obj_set_pos(socialenergy_meter, 0, 10);
  lv_obj_set_size(socialenergy_meter, 180, 180);

  /*Add a scale first*/
  lv_meter_scale_t *scale = lv_meter_add_scale(socialenergy_meter);
  lv_meter_set_scale_ticks(socialenergy_meter, scale, 41, 2, 10,
                           lv_palette_main(LV_PALETTE_GREY));
  lv_meter_set_scale_major_ticks(socialenergy_meter, scale, 8, 4, 15,
                                 lv_color_black(), 10);

  lv_meter_indicator_t *indic;

  /*Add a red arc to the start*/
  indic = lv_meter_add_arc(socialenergy_meter, scale, 3,
                           lv_palette_main(LV_PALETTE_RED), 0);
  lv_meter_set_indicator_start_value(socialenergy_meter, indic, 0);
  lv_meter_set_indicator_end_value(socialenergy_meter, indic, 20);

  /*Make the tick lines blue at the start of the scale*/
  indic = lv_meter_add_scale_lines(socialenergy_meter, scale,
                                   lv_palette_main(LV_PALETTE_RED),
                                   lv_palette_main(LV_PALETTE_RED), false, 0);
  lv_meter_set_indicator_start_value(socialenergy_meter, indic, 0);
  lv_meter_set_indicator_end_value(socialenergy_meter, indic, 20);

  /*Add a green arc to the end*/
  indic = lv_meter_add_arc(socialenergy_meter, scale, 3,
                           lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_meter_set_indicator_start_value(socialenergy_meter, indic, 80);
  lv_meter_set_indicator_end_value(socialenergy_meter, indic, 100);

  /*Make the tick lines red at the end of the scale*/
  indic = lv_meter_add_scale_lines(socialenergy_meter, scale,
                                   lv_palette_main(LV_PALETTE_GREEN),
                                   lv_palette_main(LV_PALETTE_GREEN), false, 0);
  lv_meter_set_indicator_start_value(socialenergy_meter, indic, 80);
  lv_meter_set_indicator_end_value(socialenergy_meter, indic, 100);

  /*Add a needle line indicator*/
  indic = lv_meter_add_needle_line(socialenergy_meter, scale, 4,
                                   lv_palette_main(LV_PALETTE_GREY), -10);

  /*Set the values*/
  lv_meter_set_indicator_value(socialenergy_meter, indic, 100);

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