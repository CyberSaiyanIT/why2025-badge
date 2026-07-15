#include "snake.h"

const char ui_snake_eyes[][4] = {"' '", ": ", ". .", " :"};

static snake_t snake;

// ---- high score, persisted to a SPIFFS file (like Space Invaders) ----
static uint32_t snake_load_high(void)
{
    uint32_t v = 0;
    FILE *fp = fopen(SNAKE_HIGHSCORE_FILE, "r");
    if (fp) {
        if (fscanf(fp, "%u", &v) != 1) v = 0;
        fclose(fp);
    }
    return v;
}

static void snake_save_high(uint32_t v)
{
    FILE *fp = fopen(SNAKE_HIGHSCORE_FILE, "w");
    if (fp) {
        fprintf(fp, "%u", (unsigned)v);
        fclose(fp);
    }
}

// keep the HUD labels drawn above the snake body/food
static void snake_labels_front(void)
{
    if (snake.lbl_score) lv_obj_move_foreground(snake.lbl_score);
    if (snake.lbl_high)  lv_obj_move_foreground(snake.lbl_high);
}

static void snake_update_labels(void)
{
    lv_label_set_text_fmt(snake.lbl_score, "SCORE %d", snake.score);
    lv_label_set_text_fmt(snake.lbl_high,  "HIGH %u", (unsigned)snake.high);
    // re-anchor top-right so a growing number doesn't clip off the edge
    lv_obj_align(snake.lbl_high, NULL, LV_ALIGN_IN_TOP_RIGHT, -4, 4);
    snake_labels_front();
}

void snake_set_dir(int8_t dir)
{
    dir += snake.dir;
    if (dir < SNAKE_DIR_BEGIN)
        dir = SNAKE_DIR_END;
    if (dir > SNAKE_DIR_END)
        dir = SNAKE_DIR_BEGIN;
    snake.dir = dir;
    lv_label_set_text(snake.eye, ui_snake_eyes[snake.dir - SNAKE_DIR_BEGIN]);
}

static void snake_add_body(lv_obj_t *parent)
{
    if (snake.size >= UI_SNAKE_MAX_BODY)
        return;

    int index = snake.size++;
    snake.body[index] = lv_btn_create(parent, NULL);
    lv_btn_toggle(snake.body[index]);
    lv_obj_set_size(snake.body[index], UI_SNAKE_BODY_SIZE, UI_SNAKE_BODY_SIZE);
}

