#include "dice.h"
#include "lcd.h"
#include <unistd.h>
#include "stdlib.h"
#include "esp_log.h"
#include "esp_random.h"

#define NB_ROLLS 25
static const char *btnm_map[] = {"D20\nDisadv.", "D20", "D20\nAdv.", "\n",
                                 "D2", "D4", "D6", "D8", "\n",
                                 "D10", "D12", "D14", "D100", ""};
typedef enum {
  NORMAL_ROLL,
  DISADVANTAGE_ROLL,
  ADVANTAGE_ROLL
} dice_roll_t;

typedef struct
{
  uint8_t faces;
  dice_roll_t roll_mode;
} dice_t;

static const dice_t dice_rolls[] = {
    {20, DISADVANTAGE_ROLL},
    {20, NORMAL_ROLL},
    {20, ADVANTAGE_ROLL},
    {2, NORMAL_ROLL},
    {4, NORMAL_ROLL},
    {6, NORMAL_ROLL},
    {8, NORMAL_ROLL},
    {10, NORMAL_ROLL},
    {12, NORMAL_ROLL},
    {14, NORMAL_ROLL},
    {100, NORMAL_ROLL},
};

static lv_obj_t *dice_btnmtx;
static lv_obj_t *result_win;
static lv_obj_t *result_title;
static lv_obj_t *result_label;
static lv_obj_t *roll_label;
lv_timer_t *dice_timer_handle;
static uint32_t dice_id;

void dice_set_button_color(lv_draw_task_t *draw_task, bool pressed, lv_palette_t color) {
  lv_draw_fill_dsc_t *fill_draw_dsc = lv_draw_task_get_fill_dsc(draw_task);
  if (fill_draw_dsc) {
    if (pressed)
      fill_draw_dsc->color = lv_palette_darken(color, 3);
    else
      fill_draw_dsc->color = lv_palette_main(color);
  }
  lv_draw_label_dsc_t *label_draw_dsc = lv_draw_task_get_label_dsc(draw_task);
  if (label_draw_dsc) {
    label_draw_dsc->align = LV_TEXT_ALIGN_CENTER;
    label_draw_dsc->color = lv_color_white();
  }
}

static void dice_draw_event(lv_event_t *e) {
  lv_obj_t *obj                = lv_event_get_target_obj(e);
  lv_draw_task_t *draw_task    = lv_event_get_draw_task(e);
  lv_draw_dsc_base_t *base_dsc = (lv_draw_dsc_base_t *)lv_draw_task_get_draw_dsc(draw_task);

  if (base_dsc->part == LV_PART_ITEMS) {
    bool pressed = false;
    if (lv_buttonmatrix_get_selected_button(obj) == base_dsc->id1 && lv_obj_has_state(obj, LV_STATE_PRESSED))
      pressed = true;

    if (base_dsc->id1 == 0)
      dice_set_button_color(draw_task, pressed, LV_PALETTE_RED);
    else if (base_dsc->id1 == 1)
      dice_set_button_color(draw_task, pressed, LV_PALETTE_BLUE);
    else if (base_dsc->id1 == 2)
      dice_set_button_color(draw_task, pressed, LV_PALETTE_GREEN);
  }
}

static uint8_t dice_random_roll() {
  return (esp_random() % dice_rolls[dice_id].faces) + 1;
}

static void dice_timer(lv_timer_t *timer) {
  uint8_t roll[2];
  uint8_t result = 0;

  switch (dice_rolls[dice_id].roll_mode) {
    case NORMAL_ROLL:
      result = dice_random_roll(dice_id);
      lv_label_set_text_fmt(roll_label, "%d", result);
      break;
    case DISADVANTAGE_ROLL:
      roll[0] = dice_random_roll(dice_id);
      roll[1] = dice_random_roll(dice_id);
      result  = MIN(roll[0], roll[1]);
      lv_label_set_text_fmt(roll_label, "%d - %d", roll[0], roll[1]);
      break;
    case ADVANTAGE_ROLL:
      roll[0] = dice_random_roll(dice_id);
      roll[1] = dice_random_roll(dice_id);
      result  = MAX(roll[0], roll[1]);
      lv_label_set_text_fmt(roll_label, "%d - %d", roll[0], roll[1]);
      break;
  }
  lv_label_set_text_fmt(result_label, "#0000ff %d#", result);
}

static void dice_reset_timer() {
  lv_timer_set_repeat_count(dice_timer_handle, NB_ROLLS);
  lv_timer_resume(dice_timer_handle);
}

static void dice_show_result_win() {
  const char *dice_name = lv_buttonmatrix_get_button_text(dice_btnmtx, dice_id);

  lv_label_set_text(result_title, dice_name);
  lv_label_set_text(result_label, "");
  lv_label_set_text(roll_label, "");
  lv_obj_remove_flag(result_win, LV_OBJ_FLAG_HIDDEN);
  lv_obj_remove_flag(dice_btnmtx, LV_OBJ_FLAG_CLICKABLE);
}

static void dice_roll_event(lv_event_t *e) {
  lv_obj_t *obj = lv_event_get_target_obj(e);
  dice_id       = lv_buttonmatrix_get_selected_button(obj);

  dice_show_result_win();
  dice_reset_timer();
}

static void dice_reroll_event(lv_event_t *e) {
  lv_timer_set_repeat_count(dice_timer_handle, NB_ROLLS);
  lv_timer_resume(dice_timer_handle);
}

