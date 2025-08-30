#include "ui.h"
#include "led.h"

enum screen_order {
  SCREEN_LOGO,
  SCREEN_PERSON,
  SCREEN_SOCIALENERGY,
  SCREEN_EVENT,
  SCREEN_RADAR,
  SCREEN_RSSI,
  SCREEN_ADMIN,
  SCREEN_SNAKE,
  NUM_SCREENS
};
const int nb_screens = 9;
static lv_obj_t* screens[NUM_SCREENS];
static int8_t current_screen = SCREEN_LOGO;

static uint32_t last_trigger = -1;

static lv_obj_t *person_name;
static lv_obj_t *person_organization;
static lv_obj_t *person_job;
static lv_obj_t *person_message;

static lv_obj_t *socialenergy_gauge;

static lv_obj_t *radar_node[MAX_NEARBY_NODE] = {0};
static lv_obj_t *radar_node_number[MAX_NEARBY_NODE] = {0};
static lv_obj_t *table_rssi, *table_event;
static lv_obj_t *admin_switch, *admin_switch_sta, *admin_sync;
static lv_obj_t *admin_switch_text, *admin_switch_sta_text, *admin_sync_text;
static lv_obj_t *admin_ssid, *admin_pass;
static lv_obj_t *admin_client_ip, *admin_gateway_ip;

static bool ap_started = false;
static bool sta_connected = false;

static uint8_t admin_state = ADMIN_STATE_OFF;

static uint8_t up_button_press_counter = 0;
static uint8_t down_button_press_counter = 0;
static int8_t counter_screen = -1; // Initialize to invalid screen index

// Forward declarations
void ui_update_ip_info(void);
void ui_list_all_netifs(void);

void restore_current_task(){
    if(current_screen == SCREEN_RSSI){
        lv_task_set_prio(rssi_task_handle, LV_TASK_PRIO_LOW);
    }
    else if(current_screen == SCREEN_RADAR){
        lv_task_set_prio(radar_task_handle, LV_TASK_PRIO_LOW);
    }
}

void pause_current_task(){
    if(current_screen == SCREEN_RSSI){
        lv_task_set_prio(rssi_task_handle, LV_TASK_PRIO_OFF);
    }
    else if(current_screen == SCREEN_RADAR){
        lv_task_set_prio(radar_task_handle, LV_TASK_PRIO_OFF);
    }
}

/*
* Update the screen backlight status
* returns status of backlight (true if backlight off)
*/

static bool ui_update_backlight(bool trigger)
{
    uint32_t span = lv_tick_get() - last_trigger;

    if (trigger)
    {
        set_screen_led_backlight(badge_obj.brightness_max);
        last_trigger = lv_tick_get();

        restore_current_task();
    }
    else
    {
        if (span > BRIGHT_OFF_TIMEOUT_MS){
            set_screen_led_backlight(badge_obj.brightness_off);
            pause_current_task();
        }
        else if (span > BRIGHT_MID_TIMEOUT_MS){
            set_screen_led_backlight(badge_obj.brightness_mid);
        }
    }

    /* Avoid doing action when backlight off */
    if (span > BRIGHT_OFF_TIMEOUT_MS){
        return true;
    }
    return false;
}

void ui_send_wifi_event(int event)
{
    xQueueSend(wifi_queue, &event, portMAX_DELAY);
}

void scroll_up(lv_obj_t *screen){
    lv_obj_t *page = lv_obj_get_child(screen, NULL);
    lv_page_scroll_ver(page, 80);
}

void scroll_down(lv_obj_t *screen){
    lv_obj_t *page = lv_obj_get_child(screen, NULL);
    lv_page_scroll_ver(page, -80);
}

void ui_button_up()
{
    // Check if this is the first button press or if we've changed screens
    if (counter_screen != current_screen) {
        // Reset counters when screen changes
        up_button_press_counter = 0;
        down_button_press_counter = 0;
        // Update counter_screen to current screen
        counter_screen = current_screen;
        //printf("DEBUG: Started counting UP presses on screen index: %d\n", current_screen);
    }
    
    // Increment counter for UP button presses
    up_button_press_counter++;
    ESP_LOGI("UI", "UP button press count: %d on screen index: %d", up_button_press_counter, current_screen);
    
    // Check if we've reached nb_screens presses
    if (up_button_press_counter == nb_screens
    ) {
        // printf("DEBUG: UP button pressed nb_screens times on screen %s (index: %d)\n", current_screen);
        
        // Call set_completed() function when on SCREEN_LOGO (index 0)
        if (current_screen == SCREEN_LOGO) {
            ESP_LOGI("UI", "Summoning sequence activated!");
            set_completed();
        }        
        
        // Reset the counter after reaching nb_screens
        up_button_press_counter = 0;
    }

    // Check if the backlight update is needed
    if(ui_update_backlight(true)){
        return;
    }
    int needle_value = 0;

    switch (current_screen) {
      case SCREEN_PERSON:
            ui_set_person(true);
            break;
      case SCREEN_SOCIALENERGY:
            needle_value = lv_gauge_get_value(socialenergy_gauge, 0) - 5;
            needle_value = needle_value < 0 ? 0 : needle_value; // Low limit to 0
            lv_gauge_set_value(socialenergy_gauge, 0, needle_value);
        break;
      case SCREEN_SNAKE:
            lv_task_set_prio(snake_task_handle, LV_TASK_PRIO_LOW);
            snake_set_dir(1);
            break;
        case SCREEN_ADMIN:
            switch(admin_state){
                case ADMIN_STATE_OFF: // AP and STA disabled: enable AP
                    ui_send_wifi_event(EVENT_HOTSPOT_START);
                    lv_obj_set_hidden(admin_switch_sta, true);
                    admin_state = ADMIN_STATE_AP;
                    break;
                case ADMIN_STATE_AP: // AP enabled: disable AP
                    ui_send_wifi_event(EVENT_HOTSPOT_STOP);
                    lv_obj_set_hidden(admin_switch_sta, false);
                    lv_obj_set_hidden(admin_ssid, true);
                    admin_state = ADMIN_STATE_OFF;
                    break;
                case ADMIN_STATE_STA: // STA connected: manual IP refresh
                    ESP_LOGI("UI", "Manual IP refresh triggered via UP button");
                    ui_manual_ip_update();
                    // Also test forcing labels to be visible for debugging
                    ui_force_show_ip_labels();
                    break;
            }
            break;
        case SCREEN_EVENT:
        case SCREEN_RSSI:
            scroll_up(screens[current_screen]);
            break;
        default:
            ESP_LOGI(__FILE__, "Button up, no actions");
    }
}

