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
#include "splash.h"

screen_config_t screen_config[] = {
    {
        SCREEN_SPLASH,          // ID
        ui_splash_init,         // init
        ui_splash_load,         // load
        NULL,                   // unload
        NULL,                   // resume
        NULL,                   // pause
        ui_splash_button_up,    // button up
        ui_splash_button_down,  // button down
    },
    {
        SCREEN_PERSON,
        ui_person_init,
        ui_person_load,
        NULL,
        NULL,
        NULL,
        ui_person_button_up,
        ui_person_button_down,
    },
    {
        SCREEN_SOCIALENERGY,
        ui_socialenergy_init,
        NULL,
        NULL,
        NULL,
        NULL,
        ui_socialenergy_button_up,
        ui_socialenergy_button_down,
    },
    {
        SCREEN_DICE,
        ui_dice_init,
        NULL,
        NULL,
        NULL,
        NULL,
        ui_dice_button_up,
        ui_dice_button_down,
    },
    {
        SCREEN_EVENT,
        ui_event_init,
        NULL,
        NULL,
        NULL,
        NULL,
        ui_scroll_up,
        ui_scroll_down,
    },
    {
        SCREEN_RADAR,
        ui_radar_init,
        ui_radar_load,
        ui_radar_unload,
        ui_radar_load,
        ui_radar_unload,
        NULL,
        NULL,
    },
    {
        SCREEN_RSSI,
        ui_rssi_init,
        ui_rssi_load,
        ui_rssi_unload,
        ui_rssi_load,
        ui_rssi_unload,
        ui_scroll_up,
        ui_scroll_down,
    },
    {
        SCREEN_ADMIN,
        ui_screen_admin_init,
        NULL,
        NULL,
        NULL,
        NULL,
        ui_admin_button_up,
        ui_admin_button_down,
    },
    {
        SCREEN_SNAKE,
        ui_screen_snake_init,
        NULL,
        NULL,
        NULL,
        NULL,
        ui_snake_button_up,
        ui_snake_button_down,
    },
    {
        NUM_SCREENS,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
    },
};

inline uint8_t ui_config_get_nb_screens() { return NUM_SCREENS; }