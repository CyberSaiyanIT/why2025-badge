#ifndef _BUTTONS_H
#define _BUTTONS_H

#include <stdio.h>
#include <stdlib.h>

#include "lvgl.h"
#include "lvgl_helpers.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "button.h"
#include "driver/gpio.h"
#include "ui.h"

#define BUTTON_1 0x08 // DOWN button
#define BUTTON_2 0x09 // UP button

typedef struct {
  void (*before)();
  void (*button_up)();
  void (*button_down)();
  void (*after)();
} screen_buttons_t;

void button_task(void *arg);

void ui_button_up();
void ui_button_down();

#endif