void ui_button_down()
{
    // Check if this is the first button press or if we've changed screens
    if (counter_screen != current_screen) {
        // Reset counters when screen changes
        up_button_press_counter = 0;
        down_button_press_counter = 0;
        // Update counter_screen to current screen
        counter_screen = current_screen;
    }
    
    // Increment counter for DOWN button presses
    down_button_press_counter++;
    ESP_LOGI("UI", "DOWN button press count: %d on screen index: %d", down_button_press_counter, current_screen);
    
    // Check if we've reached nb_screens presses
    if (down_button_press_counter == nb_screens) {
        // Call rainbow() function when on SCREEN_LOGO (index 0)
        if (current_screen == SCREEN_LOGO) {
            ESP_LOGI("UI", "Rainbow sequence activated!");
            rainbow();
        }        
        
        // Reset the counter after reaching nb_screens
        down_button_press_counter = 0;
    }

    if(ui_update_backlight(true)){
        return;
    }

    int needle_value = 0;

    switch(current_screen){
      case SCREEN_PERSON:
            ui_set_person(false);
            break;
      case SCREEN_SOCIALENERGY:
            needle_value = lv_gauge_get_value(socialenergy_gauge, 0) + 5;
            needle_value = needle_value > 100 ? 100 : needle_value; // TOP limit to 100
            lv_gauge_set_value(socialenergy_gauge, 0, needle_value);
        break;

      case SCREEN_SNAKE:
            lv_task_set_prio(snake_task_handle, LV_TASK_PRIO_LOW);
            snake_set_dir(-1);
            break;
        case SCREEN_ADMIN:
            switch(admin_state){
                case ADMIN_STATE_OFF: // AP and STA disabled: enable STA
                    ui_send_wifi_event(EVENT_STA_START);
                    lv_label_set_text(admin_switch_sta_text, "Started...");
                    admin_state = ADMIN_STATE_STA;
                    break;
                case ADMIN_STATE_AP: // AP enabled: test showing IP labels
                    ESP_LOGI("UI", "Force show IP labels test (DOWN button in AP mode)");
                    ui_force_show_ip_labels();
                    break;
                case ADMIN_STATE_STA: // STA mode: test showing IP labels  
                    ESP_LOGI("UI", "Force show IP labels test (DOWN button in STA mode)");
                    ui_force_show_ip_labels();
                    break;
            }
            break;
        case SCREEN_EVENT:
        case SCREEN_RSSI:
            scroll_down(screens[current_screen]);
            break;
        default:
            ESP_LOGI(__FILE__, "Button down, no actions");
    }
}

void ui_event_load()
{
    const char* buf = load_schedule_from_file();
    if (buf == NULL)
    {
        ESP_LOGI(__FILE__, "Failed to load schedule from file");
        return;
    }
    cJSON* schedule_json = cJSON_Parse(buf);
    free((char*)buf);

    cJSON* schedule_array = cJSON_GetObjectItem(schedule_json, "schedule");

    int size = cJSON_GetArraySize(schedule_array);
    lv_table_set_row_cnt(table_event, size);
    //lv_table_set_col_cnt(table_event, 2);

    char buff[256];
    for (int i = 0; i < size; i++)
    {
        cJSON* item = cJSON_GetArrayItem(schedule_array, i);

        //cJSON *index = cJSON_GetObjectItem(item, "index");
        cJSON *title = cJSON_GetObjectItem(item, "title");
        cJSON *day = cJSON_GetObjectItem(item, "day");
        cJSON *hour = cJSON_GetObjectItem(item, "hour");
        cJSON *speaker = cJSON_GetObjectItem(item, "speaker");
        cJSON *location = cJSON_GetObjectItem(item, "location");
        cJSON *duration = cJSON_GetObjectItem(item, "duration");

        //ESP_LOGI(__FILE__, "Index %d: %s", i, speaker->valuestring);

        if (day && strlen(day->valuestring) > 0) {
            snprintf(buff, sizeof(buff), "Day: %s\n", day->valuestring);
        }
        if (hour && strlen(hour->valuestring) > 0) {
            snprintf(buff + strlen(buff), sizeof(buff) - strlen(buff), "Time: %s\n", hour->valuestring);
        }
        if (location && strlen(location->valuestring) > 0) {
            snprintf(buff + strlen(buff), sizeof(buff) - strlen(buff), "Where: %s\n", location->valuestring);
        }
        if (duration && strlen(duration->valuestring) > 0) {
            snprintf(buff + strlen(buff), sizeof(buff) - strlen(buff), "How long: %s", duration->valuestring);
        }
        
        lv_table_set_cell_value(table_event, i, 0, buff);

        snprintf(buff, sizeof(buff), "%s\nby %s",
            title->valuestring, speaker->valuestring);
        lv_table_set_cell_value(table_event, i, 1, buff);
    }

    cJSON_Delete(schedule_json);
}

