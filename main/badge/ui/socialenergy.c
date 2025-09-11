#include "socialenergy.h"

#define SOCIAL_MIN 0
#define SOCIAL_MAX 100

static int32_t social_value         = SOCIAL_MAX;
static lv_obj_t *needle_line        = NULL;
static lv_obj_t *social_value_label = NULL;
static lv_obj_t *scale              = NULL;

typedef struct {
  lv_style_t items;
  lv_style_t indicator;
  lv_style_t main;
} section_styles_t;

static section_styles_t zone1_styles;
static section_styles_t zone2_styles;
static section_styles_t zone3_styles;
static section_styles_t zone4_styles;
static section_styles_t zone5_styles;

static void init_section_styles(section_styles_t *styles, lv_color_t color) {
  lv_style_init(&styles->items);
  lv_style_set_line_color(&styles->items, color);
  lv_style_set_line_width(&styles->items, 0);

  lv_style_init(&styles->indicator);
  lv_style_set_line_color(&styles->indicator, color);
  lv_style_set_line_width(&styles->indicator, 0);

  lv_style_init(&styles->main);
  lv_style_set_arc_color(&styles->main, color);
  lv_style_set_arc_width(&styles->main, 20);
}

static void add_section(lv_obj_t *target_scale, int32_t from, int32_t to, const section_styles_t *styles) {
  lv_scale_section_t *sec = lv_scale_add_section(target_scale);
  lv_scale_set_section_range(target_scale, sec, from, to);
  lv_scale_set_section_style_items(target_scale, sec, &styles->items);
  lv_scale_set_section_style_indicator(target_scale, sec, &styles->indicator);
  lv_scale_set_section_style_main(target_scale, sec, &styles->main);
}
static lv_color_t get_hr_zone_color(int32_t hr) {
  if (hr < 20)
    return lv_palette_main(LV_PALETTE_RED); /* Zone 1 */
  else if (hr < 40)
    return lv_palette_main(LV_PALETTE_ORANGE); /* Zone 2 */
  else if (hr < 60)
    return lv_palette_main(LV_PALETTE_GREY); /* Zone 3 */
  else if (hr < 80)
    return lv_palette_main(LV_PALETTE_BLUE); /* Zone 4 */
  else
    return lv_palette_main(LV_PALETTE_GREEN); /* Zone 5 */
}
static void set_needle(int value) {
  social_value = value;
  /* Update needle */
  lv_scale_set_line_needle_value(scale, needle_line, -8, social_value);

  /* Update HR text */
  lv_label_set_text_fmt(social_value_label, "%d%%", (int)social_value);

  /* Update text color based on zone */
  lv_color_t zone_color = get_hr_zone_color(social_value);
  lv_obj_set_style_text_color(social_value_label, zone_color, 0);
}

static void needle_step(int step) {
  social_value += step;

  if (social_value >= SOCIAL_MAX)
    social_value = SOCIAL_MAX;
  else if (social_value <= SOCIAL_MIN)
    social_value = SOCIAL_MIN;
  set_needle(social_value);
}

static void add_coffee() { needle_step(10); }
static void add_beer() { needle_step(20); }

