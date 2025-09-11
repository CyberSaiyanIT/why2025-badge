#include "config.h"

#include "backlight.h"
#include "splash.h"
#include "person.h"
#include "dice.h"
#include "event.h"
#include "radar.h"
#include "rssi.h"
#include "socialenergy.h"
#include "admin.h"
#include "snake.h"

screen_config_t screen_config[] = {
    {SCREEN_LOGO, ui_screen_splash_init, NULL, NULL, NULL, NULL},
    {SCREEN_PERSON, ui_screen_person_init, NULL, NULL, person_button_up, person_button_down},
    {SCREEN_SOCIALENERGY, ui_screen_socialenergy_init, NULL, NULL, socialenergy_button_up, socialenergy_button_down},
    {SCREEN_DICE, ui_screen_dice_init, NULL, NULL, dice_button_up, dice_button_down},
    {SCREEN_EVENT, ui_screen_event_init, NULL, NULL, scroll_up, scroll_down},
    {SCREEN_RADAR, ui_screen_radar_init, NULL, NULL, NULL, NULL},
    {SCREEN_RSSI, ui_screen_rssi_init, NULL, NULL, scroll_up, scroll_down},
    {SCREEN_ADMIN, ui_screen_admin_init, NULL, NULL, admin_button_up, admin_button_down},
    {SCREEN_SNAKE, ui_screen_snake_init, NULL, NULL, snake_button_up, snake_button_down},
    {NUM_SCREENS, NULL, NULL, NULL},
};