void snake_reset(lv_obj_t *parent)
{
    if(snake_task_handle)
        lv_task_set_prio(snake_task_handle, LV_TASK_PRIO_OFF);

    srand(lv_tick_get());

    // high score: load once, then let the just-finished game set a new record
    static bool hi_loaded = false;
    if (!hi_loaded) { snake.high = snake_load_high(); hi_loaded = true; }
    if (snake.score > (int)snake.high) { snake.high = snake.score; snake_save_high(snake.high); }
    snake.score = 0;

    // HUD labels: black text (white screen), top corners like Space Invaders.
    // Created once (snake_reset is also the death handler and doesn't clear them).
    if (!snake.lbl_score) {
        snake.lbl_score = lv_label_create(parent, NULL);
        lv_obj_set_style_local_text_color(snake.lbl_score, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
        lv_obj_align(snake.lbl_score, NULL, LV_ALIGN_IN_TOP_LEFT, 4, 4);
    }
    if (!snake.lbl_high) {
        snake.lbl_high = lv_label_create(parent, NULL);
        lv_obj_set_style_local_text_color(snake.lbl_high, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
        lv_obj_align(snake.lbl_high, NULL, LV_ALIGN_IN_TOP_RIGHT, -4, 4);
    }

    for (int i = 0; i < snake.size; i++)
    {
        lv_obj_del(snake.body[i]);
        snake.body[i] = NULL;
    }
    snake.size = 0;
    if (snake.food)
    {
        lv_obj_del(snake.food);
        snake.food = NULL;
    }

    snake_add_body(parent); // head
    snake.eye = lv_label_create(snake.body[0], NULL);
    snake_set_dir(SNAKE_DIR_RIGHT);
    lv_obj_set_pos(snake.body[0], UI_SNAKE_BODY_SIZE * 3, UI_SNAKE_BODY_SIZE * 6);

    snake_add_body(parent);
    lv_obj_set_pos(snake.body[1], UI_SNAKE_BODY_SIZE * 2, UI_SNAKE_BODY_SIZE * 6);
    snake_add_body(parent);
    lv_obj_set_pos(snake.body[2], UI_SNAKE_BODY_SIZE * 1, UI_SNAKE_BODY_SIZE * 6);

    snake.speed = SNAKE_MIN_SPEED;
    snake_update_labels();
}

static int counter = 0;

void snake_task(lv_task_t *arg)
{
    // not current page, ignore.
    if (lv_scr_act() != screen_snake){
        lv_task_set_prio(snake_task_handle, LV_TASK_PRIO_OFF);
        return;
    }

    if(counter < snake.speed) {
        counter++;
        return;
    }
    
    counter = 0;

    lv_obj_t *parent = screen_snake;
    lv_coord_t x = lv_obj_get_x(snake.body[0]);
    lv_coord_t y = lv_obj_get_y(snake.body[0]);

    switch (snake.dir)
    {
        case SNAKE_DIR_UP:
            y -= UI_SNAKE_BODY_SIZE;
            break;
        case SNAKE_DIR_DOWN:
            y += UI_SNAKE_BODY_SIZE;
            break;
        case SNAKE_DIR_LEFT:
            x -= UI_SNAKE_BODY_SIZE;
            break;
        case SNAKE_DIR_RIGHT:
            x += UI_SNAKE_BODY_SIZE;
            break;
    }

    // check if head hit any wall.
    if (x >= LV_HOR_RES || x < 0 || y >= LV_VER_RES || y < 0)
    {
        snake_reset(screen_snake);
        return;
    }
    // check if head hit any body.
    for (int i = 1; i < snake.size; i++)
    {
        if (x == lv_obj_get_x(snake.body[i]) && y == lv_obj_get_y(snake.body[i]))
        {
            snake_reset(screen_snake);
            return;
        }
    }
    // check if head hit any food.
    if (snake.food)
    {
        if (x == lv_obj_get_x(snake.food) && y == lv_obj_get_y(snake.food))
        {
            lv_obj_del(snake.food);
            snake.food = NULL;
            snake_add_body(parent);

            snake.score++;                 // one point per ball eaten
            if (snake.score > (int)snake.high) snake.high = snake.score;
            snake_update_labels();

            if(snake.size % SNAKE_STEP_SPEED == 0 && snake.speed > 0){
                snake.speed--;
                ESP_LOGI(__FILE__, "Snake speed is: %d", snake.speed);
            }
        }
    }
    // if no food exists, we need to generate one in random place.
    else
    {
        snake.food = lv_btn_create(screen_snake, NULL);
        lv_obj_set_size(snake.food, UI_SNAKE_BODY_SIZE, UI_SNAKE_BODY_SIZE);
        lv_obj_set_pos(snake.food,
                       (rand() % LV_HOR_RES) / UI_SNAKE_BODY_SIZE * UI_SNAKE_BODY_SIZE,
                       (rand() % LV_VER_RES) / UI_SNAKE_BODY_SIZE * UI_SNAKE_BODY_SIZE);
        snake_labels_front();   // keep the HUD above the freshly spawned food
    }

    // now we can move the body.
    for (int i = snake.size - 1; i >= 1; i--)
    {
        lv_coord_t cx = lv_obj_get_x(snake.body[i - 1]);
        lv_coord_t cy = lv_obj_get_y(snake.body[i - 1]);
        lv_obj_set_pos(snake.body[i], cx, cy);
    }
    lv_obj_set_pos(snake.body[0], x, y); // head is last one.
}