static void ui_rssi_task(lv_task_t *arg)
{
    if (lv_scr_act() != screen_rssi)
    {
        lv_task_set_prio(rssi_task_handle, LV_TASK_PRIO_OFF);
        return;
    }
    
    const uint8_t count = count_ble_nodes();
    lv_table_set_row_cnt(table_rssi, count+1);

    char buf[BADGE_BUF_SIZE] = {0};

    uint8_t pos = 1;

    for (int i = 0; i < MAX_NEARBY_NODE; i++) {
        if(!ble_nodes[i].active) continue;
        
        lv_table_set_cell_value(table_rssi, pos, 0, ble_nodes[i].name);
        lv_table_set_cell_align(table_rssi, pos, 0, LV_LABEL_ALIGN_CENTER);

        snprintf(buf, sizeof(buf), "%d dBm", ble_nodes[i].rssi);
        lv_table_set_cell_value(table_rssi, pos, 1, buf);
        lv_table_set_cell_align(table_rssi, pos, 1, LV_LABEL_ALIGN_CENTER);

        snprintf(buf, sizeof(buf), "0x0%d", ble_nodes[i].id);
        lv_table_set_cell_value(table_rssi, pos, 2, buf);
        lv_table_set_cell_align(table_rssi, pos, 2, LV_LABEL_ALIGN_CENTER);

        pos++;
    }
}

static void ui_backlight_task(lv_task_t *arg){
    ui_update_backlight(false);
}

