#include "radar.h"

static lv_obj_t *radar_node[MAX_NEARBY_NODE] = {0};
static lv_obj_t *radar_node_number[MAX_NEARBY_NODE] = {0};

static void ui_radar_timer(lv_timer_t *arg) {

  if (lv_scr_act() != screen_radar) {
    lv_timer_pause(radar_timer_handle);
    return;
  } else {
    static int mode;
    if (mode) {
      for (int i = 0; i < MAX_NEARBY_NODE; i++) {
        if (ble_nodes[i].active) {
          lv_coord_t x = rand() % 80, y = rand() % 60;

          if (ble_nodes[i].rssi > -70) {
            // near range.
            if (x < 50)
              x = (LV_HOR_RES - x) / 2 - 20;
            else
              x = (LV_HOR_RES + x) / 2 - 20;
            if (y < 40)
              y = (LV_VER_RES - y) / 2 - 20;
            else
              y = (LV_VER_RES + y) / 2 - 20;
          } else if (ble_nodes[i].rssi > -90) {
            // middle range.
            if (x < 50)
              x = (LV_HOR_RES - 100 - x) / 2;
            else
              x = (LV_HOR_RES + 100 + x) / 2 - 20;
            if (y < 40)
              y = (LV_VER_RES - 80 - y) / 2;
            else
              y = (LV_VER_RES + 80 - y) / 2 - 20;
          } else {
            // long range
            if (x < 50)
              x = (LV_HOR_RES - 200 - x) / 2;
            else
              x = (LV_HOR_RES + 200 + x) / 2 - 20;
            if (y < 40)
              y = (LV_VER_RES - 160 - y) / 2;
            else
              y = (LV_VER_RES + 160 + y) / 2 - 20;
          }

          lv_obj_set_pos(radar_node[i], x, y);
          lv_label_set_text_fmt(radar_node_number[i], "%d", ble_nodes[i].id);
          lv_obj_clear_flag(radar_node[i], LV_OBJ_FLAG_HIDDEN);
          lv_obj_fade_in(radar_node[i], 1000, 0);
        } else {
          lv_obj_add_flag(radar_node[i], LV_OBJ_FLAG_HIDDEN);
        }
      }
    } else {
      for (int i = 0; i < MAX_NEARBY_NODE; i++) {
        if (lv_obj_is_visible(radar_node[i]))
          lv_obj_fade_out(radar_node[i], 1000, 0);
      }
    }

    mode = !mode;
  }
}

lv_obj_t *ui_screen_radar_init() {
  // Page for radar
  LV_IMG_DECLARE(img_radar);

  screen_radar = lv_obj_create(NULL);
  lv_obj_t *img = lv_img_create(screen_radar);
  lv_img_set_src(img, &img_radar);
  lv_obj_align(img, LV_ALIGN_TOP_LEFT, 0, 0);

  for (int i = 0; i < sizeof(radar_node) / sizeof(lv_obj_t *); i++) {
    radar_node[i] = lv_btn_create(img);
    lv_obj_set_size(radar_node[i], 20, 20);
    lv_obj_get_state(radar_node[i]);
    lv_state_t button_state =
        lv_obj_get_state(radar_node[i]); // set to solid color.
    if (button_state == LV_STATE_PRESSED)
      lv_obj_clear_state(radar_node[i], LV_STATE_PRESSED);
    else
      lv_obj_add_state(radar_node[i], LV_STATE_PRESSED);
    lv_obj_add_flag(radar_node[i], LV_OBJ_FLAG_HIDDEN);
    radar_node_number[i] = lv_label_create(radar_node[i]);
    lv_label_set_text(radar_node_number[i], "X");
  }

  radar_timer_handle = lv_timer_create(ui_radar_timer, 2000, NULL);
  lv_timer_pause(radar_timer_handle);

  return(screen_radar);
}