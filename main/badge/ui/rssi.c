#include "rssi.h"

static lv_obj_t *table_rssi;

static void ui_rssi_timer(lv_timer_t *arg) {
  if (lv_scr_act() != screen_rssi) {
    lv_timer_pause(rssi_timer_handle);
    return;
  }

  const uint8_t count = count_ble_nodes();
  lv_table_set_row_cnt(table_rssi, count + 1);

  char buf[BADGE_BUF_SIZE] = {0};

  uint8_t pos = 1;

  for (int i = 0; i < MAX_NEARBY_NODE; i++) {
    if (!ble_nodes[i].active)
      continue;

    lv_table_set_cell_value(table_rssi, pos, 0, ble_nodes[i].name);
    // lv_table_set_cell_align(table_rssi, pos, 0, LV_TEXT_ALIGN_CENTER);

    snprintf(buf, sizeof(buf), "%d dBm", ble_nodes[i].rssi);
    lv_table_set_cell_value(table_rssi, pos, 1, buf);
    // lv_table_set_cell_align(table_rssi, pos, 1, LV_TEXT_ALIGN_CENTER);

    snprintf(buf, sizeof(buf), "0x0%d", ble_nodes[i].id);
    lv_table_set_cell_value(table_rssi, pos, 2, buf);
    // lv_table_set_cell_align(table_rssi, pos, 2, LV_TEXT_ALIGN_CENTER);

    pos++;
  }
}

lv_obj_t *ui_screen_rssi_init() {
  // page for rssi
  static lv_style_t style;
  lv_style_init(&style);

  screen_rssi = lv_obj_create(NULL);
  lv_obj_set_scrollbar_mode(screen_rssi, LV_SCROLLBAR_MODE_OFF);

  lv_style_set_text_font(&style, &lv_font_montserrat_14);

  table_rssi = lv_table_create(screen_rssi);
  lv_obj_set_scrollbar_mode(table_rssi, LV_SCROLLBAR_MODE_OFF);
  lv_table_set_col_cnt(table_rssi, 3);
  lv_obj_add_style(table_rssi, &style, LV_PART_MAIN);

  lv_obj_set_align(table_rssi, LV_ALIGN_TOP_MID);

  lv_table_set_cell_value(table_rssi, 0, 0, "NAME");
  lv_table_set_cell_value(table_rssi, 0, 1, "RSSI");
  lv_table_set_cell_value(table_rssi, 0, 2, "ID");

  lv_obj_set_width(table_rssi, lv_pct(100));
  lv_obj_set_height(table_rssi, lv_pct(100));

  lv_table_set_col_width(table_rssi, 0, LV_HOR_RES / 3);
  lv_table_set_col_width(table_rssi, 1, LV_HOR_RES / 3);
  lv_table_set_col_width(table_rssi, 2, LV_HOR_RES / 3);

  rssi_timer_handle = lv_timer_create(ui_rssi_timer, 2000, NULL);
  lv_timer_pause(rssi_timer_handle);

  return (screen_rssi);
}