#ifndef _BUTTONS_H
#define _BUTTONS_H

#include <stdio.h>
#include <stdlib.h>

#include <lvgl.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "button.h"
#include "driver/gpio.h"
#include "ui.h"

#define BUTTON_1 0x08 // DOWN button
#define BUTTON_2 0x09 // UP button

void buttons_init();

void ui_button_up();
void ui_button_down();

#endif