static void ui_radar_task(lv_task_t *arg)
{

    if (lv_scr_act() != screen_radar)
    {
        lv_task_set_prio(radar_task_handle, LV_TASK_PRIO_OFF);
        return;
    }

    if (lv_scr_act() == screen_radar)
    {
        static int mode;
        if (mode)
        {
            for (int i = 0; i < MAX_NEARBY_NODE; i++)
            {
                if(ble_nodes[i].active) {
                    lv_coord_t x = rand() % 80, y = rand() % 60;

                    if (ble_nodes[i].rssi > -70)
                    {
                        // near range.
                        if (x < 50)
                            x = (LV_HOR_RES - x) / 2 - 20;
                        else
                            x = (LV_HOR_RES + x) / 2 - 20;
                        if (y < 40)
                            y = (LV_VER_RES - y) / 2 - 20;
                        else
                            y = (LV_VER_RES + y) / 2 - 20;
                    }
                    else if (ble_nodes[i].rssi > -90)
                    {
                        // middle range.
                        if (x < 50)
                            x = (LV_HOR_RES - 100 - x) / 2;
                        else
                            x = (LV_HOR_RES + 100 + x) / 2 - 20;
                        if (y < 40)
                            y = (LV_VER_RES - 80 - y) / 2;
                        else
                            y = (LV_VER_RES + 80 - y) / 2 - 20;
                    }
                    else
                    {
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
                    lv_obj_set_hidden(radar_node[i], false);
                    lv_obj_fade_in(radar_node[i], 1000, 0);
                } else {
                    lv_obj_set_hidden(radar_node[i], true);
                }
            }
        }
        else
        {
            for (int i = 0; i < MAX_NEARBY_NODE; i++)
            {
                if (lv_obj_is_visible(radar_node[i]))
                    lv_obj_fade_out(radar_node[i], 1000, 0);
            }
        }

        mode = !mode;
    }
}

void ui_screen_event_init() {
    // page for event
    static lv_style_t style;
    lv_style_init(&style);

    screen_event = lv_obj_create(NULL, NULL);
    lv_obj_t *screen_event_page = lv_page_create(screen_event, NULL);

    lv_obj_t *scrollable = lv_page_get_scrollable(screen_event_page);
    lv_cont_set_layout(scrollable, LV_LAYOUT_PRETTY_TOP);
    lv_obj_set_style_local_pad_all(scrollable, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
	lv_obj_set_style_local_pad_inner(scrollable, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_all(screen_event_page, LV_PAGE_PART_BG, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_inner(screen_event_page, LV_PAGE_PART_BG, LV_STATE_DEFAULT, 0);
    lv_page_set_scrollbar_mode(screen_event_page, LV_SCROLLBAR_MODE_OFF);

    lv_obj_set_size(screen_event_page, LV_HOR_RES, LV_VER_RES);
    lv_coord_t content_w = lv_obj_get_width_grid(screen_event_page, 2, 1);

    lv_style_set_text_font(&style, LV_OBJ_PART_MAIN, &lv_font_montserrat_14);

    table_event = lv_table_create(screen_event_page, NULL);
    lv_obj_clean_style_list(table_event, LV_TABLE_PART_BG);
    lv_obj_set_drag_parent(table_event, true);
    lv_table_set_col_cnt(table_event, 2);
    lv_table_set_col_width(table_event, 0, 4*(2 * content_w)/10);
    lv_table_set_col_width(table_event, 1, 6*(2 * content_w)/10);
    lv_obj_add_style(table_event, LV_OBJ_PART_MAIN, &style);

    lv_obj_align(table_event, screen_event_page, LV_ALIGN_OUT_TOP_LEFT, 0, 0);

    ui_event_load(); 

    screens[SCREEN_EVENT] = screen_event;
}

void ui_screen_splash_init(){
    LV_IMG_DECLARE(img_logo);

    screen_logo = lv_obj_create(NULL, NULL);

    lv_obj_t *logo = lv_img_create(screen_logo, NULL);
    lv_img_set_src(logo, &img_logo);
    lv_obj_align(logo, NULL, LV_ALIGN_CENTER, 0, 0);
  /*Change the logo's background color*/
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_opa(&style, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_style_set_bg_color(&style, LV_STATE_DEFAULT, LV_COLOR_MAKE(0x34, 0x3a, 0x40));
    lv_obj_add_style(logo, LV_OBJ_PART_MAIN, &style);

    screens[SCREEN_LOGO] = screen_logo;
}

void ui_set_person(bool secret) {
    if (secret == true) {
        lv_label_set_text(person_name, badge_obj.secret_name);
        lv_label_set_text(person_organization, badge_obj.secret_organization);
        lv_label_set_text(person_job, badge_obj.secret_job);
        lv_label_set_text(person_message, badge_obj.secret_message);      
    } else {
        lv_label_set_text(person_name, badge_obj.person_name);
        lv_label_set_text(person_organization, badge_obj.person_organization);
        lv_label_set_text(person_job, badge_obj.person_job);
        lv_label_set_text(person_message, badge_obj.person_message);
    }
}

void ui_screen_person_init() {
    /* Styling */
    static lv_style_t style_name;
    lv_style_init(&style_name);
    lv_style_set_text_font(&style_name, LV_OBJ_PART_MAIN, &lv_font_montserrat_22);
    lv_style_set_text_decor(&style_name, LV_STATE_DEFAULT, LV_TEXT_DECOR_UNDERLINE);


    static lv_style_t style_organization_job;
    lv_style_init(&style_organization_job);
    lv_style_set_text_font(&style_organization_job, LV_OBJ_PART_MAIN, &lv_font_montserrat_18);

    /* Screen and labels creation */
    screen_person = lv_obj_create(NULL, NULL);

    person_name = lv_label_create(screen_person, NULL);    /*Used as a base label*/
    lv_label_set_long_mode(person_name, LV_LABEL_LONG_BREAK);     /*Break the long lines*/
    lv_label_set_recolor(person_name, true);                      /*Enable re-coloring by commands in the text*/
    lv_label_set_align(person_name, LV_LABEL_ALIGN_CENTER);       /*Center aligned lines*/
    lv_obj_set_width(person_name, NAME_LABEL_SIZE);
    lv_obj_align(person_name, NULL, LV_ALIGN_CENTER, 0, -60);
    
    person_organization = lv_label_create(screen_person, person_name);
    lv_obj_align(person_organization, NULL, LV_ALIGN_CENTER, 0, -30);

    person_job = lv_label_create(screen_person, person_name);
    lv_obj_align(person_job, NULL, LV_ALIGN_CENTER, 0, 0);

    person_message = lv_label_create(screen_person, person_name);
    lv_obj_align(person_message, NULL, LV_ALIGN_CENTER, 0, 60);

    /* Setting styles */
    lv_obj_add_style(person_name, LV_OBJ_PART_MAIN, &style_name);
    lv_obj_add_style(person_organization, LV_OBJ_PART_MAIN, &style_organization_job);
    lv_obj_add_style(person_job, LV_OBJ_PART_MAIN, &style_organization_job);
    /* Stetting texts */
    ui_set_person(false);
    screens[SCREEN_PERSON] = screen_person;
}

void ui_screen_socialenergy_init() {
    /* Gauge Style */
    static lv_style_t gauge_style;
    lv_style_init(&gauge_style);

    lv_style_set_pad_inner(&gauge_style, LV_STATE_DEFAULT, 5);                  // Padding
    
    lv_style_set_line_color(&gauge_style, LV_STATE_DEFAULT, LV_COLOR_GREEN);      
    lv_style_set_scale_grad_color(&gauge_style, LV_STATE_DEFAULT, LV_COLOR_GREEN);
    lv_style_set_scale_end_color(&gauge_style, LV_STATE_DEFAULT, LV_COLOR_GREEN);

    lv_style_set_line_width(&gauge_style, LV_STATE_DEFAULT, 2);
    lv_style_set_scale_end_line_width(&gauge_style, LV_STATE_DEFAULT, 4);
    lv_style_set_scale_end_border_width(&gauge_style, LV_STATE_DEFAULT, 4);

    /* Needle style */
    static lv_style_t needle_style;
    lv_style_init(&needle_style);
    lv_style_set_line_width(&needle_style, LV_STATE_DEFAULT, 5);
    lv_style_set_line_color(&needle_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);

    screen_socialenergy = lv_obj_create(NULL, NULL);
    /*Describe the color for the needles*/
    static lv_color_t needle_colors[0];
    needle_colors[0] = LV_COLOR_BLACK;

    /*Create a gauge*/
    socialenergy_gauge = lv_gauge_create(screen_socialenergy, NULL);

    lv_obj_add_style(socialenergy_gauge, LV_GAUGE_PART_MAIN, &gauge_style);
    lv_obj_add_style(socialenergy_gauge, LV_GAUGE_PART_NEEDLE, &needle_style);
    lv_gauge_set_needle_count(socialenergy_gauge, 1, needle_colors);
    lv_obj_set_size(socialenergy_gauge, 180, 180);
    lv_obj_align(socialenergy_gauge, NULL, LV_ALIGN_IN_TOP_MID, 0, 10);

    /*Set the values*/
    lv_gauge_set_value(socialenergy_gauge, 0, 100);

    /* Label */
    static lv_style_t style_label;
    lv_style_init(&style_label);
    lv_style_set_text_font(&style_label, LV_OBJ_PART_MAIN, &lv_font_montserrat_22);

    lv_obj_t *label = lv_label_create(screen_socialenergy, NULL);
    lv_label_set_long_mode(label, LV_LABEL_LONG_BREAK);     /*Break the long lines*/
    lv_label_set_recolor(label, true);                      /*Enable re-coloring by commands in the text*/
    lv_label_set_align(label, LV_LABEL_ALIGN_CENTER);       /*Center aligned lines*/
    lv_obj_set_width(label, NAME_LABEL_SIZE);
    lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 90);
    lv_obj_add_style(label, LV_OBJ_PART_MAIN, &style_label);

    lv_label_set_text(label, "Social Energy");

    screens[SCREEN_SOCIALENERGY] = screen_socialenergy;
}

void ui_screen_radar_init(){
    // Page for radar
    LV_IMG_DECLARE(img_radar);
    
    screen_radar = lv_obj_create(NULL, NULL);
    lv_obj_t *img = lv_img_create(screen_radar, NULL);
    lv_img_set_src(img, &img_radar);
    lv_obj_align(img, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);

    for (int i = 0; i < sizeof(radar_node) / sizeof(lv_obj_t *); i++)
    {
        radar_node[i] = lv_btn_create(img, NULL);
        lv_obj_set_size(radar_node[i], 20, 20);
        lv_btn_toggle(radar_node[i]); // set to solid color.
        lv_obj_set_hidden(radar_node[i], true);
        radar_node_number[i] = lv_label_create(radar_node[i], NULL);
        lv_label_set_text(radar_node_number[i], "X");
    }

    screens[SCREEN_RADAR] = screen_radar;
}

void ui_screen_rssi_init(){
    // page for rssi
    static lv_style_t style;
    lv_style_init(&style);

    screen_rssi = lv_obj_create(NULL, NULL);
    lv_obj_t *screen_rssi_page = lv_page_create(screen_rssi, NULL);

    lv_obj_t *scrollable = lv_page_get_scrollable(screen_rssi_page);
    lv_cont_set_layout(scrollable, LV_LAYOUT_PRETTY_TOP);
    lv_obj_set_style_local_pad_all(scrollable, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
	lv_obj_set_style_local_pad_inner(scrollable, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_all(screen_rssi_page, LV_PAGE_PART_BG, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_inner(screen_rssi_page, LV_PAGE_PART_BG, LV_STATE_DEFAULT, 0);
    lv_page_set_scrollbar_mode(screen_rssi_page, LV_SCROLLBAR_MODE_OFF);

    lv_obj_set_size(screen_rssi_page, LV_HOR_RES, LV_VER_RES);
    lv_coord_t content_w = lv_obj_get_width_grid(screen_rssi_page, 3, 1);

    lv_style_set_text_font(&style, LV_OBJ_PART_MAIN, &lv_font_montserrat_14);

    table_rssi = lv_table_create(screen_rssi_page, NULL);
    lv_obj_clean_style_list(table_rssi, LV_TABLE_PART_BG);
    lv_obj_set_drag_parent(table_rssi, true);
    lv_table_set_col_cnt(table_rssi, 3);
    lv_table_set_col_width(table_rssi, 0, content_w);
    lv_table_set_col_width(table_rssi, 1, content_w);
    lv_table_set_col_width(table_rssi, 2, content_w);
    lv_obj_add_style(table_rssi, LV_OBJ_PART_MAIN, &style);

    lv_obj_align(table_rssi, screen_rssi_page, LV_ALIGN_OUT_TOP_LEFT, 0, 0);

    lv_table_set_cell_value(table_rssi, 0, 0, "NAME");
    lv_table_set_cell_value(table_rssi, 0, 1, "RSSI");
    lv_table_set_cell_value(table_rssi, 0, 2, "ID");

    lv_table_set_cell_align(table_rssi, 0, 0, LV_LABEL_ALIGN_CENTER);
    lv_table_set_cell_align(table_rssi, 0, 1, LV_LABEL_ALIGN_CENTER);
    lv_table_set_cell_align(table_rssi, 0, 2, LV_LABEL_ALIGN_CENTER);

    screens[SCREEN_RSSI] = screen_rssi;
}

void ui_screen_admin_init(){
    // page for admin
    screen_admin = lv_obj_create(NULL, NULL);
    admin_switch = lv_btn_create(screen_admin, NULL);
    lv_obj_set_size(admin_switch, 200, 50);
    lv_obj_set_pos(admin_switch, 60, 35);
    admin_switch_text = lv_label_create(admin_switch, NULL);
    lv_label_set_text(admin_switch_text, "TURN ON AP");

    admin_sync = lv_btn_create(screen_admin, NULL);
    lv_obj_set_size(admin_sync, 200, 50);
    lv_obj_set_pos(admin_sync, 60, 180);
    admin_sync_text = lv_label_create(admin_sync, NULL);
    lv_label_set_text(admin_sync_text, "FORCE SCHEDULE SYNC");
    lv_obj_set_hidden(admin_sync, true);

    admin_ssid = lv_label_create(screen_admin, NULL);
    lv_obj_align(admin_ssid, admin_switch, LV_ALIGN_OUT_BOTTOM_MID, -50, 10);
    lv_obj_set_hidden(admin_ssid, true);
    admin_pass = lv_label_create(screen_admin, NULL);
    lv_obj_align(admin_pass, admin_switch, LV_ALIGN_OUT_BOTTOM_MID, -50, 30);
    lv_obj_set_hidden(admin_pass, true);

    admin_client_ip = lv_label_create(screen_admin, NULL);
    lv_obj_align(admin_client_ip, admin_switch, LV_ALIGN_OUT_BOTTOM_MID, -50, 50);
    lv_label_set_text(admin_client_ip, "Client IP: [Not Connected]");
    lv_obj_set_hidden(admin_client_ip, true);
    admin_gateway_ip = lv_label_create(screen_admin, NULL);
    lv_obj_align(admin_gateway_ip, admin_switch, LV_ALIGN_OUT_BOTTOM_MID, -50, 70);
    lv_label_set_text(admin_gateway_ip, "Gateway: [Not Available]");
    lv_obj_set_hidden(admin_gateway_ip, true);

    admin_switch_sta = lv_btn_create(screen_admin, NULL);
    lv_obj_set_size(admin_switch_sta, 200, 50);
    lv_obj_set_pos(admin_switch_sta, 60, 180);
    admin_switch_sta_text = lv_label_create(admin_switch_sta, NULL);
    lv_label_set_text(admin_switch_sta_text, "SYNC SCHEDULE");

    screens[SCREEN_ADMIN] = screen_admin;
}

void ui_screen_snake_init(){
    // page for snake
    screen_snake = lv_obj_create(NULL, NULL);
    snake_reset(screen_snake);

    screens[SCREEN_SNAKE] = screen_snake;
}

void ui_ap_start_handler() {
    ap_started = true;

    ESP_LOGI("UI", "AP started handler called");
    lv_label_set_text(admin_switch_text, "TURN OFF AP");

    char buf[BADGE_BUF_SIZE + 19] = {0};
    snprintf(buf, sizeof(buf), "SSID: %s", badge_obj.ap_ssid);
    lv_label_set_text(admin_ssid, buf);
    snprintf(buf, sizeof(buf), "PASS: %s", badge_obj.ap_password);
    lv_label_set_text(admin_pass, buf);

    lv_obj_set_hidden(admin_ssid, false);
    lv_obj_set_hidden(admin_pass, false);

    // Update IP information immediately
    ui_update_ip_info();
    
    // TODO: Also create a delayed task to retry getting IP info
    // xTaskCreate(ui_delayed_ip_update_task, "delayed_ip_update", 2048, NULL, 5, NULL);

    lv_btn_set_state(admin_switch, LV_BTN_STATE_CHECKED_PRESSED);
    admin_state = ADMIN_STATE_AP;
}

void ui_ap_stop_handler(){
    ap_started = false;

    lv_label_set_text(admin_switch_text, "TURN ON AP");
    lv_obj_set_hidden(admin_ssid, true);
    lv_obj_set_hidden(admin_pass, true);
    lv_obj_set_hidden(admin_client_ip, true);
    lv_obj_set_hidden(admin_gateway_ip, true);
    lv_obj_set_hidden(admin_switch_sta, false);

    lv_btn_set_state(admin_switch, LV_BTN_STATE_RELEASED);//enabl(admin_switch);
    admin_state = ADMIN_STATE_OFF;
}

void ui_sta_connected_handler(){
    sta_connected = true;

    ESP_LOGI("UI", "STA connected handler called");
    ESP_LOGI("UI", "Current admin_state: %d", admin_state);
    ESP_LOGI("UI", "Current screen: %d", current_screen);
    
    lv_btn_set_state(admin_switch_sta, LV_BTN_STATE_CHECKED_PRESSED);
    lv_label_set_text(admin_switch_sta_text, "Downloading...");
    
    // Update IP information when connected as station immediately
    ESP_LOGI("UI", "About to call ui_update_ip_info from STA connected handler");
    ui_update_ip_info();
    ESP_LOGI("UI", "ui_update_ip_info call completed from STA connected handler");
    
    // TODO: Also create a delayed task to retry getting IP info
    // xTaskCreate(ui_delayed_ip_update_task, "delayed_ip_update", 2048, NULL, 5, NULL);
    
    admin_state = ADMIN_STATE_STA;
}

void ui_sta_disconnected_handler(){
    sta_connected = false;
    lv_btn_set_state(admin_switch_sta, LV_BTN_STATE_RELEASED);
    lv_obj_set_hidden(admin_client_ip, true);
    lv_obj_set_hidden(admin_gateway_ip, true);
    admin_state = ADMIN_STATE_OFF;
}

void ui_sta_stop_handler() {
    sta_connected = false;
    lv_label_set_text(admin_switch_sta_text, "SYNC SCHEDULE");
    lv_obj_set_hidden(admin_client_ip, true);
    lv_obj_set_hidden(admin_gateway_ip, true);
    admin_state = ADMIN_STATE_OFF;
}

void ui_connection_progress(uint8_t cur, uint8_t max){
    if(cur != max){
        char buf[BADGE_BUF_SIZE + 20] = {0}; // Increase the size of buf to accommodate the entire formatted string
        snprintf(buf, sizeof(buf), "Connecting (%d/%d)", cur, max);
        lv_label_set_text(admin_switch_sta_text, buf);
        lv_obj_set_hidden(admin_switch_sta_text, false);
    } else {
        lv_label_set_text(admin_switch_sta_text, "Connection failed!");
        lv_obj_set_hidden(admin_switch_sta_text, false);
    }
}

void ui_toggle_sync(){
    lv_btn_set_state(admin_sync, LV_BTN_STATE_RELEASED);
    ui_send_wifi_event(EVENT_STA_STOP);
}

void ui_update_ip_info() {
    char buf[BADGE_BUF_SIZE + 30] = {0};
    
    ESP_LOGI("UI", "=== IP INFO DEBUG ===");
    ESP_LOGI("UI", "sta_connected: %s, ap_started: %s, admin_state: %d", 
             sta_connected ? "true" : "false", 
             ap_started ? "true" : "false", 
             admin_state);
    
    // First, list all network interfaces for debugging
    ui_list_all_netifs();
    
    // Get AP interface and show AP IP when AP is started
    if (ap_started) {
        esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
        ESP_LOGI("UI", "AP netif handle (WIFI_AP_DEF): %p", ap_netif);
        
        if (ap_netif) {
            esp_netif_ip_info_t ip_info;
            esp_err_t ret = esp_netif_get_ip_info(ap_netif, &ip_info);
            ESP_LOGI("UI", "AP esp_netif_get_ip_info returned: %s", esp_err_to_name(ret));
            ESP_LOGI("UI", "AP IP: " IPSTR, IP2STR(&ip_info.ip));
            ESP_LOGI("UI", "AP Gateway: " IPSTR, IP2STR(&ip_info.gw));
            ESP_LOGI("UI", "AP Netmask: " IPSTR, IP2STR(&ip_info.netmask));
            
            if (ret == ESP_OK && ip_info.ip.addr != 0) {
                // Show AP IP information
                snprintf(buf, sizeof(buf), "AP IP: " IPSTR, IP2STR(&ip_info.ip));
                lv_label_set_text(admin_client_ip, buf);
                lv_obj_set_hidden(admin_client_ip, false);
                ESP_LOGI("UI", "Set admin_client_ip text to: %s", buf);
                
                snprintf(buf, sizeof(buf), "\nConnect to\nhttp://" IPSTR, IP2STR(&ip_info.gw));
                lv_label_set_text(admin_gateway_ip, buf);
                lv_obj_set_hidden(admin_gateway_ip, false);
                ESP_LOGI("UI", "Set admin_gateway_ip text to: %s", buf);
                ESP_LOGI("UI", "Successfully displayed AP IP info");
                return;
            }
        }
    }
    
    // Get STA interface and show STA IP when connected as station
    if (sta_connected) {
        esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        ESP_LOGI("UI", "STA netif handle (WIFI_STA_DEF): %p", sta_netif);
        
        if (sta_netif) {
            esp_netif_ip_info_t ip_info;
            esp_err_t ret = esp_netif_get_ip_info(sta_netif, &ip_info);
            ESP_LOGI("UI", "STA esp_netif_get_ip_info returned: %s", esp_err_to_name(ret));
            ESP_LOGI("UI", "STA IP: " IPSTR, IP2STR(&ip_info.ip));
            ESP_LOGI("UI", "STA Gateway: " IPSTR, IP2STR(&ip_info.gw));
            ESP_LOGI("UI", "STA Netmask: " IPSTR, IP2STR(&ip_info.netmask));
            
            if (ret == ESP_OK && ip_info.ip.addr != 0) {
                ESP_LOGI("UI", "STA IP is valid, updating UI labels...");
                snprintf(buf, sizeof(buf), "Client IP: " IPSTR, IP2STR(&ip_info.ip));
                lv_label_set_text(admin_client_ip, buf);
                lv_obj_set_hidden(admin_client_ip, false);
                ESP_LOGI("UI", "Set admin_client_ip text to: %s", buf);
                
                snprintf(buf, sizeof(buf), "Gateway: " IPSTR, IP2STR(&ip_info.gw));
                lv_label_set_text(admin_gateway_ip, buf);
                lv_obj_set_hidden(admin_gateway_ip, false);
                ESP_LOGI("UI", "Set admin_gateway_ip text to: %s", buf);
                ESP_LOGI("UI", "Successfully displayed STA IP info");
                return;
            } else {
                ESP_LOGW("UI", "STA IP is not valid or error occurred. ret=%s, ip.addr=0x%08x", 
                         esp_err_to_name(ret), ip_info.ip.addr);
            }
        } else {
            ESP_LOGW("UI", "Could not get STA netif handle");
        }
    }
    
    // If we reach here, we couldn't get IP info through normal methods
    // Try iterating through all interfaces as fallback
    ESP_LOGI("UI", "Primary methods failed, trying to iterate through all interfaces...");
    
    esp_netif_t *netif = NULL;
    esp_netif_t *temp_netif = esp_netif_next(netif);
    bool ip_found = false;
    
    while (temp_netif != NULL && !ip_found) {
        esp_netif_ip_info_t ip_info;
        esp_err_t ret = esp_netif_get_ip_info(temp_netif, &ip_info);
        
        if (ret == ESP_OK && ip_info.ip.addr != 0) {
            const char* desc = esp_netif_get_desc(temp_netif);
            ESP_LOGI("UI", "Found valid IP on interface %s: " IPSTR, 
                     desc ? desc : "unknown", IP2STR(&ip_info.ip));
            
            // Use interface description to determine type instead of IP range heuristic
            if (desc && strstr(desc, "ap")) {
                // AP interface
                snprintf(buf, sizeof(buf), "AP IP: " IPSTR, IP2STR(&ip_info.ip));
                lv_label_set_text(admin_client_ip, buf);
                lv_obj_set_hidden(admin_client_ip, false);
                
                snprintf(buf, sizeof(buf), "\nConnect to\nhttp://" IPSTR, IP2STR(&ip_info.gw));
                lv_label_set_text(admin_gateway_ip, buf);
                lv_obj_set_hidden(admin_gateway_ip, false);
                ip_found = true;
            } else if (desc && strstr(desc, "sta")) {
                // STA interface
                snprintf(buf, sizeof(buf), "Client IP: " IPSTR, IP2STR(&ip_info.ip));
                lv_label_set_text(admin_client_ip, buf);
                lv_obj_set_hidden(admin_client_ip, false);
                
                snprintf(buf, sizeof(buf), "\nConnect to\nhttp://" IPSTR, IP2STR(&ip_info.gw));
                lv_label_set_text(admin_gateway_ip, buf);
                lv_obj_set_hidden(admin_gateway_ip, false);
                ip_found = true;
            }
            ESP_LOGI("UI", "Successfully displayed IP info from interface iteration");
        }
        
        temp_netif = esp_netif_next(temp_netif);
    }
    
    if (!ip_found) {
        ESP_LOGW("UI", "No valid IP information found to display");
        lv_obj_set_hidden(admin_client_ip, true);
        lv_obj_set_hidden(admin_gateway_ip, true);
    }
    
    ESP_LOGI("UI", "=== END IP INFO DEBUG ===");
}


static void ui_init(void)
{
    ui_screen_splash_init();

    ui_screen_person_init();

    ui_screen_socialenergy_init();

    ui_screen_event_init();

    ui_screen_radar_init();
    
    ui_screen_rssi_init();

    ui_screen_admin_init();

    ui_screen_snake_init();
    
    radar_task_handle = lv_task_create(ui_radar_task, 2000, LV_TASK_PRIO_OFF, NULL);
    rssi_task_handle = lv_task_create(ui_rssi_task, 2000, LV_TASK_PRIO_OFF, NULL);
    snake_task_handle = lv_task_create(snake_task, 50, LV_TASK_PRIO_OFF, NULL);

    // show first page.
    lv_scr_load(screens[current_screen]);

    // Turn on backlight and run backlight management task
    ui_update_backlight(true);
    backlight_task_handle = lv_task_create(ui_backlight_task, 1000, LV_TASK_PRIO_LOW, NULL);

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_START, &ui_ap_start_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STOP, &ui_ap_stop_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ui_sta_connected_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &ui_sta_disconnected_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_STOP, &ui_sta_stop_handler, NULL));
}

static void ui_tick_task(void *arg)
{
    lv_tick_inc(1);
}

void ui_task(void *arg)
{
    SemaphoreHandle_t xGuiSemaphore;
    xGuiSemaphore = xSemaphoreCreateMutex();

    lv_init();
    lvgl_driver_init();

    lv_color_t *buf1 = (lv_color_t*) heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_color_t *buf2 = (lv_color_t*) heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);

    static lv_disp_buf_t disp_buf;
    uint32_t size_in_px = DISP_BUF_SIZE;

    lv_disp_buf_init(&disp_buf, buf1, buf2, size_in_px);
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &ui_tick_task,
        .name = "ui_tick_task",
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 1000));

    ui_init();

    while (1)
    {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));

        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
        {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
        }
    }

    free(buf1);
    free(buf2);
    vTaskDelete(NULL);
}

