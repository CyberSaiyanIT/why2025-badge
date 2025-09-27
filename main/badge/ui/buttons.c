#include "buttons.h"

#include "backlight.h"
#include "config.h"
#include "esp_log.h"
#include "../badge.h"

static QueueHandle_t button_events;
static lv_timer_t *buttons_timer_handle;

void ui_button_up() {
  int8_t current_screen = ui_get_current_screen();

  if (ui_backlight_update(true))  // Check if the backlight update is needed
    return;

  if (screen_config[current_screen].button_up != NULL)
    screen_config[current_screen].button_up();
  else
    ESP_LOGI(__FILE__, "Button up, no actions");
}

void ui_button_down() {
  int8_t current_screen = ui_get_current_screen();
  if (ui_backlight_update(true))  // Check if the backlight update is needed
    return;
  if (screen_config[current_screen].button_down != NULL)
    screen_config[current_screen].button_down();
  else
    ESP_LOGI(__FILE__, "Button down, no actions");
}

static void ui_button_timer_handler(lv_timer_t *arg) {
  static button_event_t curr_ev;
  static button_event_t prev_ev[2];

  if (xQueueReceive(button_events, &curr_ev, 0)) {
    uint8_t btn_id = curr_ev.pin - BUTTON_1;
    if (curr_ev.event == BUTTON_HELD)
      ui_backlight_set_mid();

    if (curr_ev.pin == BUTTON_1)  // DOWN button event
    {
      if ((prev_ev[btn_id].event == BUTTON_HELD) && (curr_ev.event == BUTTON_UP)) {
        ui_switch_page_down();
      } else if ((prev_ev[btn_id].event == BUTTON_DOWN) && (curr_ev.event == BUTTON_UP)) {
        ui_button_down();
      }
    }

    if (curr_ev.pin == BUTTON_2)  // UP button event
    {
      if ((prev_ev[btn_id].event == BUTTON_HELD) && (curr_ev.event == BUTTON_UP)) {
        ui_switch_page_up();
      } else if ((prev_ev[btn_id].event == BUTTON_DOWN) && (curr_ev.event == BUTTON_UP)) {
        ui_button_up();
      }
    }
    prev_ev[btn_id] = curr_ev;
  }
}

void buttons_init() {
  ESP_LOGI(__FILE__, "Starting button init");
  button_events        = button_init(PIN_BIT(BUTTON_1) | PIN_BIT(BUTTON_2));
  buttons_timer_handle = lv_timer_create(ui_button_timer_handler, 100, NULL);
  lv_timer_resume(buttons_timer_handle);
}