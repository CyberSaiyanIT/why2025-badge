#include "event.h"

static lv_obj_t *table_event;


void ui_event_load() {
  const char *buf = load_schedule_from_file();
  if (buf == NULL) {
    ESP_LOGI(__FILE__, "Failed to load schedule from file");
    return;
  }
  cJSON *schedule_json = cJSON_Parse(buf);
  free((char *)buf);

  cJSON *schedule_array = cJSON_GetObjectItem(schedule_json, "schedule");

  int size = cJSON_GetArraySize(schedule_array);
  lv_table_set_row_cnt(table_event, size);
  // lv_table_set_col_cnt(table_event, 2);

  char buff[256];
  for (int i = 0; i < size; i++) {
    cJSON *item = cJSON_GetArrayItem(schedule_array, i);

    // cJSON *index = cJSON_GetObjectItem(item, "index");
    cJSON *title = cJSON_GetObjectItem(item, "title");
    cJSON *day = cJSON_GetObjectItem(item, "day");
    cJSON *hour = cJSON_GetObjectItem(item, "hour");
    cJSON *speaker = cJSON_GetObjectItem(item, "speaker");
    cJSON *location = cJSON_GetObjectItem(item, "location");
    cJSON *duration = cJSON_GetObjectItem(item, "duration");

    // ESP_LOGI(__FILE__, "Index %d: %s", i, speaker->valuestring);

    if (day && strlen(day->valuestring) > 0) {
      snprintf(buff, sizeof(buff), "Day: %s\n", day->valuestring);
    }
    if (hour && strlen(hour->valuestring) > 0) {
      snprintf(buff + strlen(buff), sizeof(buff) - strlen(buff), "Time: %s\n",
               hour->valuestring);
    }
    if (location && strlen(location->valuestring) > 0) {
      snprintf(buff + strlen(buff), sizeof(buff) - strlen(buff), "Where: %s\n",
               location->valuestring);
    }
    if (duration && strlen(duration->valuestring) > 0) {
      snprintf(buff + strlen(buff), sizeof(buff) - strlen(buff), "How long: %s",
               duration->valuestring);
    }

    lv_table_set_cell_value(table_event, i, 0, buff);

    snprintf(buff, sizeof(buff), "%s\nby %s", title->valuestring,
             speaker->valuestring);
    lv_table_set_cell_value(table_event, i, 1, buff);
  }

  cJSON_Delete(schedule_json);
}

lv_obj_t *ui_screen_event_init() {
  // page for event
  static lv_style_t style;
  lv_style_init(&style);

  lv_obj_t *screen_event = lv_obj_create(NULL);
  lv_obj_set_scrollbar_mode(screen_event, LV_SCROLLBAR_MODE_OFF);

  lv_style_set_text_font(&style, &lv_font_montserrat_14);

  table_event = lv_table_create(screen_event);
  lv_table_set_col_cnt(table_event, 2);
  lv_obj_add_style(table_event, &style, LV_PART_MAIN);

  lv_obj_set_align(table_event, LV_ALIGN_TOP_MID);
  ui_event_load();

  lv_obj_set_width(table_event, lv_pct(100));

  lv_table_set_col_width(table_event, 0, LV_HOR_RES / 2);
  lv_table_set_col_width(table_event, 1, LV_HOR_RES / 2);

  return (screen_event);
}

void event_button_up() {}
void event_button_down() {}