void ui_switch_page_down()
{
    ui_update_backlight(true);

    current_screen++;
    current_screen %= NUM_SCREENS;
    ESP_LOGI("DISPLAY", "DISPLAY COUNTER: %d/%d", current_screen+1, NUM_SCREENS);

    lv_scr_load_anim(screens[current_screen], LV_SCR_LOAD_ANIM_OVER_TOP, 300, 0, false);

    restore_current_task();
}

void ui_switch_page_up()
{
    ui_update_backlight(true);

    current_screen--;
    current_screen = (NUM_SCREENS + (current_screen % NUM_SCREENS)) % NUM_SCREENS;
    ESP_LOGI("DISPLAY", "DISPLAY COUNTER: %d/%d", current_screen+1, NUM_SCREENS);
    
    lv_scr_load_anim(screens[current_screen], LV_SCR_LOAD_ANIM_OVER_BOTTOM, 300, 0, false);

    restore_current_task();
}

void button_task(void *arg)
{
    static button_event_t curr_ev;
    static button_event_t prev_ev[2];
    static QueueHandle_t button_events;
    button_events = button_init(PIN_BIT(BUTTON_1) | PIN_BIT(BUTTON_2));

    while (true)
    {
        if (xQueueReceive(button_events, &curr_ev, 1000 / portTICK_PERIOD_MS))
        {
            uint8_t btn_id = curr_ev.pin - 0x08;
            if (curr_ev.event == BUTTON_HELD)
            {
                set_screen_led_backlight(badge_obj.brightness_mid);
            }
            if (curr_ev.pin == BUTTON_1) // DOWN button event
            {
                if ((prev_ev[btn_id].event == BUTTON_HELD) && (curr_ev.event == BUTTON_UP))
                {
                    ui_switch_page_down();
                }
                else if ((prev_ev[btn_id].event == BUTTON_DOWN) && (curr_ev.event == BUTTON_UP))
                {
                    ui_button_down();
                }
            }

            if (curr_ev.pin == BUTTON_2) // UP button event
            {
                if ((prev_ev[btn_id].event == BUTTON_HELD) && (curr_ev.event == BUTTON_UP))
                {
                    ui_switch_page_up();
                }
                else if ((prev_ev[btn_id].event == BUTTON_DOWN) && (curr_ev.event == BUTTON_UP))
                {
                    ui_button_up();
                }
            }
            prev_ev[btn_id] = curr_ev;
        }
    }
}

