#include "snake.h"
#include "esp_log.h"

static void ui_snake_set_dir(int8_t dir);

static void ui_snake_timer_handler(lv_timer_t *arg);
static void ui_snake_reset(lv_obj_t *parent);

const char ui_snake_eyes[][4] = {"' '", ": ", ". .", " :"};

static snake_t snake;
static lv_timer_t *snake_timer_handle;
static lv_obj_t *screen_snake;

static uint8_t snake_score;
static uint8_t snake_max_score;
static lv_obj_t *snake_score_label;

static void ui_snake_set_dir(int8_t dir) {
  dir += snake.dir;
  if (dir < SNAKE_DIR_BEGIN)
    dir = SNAKE_DIR_END;
  if (dir > SNAKE_DIR_END)
    dir = SNAKE_DIR_BEGIN;
  snake.dir = dir;
  lv_label_set_text(snake.eye, ui_snake_eyes[snake.dir - SNAKE_DIR_BEGIN]);
}

static void ui_snake_add_body(lv_obj_t *parent) {
  if (snake.size >= UI_SNAKE_MAX_BODY)
    return;

  int index         = snake.size++;
  snake.body[index] = lv_btn_create(parent);
  // TODO togggle
  // lv_btn_toggle(snake.body[index]);
  lv_obj_set_size(snake.body[index], UI_SNAKE_BODY_SIZE, UI_SNAKE_BODY_SIZE);
}

static void ui_snake_reset(lv_obj_t *parent) {
  if (snake_timer_handle)
    lv_timer_pause(snake_timer_handle);

  if (snake_score > snake_max_score)
    snake_max_score = snake_score;
  snake_score = 0;
  lv_label_set_text_fmt(snake_score_label, "%d/%d", snake_score, snake_max_score);

  srand(lv_tick_get());

  for (int i = 0; i < snake.size; i++) {
    lv_obj_del(snake.body[i]);
    snake.body[i] = NULL;
  }
  snake.size = 0;
  if (snake.food) {
    lv_obj_del(snake.food);
    snake.food = NULL;
  }

  ui_snake_add_body(parent);  // head
  snake.eye = lv_label_create(snake.body[0]);
  ui_snake_set_dir(SNAKE_DIR_RIGHT);
  lv_obj_set_pos(snake.body[0], UI_SNAKE_BODY_SIZE * 3, UI_SNAKE_BODY_SIZE * 6);

  ui_snake_add_body(parent);
  lv_obj_set_pos(snake.body[1], UI_SNAKE_BODY_SIZE * 2, UI_SNAKE_BODY_SIZE * 6);
  ui_snake_add_body(parent);
  lv_obj_set_pos(snake.body[2], UI_SNAKE_BODY_SIZE * 1, UI_SNAKE_BODY_SIZE * 6);

  snake.speed = SNAKE_MIN_SPEED;
}

static int counter = 0;

static void ui_snake_timer_handler(lv_timer_t *arg) {
  // not current page, ignore.
  if (lv_scr_act() != screen_snake) {
    lv_timer_pause(snake_timer_handle);
    return;
  }

  if (counter < snake.speed) {
    counter++;
    return;
  }

  counter = 0;

  lv_obj_t *parent = screen_snake;
  lv_coord_t x     = lv_obj_get_x(snake.body[0]);
  lv_coord_t y     = lv_obj_get_y(snake.body[0]);

  switch (snake.dir) {
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
  if (x >= LV_HOR_RES || x < 0 || y >= LV_VER_RES || y < 0) {
    ui_snake_reset(screen_snake);
    return;
  }
  // check if head hit any body.
  for (int i = 1; i < snake.size; i++) {
    if (x == lv_obj_get_x(snake.body[i]) && y == lv_obj_get_y(snake.body[i])) {
      ui_snake_reset(screen_snake);
      return;
    }
  }
  // check if head hit any food.
  if (snake.food) {
    if (x == lv_obj_get_x(snake.food) && y == lv_obj_get_y(snake.food)) {
      lv_obj_del(snake.food);
      snake.food = NULL;
      snake_score++;
      lv_label_set_text_fmt(snake_score_label, "%d/%d", snake_score, snake_max_score);
      ui_snake_add_body(parent);

      if (snake.size % SNAKE_STEP_SPEED == 0 && snake.speed > 0) {
        snake.speed--;
        ESP_LOGI(__FILE__, "Snake speed is: %d", snake.speed);
      }
    }
  }
  // if no food exists, we need to generate one in random place.
  else {
    snake.food = lv_btn_create(screen_snake);
    lv_obj_set_size(snake.food, UI_SNAKE_BODY_SIZE, UI_SNAKE_BODY_SIZE);
    lv_obj_set_pos(
        snake.food,
        (rand() % LV_HOR_RES) / UI_SNAKE_BODY_SIZE * UI_SNAKE_BODY_SIZE,
        (rand() % LV_VER_RES) / UI_SNAKE_BODY_SIZE * UI_SNAKE_BODY_SIZE);
  }

  // now we can move the body.
  for (int i = snake.size - 1; i >= 1; i--) {
    lv_coord_t cx = lv_obj_get_x(snake.body[i - 1]);
    lv_coord_t cy = lv_obj_get_y(snake.body[i - 1]);
    lv_obj_set_pos(snake.body[i], cx, cy);
  }
  lv_obj_set_pos(snake.body[0], x, y);  // head is last one.
}

lv_obj_t *ui_screen_snake_init() {
  // page for snake
  screen_snake    = lv_obj_create(NULL);
  snake_score     = 0;
  snake_max_score = 0;

  snake_score_label = lv_label_create(screen_snake);
  lv_obj_align(snake_score_label, LV_ALIGN_TOP_RIGHT, -5, 5);
  lv_obj_set_style_opa(snake_score_label, 85, LV_PART_MAIN);
  lv_obj_set_style_text_font(snake_score_label, &lv_font_montserrat_18, LV_PART_MAIN);

  ui_snake_reset(screen_snake);
  snake_timer_handle = lv_timer_create(ui_snake_timer_handler, 50, NULL);
  lv_timer_pause(snake_timer_handle);

  return (screen_snake);
}

void ui_snake_button_up() {
  lv_timer_resume(snake_timer_handle);
  ui_snake_set_dir(1);
}
void ui_snake_button_down() {
  lv_timer_resume(snake_timer_handle);
  ui_snake_set_dir(-1);
}