static void dice_close_event(lv_event_t *e) {
  lv_obj_add_flag(result_win, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(dice_btnmtx, LV_OBJ_FLAG_CLICKABLE);
  lv_timer_pause(dice_timer_handle);
}

lv_obj_t *ui_screen_dice_init() {
  lv_obj_t *screen_dice = lv_obj_create(NULL);

  static lv_style_t style_bg;
  lv_style_init(&style_bg);
  lv_style_set_pad_all(&style_bg, 10);
  lv_style_set_pad_gap(&style_bg, 10);
  lv_style_set_border_width(&style_bg, 0);

  static lv_style_t style_result_win;
  lv_style_init(&style_result_win);
  lv_style_set_border_width(&style_result_win, 2);
  lv_style_set_border_color(&style_result_win, lv_palette_lighten(LV_PALETTE_GREY, 1));

  static lv_style_t style_result;
  lv_style_init(&style_result);
  lv_style_set_text_font(&style_result, &lv_font_montserrat_36);

  dice_btnmtx = lv_buttonmatrix_create(screen_dice);
  lv_buttonmatrix_set_map(dice_btnmtx, btnm_map);
  lv_obj_set_size(dice_btnmtx, LCD_H_RES, LCD_V_RES);
  lv_obj_align(dice_btnmtx, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_style(dice_btnmtx, &style_bg, 0);
  lv_obj_add_event_cb(dice_btnmtx, dice_roll_event, LV_EVENT_PRESSED, NULL);

  lv_obj_add_event_cb(dice_btnmtx, dice_draw_event, LV_EVENT_DRAW_TASK_ADDED, NULL);
  lv_obj_add_flag(dice_btnmtx, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);

  result_win = lv_win_create(screen_dice);
  lv_obj_add_flag(result_win, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_style(result_win, &style_result_win, LV_PART_MAIN);
  lv_obj_set_size(result_win, 200, 180);
  lv_obj_center(result_win);
  lv_obj_t *result_header = lv_win_get_header(result_win);
  lv_obj_set_height(result_header, 30);
  result_title = lv_win_add_title(result_win, "Roll results");
  lv_obj_center(result_title);
  lv_obj_t *result_close = lv_win_add_button(result_win, LV_SYMBOL_CLOSE, 30);
  lv_obj_add_event_cb(result_close, dice_close_event, LV_EVENT_PRESSED, NULL);

  lv_obj_t *result_cont = lv_win_get_content(result_win);
  lv_obj_remove_flag(result_cont, LV_OBJ_FLAG_SCROLLABLE);

  result_label = lv_label_create(result_cont);
  lv_obj_remove_flag(result_label, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(result_label, LV_OBJ_FLAG_CLICKABLE);
  lv_label_set_long_mode(result_label, LV_LABEL_LONG_WRAP);
  lv_label_set_recolor(result_label, true);
  lv_obj_set_style_text_align(result_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_add_style(result_label, &style_result, LV_PART_MAIN);
  lv_label_set_text(result_label, "");
  lv_obj_align(result_label, LV_ALIGN_CENTER, 0, -30);
  lv_obj_add_event_cb(result_label, dice_reroll_event, LV_EVENT_PRESSED, NULL);

  roll_label = lv_label_create(result_cont);
  lv_obj_remove_flag(roll_label, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(roll_label, LV_OBJ_FLAG_CLICKABLE);
  lv_label_set_long_mode(roll_label, LV_LABEL_LONG_WRAP);
  lv_label_set_recolor(roll_label, true);
  lv_obj_set_style_text_align(roll_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_label_set_text(roll_label, "");
  lv_obj_align(roll_label, LV_ALIGN_CENTER, 0, 10);
  lv_obj_add_event_cb(roll_label, dice_reroll_event, LV_EVENT_PRESSED, NULL);

  result_close = lv_button_create(result_cont);
  lv_obj_align(result_close, LV_ALIGN_BOTTOM_MID, -50, 5);
  lv_obj_t *result_close_label = lv_label_create(result_close);
  lv_label_set_text(result_close_label, "Close");
  lv_obj_center(result_close_label);
  lv_obj_add_event_cb(result_close, dice_close_event, LV_EVENT_PRESSED, NULL);
  lv_obj_add_flag(result_close_label, LV_OBJ_FLAG_EVENT_BUBBLE);

  lv_obj_t *result_reroll = lv_button_create(result_cont);
  lv_obj_align(result_reroll, LV_ALIGN_BOTTOM_MID, 50, 5);
  lv_obj_t *result_reroll_label = lv_label_create(result_reroll);
  lv_label_set_text(result_reroll_label, "Reroll");
  lv_obj_center(result_reroll_label);
  lv_obj_add_event_cb(result_reroll, dice_reroll_event, LV_EVENT_PRESSED, NULL);
  lv_obj_add_flag(result_reroll, LV_OBJ_FLAG_EVENT_BUBBLE);

  dice_timer_handle = lv_timer_create(dice_timer, 100, NULL);
  lv_timer_set_auto_delete(dice_timer_handle, false);

  return screen_dice;
}
void dice_button_up() {
  if (lv_obj_has_flag(result_win, LV_OBJ_FLAG_HIDDEN)) {
    dice_id = 1;  // D20
    dice_show_result_win();
  }
  dice_reset_timer();
}
void dice_button_down() {
  if (lv_obj_has_flag(result_win, LV_OBJ_FLAG_HIDDEN)) {
    dice_id = 2;  // D20 ADV
    dice_show_result_win();
    dice_reset_timer();
  } else {
    lv_obj_add_flag(result_win, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(dice_btnmtx, LV_OBJ_FLAG_CLICKABLE);
    lv_timer_pause(dice_timer_handle);
  }
}