lv_obj_t *ui_screen_socialenergy_init() {
  lv_obj_t *screen_socialenergy = lv_obj_create(NULL);
  lv_obj_clear_flag(screen_socialenergy, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *add_button = lv_button_create(screen_socialenergy);
  lv_obj_set_size(add_button, 50, 50);
  lv_obj_align(add_button, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
  lv_obj_set_style_bg_color(add_button, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN);
  lv_obj_t *add_button_text = lv_label_create(add_button);
  lv_obj_center(add_button_text);
  lv_obj_set_style_text_font(add_button_text, &lv_font_montserrat_36, 0);
  lv_label_set_text(add_button_text, "+");
  lv_obj_center(add_button_text);
  lv_obj_add_event_cb(add_button, socialenergy_button_down, LV_EVENT_PRESSED, NULL);
  lv_obj_add_flag(add_button_text, LV_OBJ_FLAG_EVENT_BUBBLE);

  lv_obj_t *minus_button = lv_button_create(screen_socialenergy);
  lv_obj_set_size(minus_button, 50, 50);
  lv_obj_align(minus_button, LV_ALIGN_BOTTOM_LEFT, 10, -10);
  lv_obj_set_style_bg_color(minus_button, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
  lv_obj_t *minus_button_text = lv_label_create(minus_button);
  lv_obj_center(minus_button_text);
  lv_obj_set_style_text_font(minus_button_text, &lv_font_montserrat_36, 0);
  lv_label_set_text(minus_button_text, "-");
  lv_obj_center(minus_button_text);
  lv_obj_add_event_cb(minus_button, socialenergy_button_up, LV_EVENT_PRESSED, NULL);
  lv_obj_add_flag(minus_button_text, LV_OBJ_FLAG_EVENT_BUBBLE);

  lv_obj_t *coffee_button = lv_button_create(screen_socialenergy);
  lv_obj_set_size(coffee_button, 95, 50);
  lv_obj_align(coffee_button, LV_ALIGN_BOTTOM_MID, -50, -10);
  lv_obj_t *coffee_button_text = lv_label_create(coffee_button);
  lv_obj_center(coffee_button_text);
  lv_obj_set_style_text_font(coffee_button_text, &lv_font_montserrat_18, 0);
  lv_label_set_text(coffee_button_text, "Coffee++");
  lv_obj_center(coffee_button_text);
  lv_obj_add_event_cb(coffee_button, add_coffee, LV_EVENT_PRESSED, NULL);
  lv_obj_add_flag(coffee_button_text, LV_OBJ_FLAG_EVENT_BUBBLE);

  lv_obj_t *beer_button = lv_button_create(screen_socialenergy);
  lv_obj_set_size(beer_button, 95, 50);
  lv_obj_align(beer_button, LV_ALIGN_BOTTOM_MID, 50, -10);
  lv_obj_t *beer_button_text = lv_label_create(beer_button);
  lv_obj_center(beer_button_text);
  lv_obj_set_style_text_font(beer_button_text, &lv_font_montserrat_18, 0);
  lv_label_set_text(beer_button_text, "Beer++");
  lv_obj_center(beer_button_text);
  lv_obj_add_event_cb(beer_button, add_beer, LV_EVENT_PRESSED, NULL);
  lv_obj_add_flag(beer_button_text, LV_OBJ_FLAG_EVENT_BUBBLE);

  scale = lv_scale_create(screen_socialenergy);

  lv_obj_align(scale, LV_ALIGN_CENTER, 0, -20);
  lv_obj_set_size(scale, 180, 180);

  lv_scale_set_mode(scale, LV_SCALE_MODE_ROUND_INNER);
  lv_scale_set_range(scale, SOCIAL_MIN, SOCIAL_MAX);
  lv_scale_set_total_tick_count(scale, 20);
  lv_scale_set_major_tick_every(scale, 2);
  lv_scale_set_angle_range(scale, 280);
  lv_scale_set_rotation(scale, 130);
  lv_scale_set_label_show(scale, false);

  lv_obj_set_style_length(scale, 6, LV_PART_ITEMS);
  lv_obj_set_style_length(scale, 10, LV_PART_INDICATOR);
  lv_obj_set_style_arc_width(scale, 0, LV_PART_MAIN);

  /* Zone 1: (Grey) */
  init_section_styles(&zone1_styles, lv_palette_main(LV_PALETTE_RED));
  add_section(scale, 0, 20, &zone1_styles);

  /* Zone 2: (Blue) */
  init_section_styles(&zone2_styles, lv_palette_main(LV_PALETTE_ORANGE));
  add_section(scale, 20, 40, &zone2_styles);

  /* Zone 3: (Green) */
  init_section_styles(&zone3_styles, lv_palette_main(LV_PALETTE_GREY));
  add_section(scale, 40, 60, &zone3_styles);

  /* Zone 4: (Orange) */
  init_section_styles(&zone4_styles, lv_palette_main(LV_PALETTE_BLUE));
  add_section(scale, 60, 80, &zone4_styles);

  /* Zone 5: (Red) */
  init_section_styles(&zone5_styles, lv_palette_main(LV_PALETTE_GREEN));
  add_section(scale, 80, 100, &zone5_styles);

  needle_line = lv_line_create(scale);

  /* Optional styling */
  lv_obj_set_style_line_color(needle_line, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_line_width(needle_line, 12, LV_PART_MAIN);
  lv_obj_set_style_length(needle_line, 20, LV_PART_MAIN);
  lv_obj_set_style_line_rounded(needle_line, false, LV_PART_MAIN);
  lv_obj_set_style_pad_right(needle_line, 50, LV_PART_MAIN);

  lv_obj_t *circle = lv_obj_create(screen_socialenergy);
  lv_obj_set_size(circle, 120, 120);
  lv_obj_align(circle, LV_ALIGN_CENTER, 0, -20);

  lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);

  lv_obj_set_style_bg_color(circle, lv_obj_get_style_bg_color(lv_scr_act(), LV_PART_MAIN), 0);
  lv_obj_set_style_bg_opa(circle, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(circle, 0, LV_PART_MAIN);

  lv_obj_t *hr_container = lv_obj_create(circle);
  lv_obj_center(hr_container);
  lv_obj_set_size(hr_container, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(hr_container, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(hr_container, 0, 0);
  lv_obj_set_layout(hr_container, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(hr_container, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(hr_container, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(hr_container, 0, 0);
  lv_obj_set_flex_align(hr_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  social_value_label = lv_label_create(hr_container);
  lv_obj_clear_flag(social_value_label, LV_OBJ_FLAG_SCROLLABLE);
  lv_label_set_text_fmt(social_value_label, "%d%%", SOCIAL_MAX / 2);
  lv_obj_set_style_text_font(social_value_label, &lv_font_montserrat_36, 0);
  lv_obj_set_style_text_align(social_value_label, LV_TEXT_ALIGN_CENTER, 0);

  lv_color_t zone_color = get_hr_zone_color(social_value);
  lv_obj_set_style_text_color(social_value_label, zone_color, 0);

  set_needle(SOCIAL_MAX / 2);
  return (screen_socialenergy);
}

void socialenergy_button_up() {
  needle_step(-5);
}
void socialenergy_button_down() {
  needle_step(5);
}