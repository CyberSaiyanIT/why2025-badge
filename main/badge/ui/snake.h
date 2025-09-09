#ifndef _SNAKE_H
#define _SNAKE_H

#include <stdio.h>
#include <stdlib.h>

#include "ui.h"

#define UI_SNAKE_MAX_BODY 64
#define UI_SNAKE_BODY_SIZE 20

#define SNAKE_MIN_SPEED 8
#define SNAKE_STEP_SPEED 3

enum SNAKE_DIR
{
    SNAKE_DIR_BEGIN = 10,
    SNAKE_DIR_UP = SNAKE_DIR_BEGIN,
    SNAKE_DIR_LEFT,
    SNAKE_DIR_DOWN,
    SNAKE_DIR_RIGHT,
    SNAKE_DIR_END = SNAKE_DIR_RIGHT,
};

typedef struct 
{
    lv_obj_t *body[UI_SNAKE_MAX_BODY];
    lv_obj_t *eye; // the 'eye' on snake to locate it direction.
    lv_obj_t *food;
    enum SNAKE_DIR dir;
    int size;
    int speed;
} snake_t;

lv_timer_t* snake_timer_handle;
lv_obj_t *screen_snake; 

lv_obj_t *ui_screen_snake_init();
    
void snake_set_dir(int8_t dir);
void snake_reset(lv_obj_t *parent);
void snake_timer(lv_timer_t *arg);

void snake_button_up();
void snake_button_down();
#endif