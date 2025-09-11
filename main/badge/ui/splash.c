#include "splash.h"
#include "ui.h"
#include "esp_log.h"
#include "../led.h"

#define STD_LOGO   "A:/data/img/img_logo.bin"
#define MILLI_LOGO "A:/data/img/img_milli.bin"

static lv_obj_t *logo;
static bool milli_logo = false;

static uint8_t up_button_press_counter   = 0;
static uint8_t down_button_press_counter = 0;

static void ui_splash_single_click_event(lv_event_t *event) {
  // lv_event_code_t event_code = lv_event_get_code(event);
  //  ESP_LOGI(__FILE__, "EVENT ui_screen_splash_event : %s", lv_event_code_get_name(event_code));
  ui_switch_page_down();
}
static void ui_splash_triple_click_event(lv_event_t *event) {
  lv_image_set_src(logo, milli_logo ? MILLI_LOGO : STD_LOGO);
  milli_logo = !milli_logo;
}

lv_obj_t *ui_splash_init() {
  lv_obj_t *screen_logo = lv_obj_create(NULL);
  lv_obj_clear_flag(screen_logo, LV_OBJ_FLAG_SCROLLABLE);

  logo = lv_image_create(screen_logo);
  lv_image_set_src(logo, STD_LOGO);
  lv_obj_align(logo, LV_ALIGN_CENTER, 0, 0);

  /*Change the logo's background color*/
  static lv_style_t style;
  lv_style_init(&style);
  lv_style_set_bg_opa(&style, LV_OPA_COVER);
  lv_style_set_bg_color(&style, lv_color_hex(0x343a40));
  lv_obj_add_style(logo, &style, LV_PART_MAIN);

  lv_obj_add_flag(logo, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(logo, ui_splash_single_click_event, LV_EVENT_DOUBLE_CLICKED, NULL);
  lv_obj_add_event_cb(logo, ui_splash_triple_click_event, LV_EVENT_LONG_PRESSED, NULL);

  return (screen_logo);
}

void ui_splash_load() {
  up_button_press_counter   = 0;
  down_button_press_counter = 0;
};

void ui_splash_button_up() {
  // Increment counter for UP button presses
  up_button_press_counter++;
  ESP_LOGI("UI", "UP button press count: %d", up_button_press_counter);

  // Check if we've reached nb_screens presses
  if (up_button_press_counter == ui_config_get_nb_screens()) {
    // Call set_completed() function when on SCREEN_LOGO (index 0)
    ESP_LOGI("UI", "Summoning sequence activated!");
    led_set_completed();

    // Reset the counter after reaching nb_screens
    up_button_press_counter = 0;
  }
}
void ui_splash_button_down() {
  // Increment counter for DOWN button presses
  down_button_press_counter++;
  ESP_LOGI("UI", "DOWN button press count: %d", down_button_press_counter);

  // Check if we've reached nb_screens presses
  if (down_button_press_counter == ui_config_get_nb_screens()) {
    // Call led_rainbow() function when on SCREEN_LOGO (index 0)
    ESP_LOGI("UI", "Rainbow sequence activated!");
    led_rainbow();

    // Reset the counter after reaching nb_screens
    down_button_press_counter = 0;
  }
}