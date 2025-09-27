#ifndef _UI_H
#define _UI_H

#include <stdio.h>
#include <stdlib.h>

#include <lvgl.h>

#include "config.h"

#define NAME_LABEL_SIZE 300
#define SCROLL_UP 80
#define SCROLL_DOWN -80

int8_t ui_get_current_screen();
void ui_resume_current_screen();
void ui_pause_current_screen();
void ui_scroll_up();
void ui_scroll_down();
void ui_load_current_screen();
void ui_unload_current_screen();
void ui_switch_page_up();
void ui_switch_page_down();
void ui_task(void *);

#endif