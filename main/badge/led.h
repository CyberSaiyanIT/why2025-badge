#ifndef _LED_H
#define _LED_H

#include <stdint.h>
#include <stdio.h>

#include "driver/i2c.h"
#include "common/led_strip.h"
#include "rgb.h"
#include "badge.h"

#define I2C_MASTER_SCL_IO           0
#define I2C_MASTER_SDA_IO           1
#define I2C_MASTER_NUM              0
#define I2C_MASTER_FREQ_HZ          400000
#define I2C_MASTER_TIMEOUT_MS       1000

#define MAGENTA_SAIYAN              0xE80B60

#define NUM_LEDS	                7
#define LED_RMT_TX_CHANNEL          RMT_CHANNEL_0
#define LED_RMT_TX_GPIO             GPIO_NUM_5
// ****************************************************

enum LED_COLOR {
    RED, GREEN, BLUE,
};

void led_init();

void set_screen_led_backlight(uint8_t);

void led_task(void* arg);

void set_completed(void);

void rainbow(void);

void flash(int period, uint8_t fade_factor);

void set_easter_egg_active(bool active);

// Continuous, non-blocking rainbow LED animations, driven from led_task's own
// loop (not LVGL) so they never stall the UI. Toggle on/off from the rainbow
// screen's enter/leave handling in ui.c; cycle through the 5 animations
// (short-press either wheel on that screen) with rainbow_next_animation(), and
// read the active one's display name with rainbow_current_animation_name().
void set_rainbow_loop_active(bool active);
void rainbow_next_animation(void);
const char* rainbow_current_animation_name(void);

#endif // _LED_H
