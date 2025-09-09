#include "buttons.h"

const int nb_screens = NUM_SCREENS + 1;
static uint8_t up_button_press_counter = 0;
static uint8_t down_button_press_counter = 0;

static int8_t counter_screen = -1; // Initialize to invalid screen index

static screen_buttons_t button_action[NUM_SCREENS] = {
    {NULL, NULL, NULL, NULL},                           // LOGO
    {NULL, person_button_up, person_button_down, NULL}, // PERSON
    {NULL, socialenergy_button_up, socialenergy_button_down,
     NULL},                                           // SOCIALENERGY
    {NULL, scroll_up, scroll_down, NULL},             // EVENT
    {NULL, NULL, NULL, NULL},                         // RADAR
    {NULL, scroll_up, scroll_down, NULL},             // RSSI
    {NULL, admin_button_up, admin_button_down, NULL}, // ADMIN
    {NULL, snake_button_up, snake_button_down, NULL}, // SNAKE
};

void check_counter() {
  // Check if this is the first button press or if we've changed screens
  if (counter_screen != current_screen) {
    // Reset counters when screen changes
    up_button_press_counter = 0;
    down_button_press_counter = 0;
    // Update counter_screen to current screen
    counter_screen = current_screen;
    // printf("DEBUG: Started counting UP presses on screen index: %d\n",
    // current_screen);
  }
}

void ui_button_up() {
  check_counter();
  // Increment counter for UP button presses
  up_button_press_counter++;
  ESP_LOGI("UI", "UP button press count: %d on screen index: %d",
           up_button_press_counter, current_screen);

  // Check if we've reached nb_screens presses
  if (up_button_press_counter == nb_screens) {
    // Call set_completed() function when on SCREEN_LOGO (index 0)
    if (current_screen == SCREEN_LOGO) {
      ESP_LOGI("UI", "Summoning sequence activated!");
      set_completed();
    }

    // Reset the counter after reaching nb_screens
    up_button_press_counter = 0;
  }

  // Check if the backlight update is needed
  if (ui_update_backlight(true))
    return;

  if (button_action[current_screen].before != NULL)
    button_action[current_screen].before();
  if (button_action[current_screen].button_up != NULL)
    button_action[current_screen].button_up();
  else
    ESP_LOGI(__FILE__, "Button up, no actions");
  if (button_action[current_screen].before != NULL)
    button_action[current_screen].after();
}

void ui_button_down() {
  check_counter();
  // Increment counter for DOWN button presses
  down_button_press_counter++;
  ESP_LOGI("UI", "DOWN button press count: %d on screen index: %d",
           down_button_press_counter, current_screen);

  // Check if we've reached nb_screens presses
  if (down_button_press_counter == nb_screens) {
    // Call rainbow() function when on SCREEN_LOGO (index 0)
    if (current_screen == SCREEN_LOGO) {
      ESP_LOGI("UI", "Rainbow sequence activated!");
      rainbow();
    }

    // Reset the counter after reaching nb_screens
    down_button_press_counter = 0;
  }

  if (ui_update_backlight(true))
    return;
  if (button_action[current_screen].before != NULL)
    button_action[current_screen].before();
  if (button_action[current_screen].button_down != NULL)
    button_action[current_screen].button_down();
  else
    ESP_LOGI(__FILE__, "Button down, no actions");
  if (button_action[current_screen].before != NULL)
    button_action[current_screen].after();
}

void button_task(void *arg) {
  static button_event_t curr_ev;
  static button_event_t prev_ev[2];
  static QueueHandle_t button_events;
  button_events = button_init(PIN_BIT(BUTTON_1) | PIN_BIT(BUTTON_2));

  while (true) {
    if (xQueueReceive(button_events, &curr_ev, 1000 / portTICK_PERIOD_MS)) {
      uint8_t btn_id = curr_ev.pin - 0x08;
      if (curr_ev.event == BUTTON_HELD) {
        set_screen_led_backlight(badge_obj.brightness_mid);
      }
      if (curr_ev.pin == BUTTON_1) // DOWN button event
      {
        if ((prev_ev[btn_id].event == BUTTON_HELD) &&
            (curr_ev.event == BUTTON_UP)) {
          ui_switch_page_down();
        } else if ((prev_ev[btn_id].event == BUTTON_DOWN) &&
                   (curr_ev.event == BUTTON_UP)) {
          ui_button_down();
        }
      }

      if (curr_ev.pin == BUTTON_2) // UP button event
      {
        if ((prev_ev[btn_id].event == BUTTON_HELD) &&
            (curr_ev.event == BUTTON_UP)) {
          ui_switch_page_up();
        } else if ((prev_ev[btn_id].event == BUTTON_DOWN) &&
                   (curr_ev.event == BUTTON_UP)) {
          ui_button_up();
        }
      }
      prev_ev[btn_id] = curr_ev;
    }
  }
}