void ui_list_all_netifs() {
    ESP_LOGI("UI", "=== LISTING ALL NETWORK INTERFACES ===");
    
    // Try to iterate through all available network interfaces
    esp_netif_t *netif = NULL;
    esp_netif_t *temp_netif = esp_netif_next(netif);
    int count = 0;
    
    while (temp_netif != NULL) {
        count++;
        ESP_LOGI("UI", "Found netif %d: %p", count, temp_netif);
        
        // Get interface description
        const char* desc = esp_netif_get_desc(temp_netif);
        ESP_LOGI("UI", "Interface %d description: %s", count, desc ? desc : "unknown");
        
        // Get IP info for this interface
        esp_netif_ip_info_t ip_info;
        esp_err_t ret = esp_netif_get_ip_info(temp_netif, &ip_info);
        ESP_LOGI("UI", "Interface %d IP info (ret: %s):", count, esp_err_to_name(ret));
        ESP_LOGI("UI", "  IP: " IPSTR, IP2STR(&ip_info.ip));
        ESP_LOGI("UI", "  Gateway: " IPSTR, IP2STR(&ip_info.gw));
        ESP_LOGI("UI", "  Netmask: " IPSTR, IP2STR(&ip_info.netmask));
        
        temp_netif = esp_netif_next(temp_netif);
    }
    
    ESP_LOGI("UI", "Total network interfaces found: %d", count);
    ESP_LOGI("UI", "=== END NETIF LISTING ===");
}

void ui_manual_ip_update() {
    ESP_LOGI("UI", "Manual IP update triggered from admin screen");
    ui_update_ip_info();
}

void ui_force_show_ip_labels() {
    ESP_LOGI("UI", "Force showing IP labels for testing - calling ui_update_ip_info instead");
    ui_update_ip_info();
}
