#include "ui.h"
#include "led.h"

enum screen_order {SCREEN_LOGO, SCREEN_EVENT, SCREEN_RADAR, SCREEN_RSSI,  SCREEN_ADMIN, SCREEN_SNAKE, SCREEN_INVADERS, NUM_SCREENS};
static lv_obj_t* screens[NUM_SCREENS];
static int8_t current_screen = SCREEN_LOGO;

static uint32_t last_trigger = -1;

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

// Event schedule pagination. The schedule can hold many entries; the display
// loads only one page at a time (streamed from the file) to bound memory.
#define SCHEDULE_MAX_ROWS  400   // hard safety cap on entries scanned
#define EVENT_ROW_BUF      768   // max bytes of one schedule row object

// Sorted display order. The schedule file is stored in feed order, so we build
// a compact index (per entry: its "sort" key YYYYMMDDHHMM and its rank in
// day-then-time order), then stream-fill the table with every entry placed at
// its sorted rank. Bounded RAM (no whole-file DOM); the whole list is one
// scrollable table.
// Allocated (SCHEDULE_MAX_ROWS entries) only while the schedule is loaded, and
// freed together with the table on ui_event_unload() so AP/STA get the heap.
static uint64_t *sched_keys = NULL;   // key per file-order index
static uint16_t *sched_rank = NULL;   // file-order index -> sorted rank
static int      sched_total = 0;
static bool     sched_index_valid = false;
static bool     event_loaded = false;            // table built lazily on first view

static int cmp_sched_order(const void* a, const void* b) {
    uint64_t ka = sched_keys[*(const uint16_t*)a];
    uint64_t kb = sched_keys[*(const uint16_t*)b];
    return (ka > kb) - (ka < kb);
}

// Forward declarations
void ui_update_ip_info(void);
void ui_list_all_netifs(void);
static void ui_event_unload(void);

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
    
    // Check if we've reached 7 presses
    if (up_button_press_counter == 7) {
        // printf("DEBUG: UP button pressed 7 times on screen %s (index: %d)\n", current_screen);
        
        // Call set_completed() function when on SCREEN_LOGO (index 0)
        if (current_screen == SCREEN_LOGO) {
            ESP_LOGI("UI", "Summoning sequence activated!");
            set_completed();
        }        
        
        // Reset the counter after reaching 7
        up_button_press_counter = 0;
    }

    // Check if the backlight update is needed
    if(ui_update_backlight(true)){
        return;
    }

    switch(current_screen){
        case SCREEN_SNAKE:
            lv_task_set_prio(snake_task_handle, LV_TASK_PRIO_LOW);
            snake_set_dir(1);
            break;
        case SCREEN_INVADERS:
            invaders_move_cannon(-1);   // left wheel -> move cannon left
            break;
        case SCREEN_ADMIN:
            switch(admin_state){
                case ADMIN_STATE_OFF: // AP and STA disabled: enable AP
                    ui_event_unload();   // free schedule table -> heap for AP + web server
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
    
    // Check if we've reached 7 presses
    if (down_button_press_counter == 7) {
        // Call rainbow() function when on SCREEN_LOGO (index 0)
        if (current_screen == SCREEN_LOGO) {
            ESP_LOGI("UI", "Rainbow sequence activated!");
            rainbow();
        }        
        
        // Reset the counter after reaching 7
        down_button_press_counter = 0;
    }

    if(ui_update_backlight(true)){
        return;
    }

    switch(current_screen){
        case SCREEN_SNAKE:
            lv_task_set_prio(snake_task_handle, LV_TASK_PRIO_LOW);
            snake_set_dir(-1);
            break;
        case SCREEN_INVADERS:
            invaders_move_cannon(1);    // right wheel -> move cannon right
            break;
        case SCREEN_ADMIN:
            switch(admin_state){
                case ADMIN_STATE_OFF: // AP and STA disabled: enable STA
                    ui_event_unload();   // free schedule table -> heap for TLS sync
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

// Set a table cell WITHOUT triggering a relayout (unlike lv_table_set_cell_value,
// which calls refr_size on every call -> O(n^2) when filling a large table). The
// caller must trigger a single relayout afterwards (lv_table_set_row_cnt). The
// format byte 0 == left-aligned, no merge/crop, matching the default cells.
static void event_set_cell(int row, int col, const char* txt)
{
    lv_table_ext_t *ext = lv_obj_get_ext_attr(table_event);
    uint32_t cell = (uint32_t)row * ext->col_cnt + col;
    char *p = lv_mem_realloc(ext->cell_data[cell], strlen(txt) + 2);  // +format byte +'\0'
    if (!p) return;
    p[0] = 0;
    strcpy(p + 1, txt);
    ext->cell_data[cell] = p;
}

// Fill one 2-cell table row from a parsed schedule item (NULL-safe).
static void event_fill_row(cJSON* item, int row)
{
    char buff[256];
    cJSON *title    = cJSON_GetObjectItem(item, "title");
    cJSON *day      = cJSON_GetObjectItem(item, "day");
    cJSON *hour     = cJSON_GetObjectItem(item, "hour");
    cJSON *speaker  = cJSON_GetObjectItem(item, "speaker");
    cJSON *location = cJSON_GetObjectItem(item, "location");
    cJSON *duration = cJSON_GetObjectItem(item, "duration");

    buff[0] = '\0';
    if (cJSON_IsString(day) && strlen(day->valuestring) > 0)
        snprintf(buff, sizeof(buff), "Day: %s\n", day->valuestring);
    if (cJSON_IsString(hour) && strlen(hour->valuestring) > 0)
        snprintf(buff + strlen(buff), sizeof(buff) - strlen(buff), "Time: %s\n", hour->valuestring);
    if (cJSON_IsString(location) && strlen(location->valuestring) > 0)
        snprintf(buff + strlen(buff), sizeof(buff) - strlen(buff), "Where: %s\n", location->valuestring);
    if (cJSON_IsString(duration) && strlen(duration->valuestring) > 0)
        snprintf(buff + strlen(buff), sizeof(buff) - strlen(buff), "How long: %s", duration->valuestring);
    event_set_cell(row, 0, buff);

    snprintf(buff, sizeof(buff), "%s\nby %s",
             cJSON_IsString(title)   ? title->valuestring   : "",
             cJSON_IsString(speaker) ? speaker->valuestring : "");
    event_set_cell(row, 1, buff);
}

// Open the schedule file and advance past the '[' that starts the schedule
// array (the "info" field is base64, so it contains no '['). Returns NULL on
// error or if no array is present.
static FILE* schedule_open_array(void)
{
    FILE* fp = fopen(SCHEDULE_FILE, "r");
    if (!fp) { ESP_LOGI(__FILE__, "Failed to open schedule file"); return NULL; }
    int c;
    while ((c = fgetc(fp)) != EOF && c != '[') { }
    if (c == EOF) { fclose(fp); ESP_LOGI(__FILE__, "No schedule array found"); return NULL; }
    return fp;
}

// Build the sorted display index: scan the file once, read each entry's "sort"
// key, and rank entries in ascending (day-then-time) order. Cached until the
// schedule is replaced (ui_schedule_reset).
static void ui_schedule_build_index(void)
{
    sched_total = 0;
    if (!sched_keys) sched_keys = malloc(SCHEDULE_MAX_ROWS * sizeof(*sched_keys));
    if (!sched_rank) sched_rank = malloc(SCHEDULE_MAX_ROWS * sizeof(*sched_rank));
    if (!sched_keys || !sched_rank) {
        ESP_LOGE(__FILE__, "Cannot allocate schedule sort index");
        sched_index_valid = true;   // avoid retry loop; renders as empty
        return;
    }
    FILE* fp = schedule_open_array();
    if (!fp) { sched_index_valid = true; return; }

    int c, blen = 0, depth = 0;
    bool overflow = false, in_str = false, esc = false, in_obj = false;
    char buf[EVENT_ROW_BUF];

    while ((c = fgetc(fp)) != EOF) {
        char ch = (char)c;
        if (!in_obj) {
            if (ch == '{') { in_obj = true; depth = 1; blen = 0; overflow = false;
                             in_str = false; esc = false; buf[blen++] = '{'; }
            else if (ch == ']') break;
            continue;
        }
        if (!overflow) { if (blen < EVENT_ROW_BUF - 1) buf[blen++] = ch; else overflow = true; }
        if (in_str) {
            if (esc) esc = false; else if (ch == '\\') esc = true; else if (ch == '"') in_str = false;
        } else if (ch == '"') { in_str = true; }
        else if (ch == '{' || ch == '[') { depth++; }
        else if (ch == '}' || ch == ']') {
            if (--depth == 0) {
                in_obj = false;
                uint64_t key = 0;
                if (!overflow) {
                    buf[blen] = '\0';
                    // "sort" is the first field of each row; read it textually
                    // to avoid a full cJSON parse per entry in this index pass.
                    char* p = strstr(buf, "\"sort\":\"");
                    if (p) key = strtoull(p + 8, NULL, 10);
                }
                if (sched_total < SCHEDULE_MAX_ROWS) sched_keys[sched_total++] = key;
                else break;
            }
        }
    }
    fclose(fp);

    // Sort a temporary index array by key, then invert to file-order -> rank.
    uint16_t *order = malloc(sched_total * sizeof(*order));
    if (order) {
        for (int i = 0; i < sched_total; i++) order[i] = (uint16_t)i;
        qsort(order, sched_total, sizeof(*order), cmp_sched_order);
        for (int i = 0; i < sched_total; i++) sched_rank[order[i]] = (uint16_t)i;
        free(order);
    } else {
        for (int i = 0; i < sched_total; i++) sched_rank[i] = (uint16_t)i;  // unsorted fallback
    }

    sched_index_valid = true;
    ESP_LOGI(__FILE__, "Schedule index built: %d entries", sched_total);
}

// Invalidate the cached sort index and mark the table stale so it is rebuilt
// the next time the event screen is opened. Called after a sync replaces the
// file (cheap; the actual rebuild is deferred off the sync path).
void ui_schedule_reset(void)
{
    sched_index_valid = false;
    event_loaded = false;
}

// Free the schedule table's cells to reclaim ~50 KB of heap for Wi-Fi (AP web
// server / TLS sync). The table is rebuilt lazily the next time the event
// screen is opened (ui_prepare_current_screen). Only called while the event
// screen is inactive (from the admin screen), so freeing it is safe.
static void ui_event_unload(void)
{
    if (!event_loaded) return;

    // Free the per-cell strings directly. lv_table_set_row_cnt(t,0) frees only
    // the pointer array, not the strings (LVGL v7) -> leaks ~30 KB otherwise.
    // We must NOT clear cells via lv_table_set_cell_value("") because each call
    // triggers a full table relayout (re-measuring every row) -> ~O(n^2), which
    // froze the UI for ~15 s on a 142-row schedule. Freeing cell_data directly
    // is a single O(n) pass with no relayout.
    lv_table_ext_t *ext = lv_obj_get_ext_attr(table_event);
    if (ext && ext->cell_data) {
        uint32_t n = (uint32_t)ext->row_cnt * ext->col_cnt;
        for (uint32_t i = 0; i < n; i++) {
            if (ext->cell_data[i]) { lv_mem_free(ext->cell_data[i]); ext->cell_data[i] = NULL; }
        }
    }
    lv_table_set_row_cnt(table_event, 0);   // frees the pointer array (one relayout)

    // Also free the sort index; it is rebuilt with the table on next view.
    free(sched_keys); sched_keys = NULL;
    free(sched_rank); sched_rank = NULL;
    sched_index_valid = false;

    event_loaded = false;
    ESP_LOGI(__FILE__, "Freed event table; free_heap=%lu", esp_get_free_heap_size());
}

// Load the whole schedule into table_event in sorted (day-then-time) order, as
// one scrollable list. Streams the file and parses one entry at a time (no
// whole-file DOM), placing each at its sorted rank.
void ui_event_load()
{
    if (!sched_index_valid) ui_schedule_build_index();
    event_loaded = true;
    const int total = sched_total;

    lv_table_set_row_cnt(table_event, total > 0 ? total : 1);
    if (total == 0) {
        lv_table_set_cell_value(table_event, 0, 0, "No schedule");
        lv_table_set_cell_value(table_event, 0, 1, "");
        return;
    }

    FILE* fp = schedule_open_array();
    if (!fp) return;

    int c, obj_index = 0, filled = 0, blen = 0, depth = 0;
    bool overflow = false, in_str = false, esc = false, in_obj = false;
    char buf[EVENT_ROW_BUF];

    while ((c = fgetc(fp)) != EOF) {
        char ch = (char)c;
        if (!in_obj) {
            if (ch == '{') { in_obj = true; depth = 1; blen = 0; overflow = false;
                             in_str = false; esc = false; buf[blen++] = '{'; }
            else if (ch == ']') break;
            continue;
        }
        if (!overflow) { if (blen < EVENT_ROW_BUF - 1) buf[blen++] = ch; else overflow = true; }
        if (in_str) {
            if (esc) esc = false; else if (ch == '\\') esc = true; else if (ch == '"') in_str = false;
        } else if (ch == '"') { in_str = true; }
        else if (ch == '{' || ch == '[') { depth++; }
        else if (ch == '}' || ch == ']') {
            if (--depth == 0) {
                in_obj = false;
                int rank = (obj_index < SCHEDULE_MAX_ROWS) ? sched_rank[obj_index] : -1;
                if (!overflow && rank >= 0 && rank < total) {
                    buf[blen] = '\0';
                    cJSON* item = cJSON_Parse(buf);
                    if (item) { event_fill_row(item, rank); cJSON_Delete(item); filled++; }
                }
                if (++obj_index >= SCHEDULE_MAX_ROWS) break;
            }
        }
    }
    fclose(fp);

    // Cells were filled directly (no per-cell relayout); trigger ONE relayout
    // now to compute row heights over the full table (no-op realloc, same count).
    lv_table_set_row_cnt(table_event, total);

    ESP_LOGI(__FILE__, "Schedule loaded: %d/%d rows (sorted)", filled, total);
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

    // Load the full schedule now, during init (before the logo is shown), so
    // the badge does its heavy loading up front and the UI is responsive after.
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
    lv_style_set_bg_color(&style, LV_STATE_DEFAULT, LV_COLOR_MAKE(0xff, 0xff, 0xff));
    lv_obj_add_style(logo, LV_OBJ_PART_MAIN, &style);

    screens[SCREEN_LOGO] = screen_logo;
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

void ui_screen_invaders_init(){
    // page for space invaders (black "space" background)
    screen_invaders = lv_obj_create(NULL, NULL);
    lv_obj_set_style_local_bg_color(screen_invaders, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_obj_set_style_local_bg_opa(screen_invaders, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
    invaders_reset(screen_invaders);

    screens[SCREEN_INVADERS] = screen_invaders;
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

    ui_screen_event_init();

    ui_screen_radar_init();
    
    ui_screen_rssi_init();

    ui_screen_admin_init();

    ui_screen_snake_init();

    ui_screen_invaders_init();

    radar_task_handle = lv_task_create(ui_radar_task, 2000, LV_TASK_PRIO_OFF, NULL);
    rssi_task_handle = lv_task_create(ui_rssi_task, 2000, LV_TASK_PRIO_OFF, NULL);
    snake_task_handle = lv_task_create(snake_task, 50, LV_TASK_PRIO_OFF, NULL);
    invaders_task_handle = lv_task_create(invaders_task, INVADERS_TICK_MS, LV_TASK_PRIO_OFF, NULL);

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

// Rebuild the event table if it was marked stale (e.g. after a sync). The
// schedule is loaded eagerly at boot, so this is normally a no-op. Also start a
// fresh Space Invaders game (and enable its task) on entry, and stop the game
// task when leaving that screen.
static void ui_prepare_current_screen(void)
{
    if (current_screen == SCREEN_EVENT && !event_loaded) ui_event_load();

    if (current_screen == SCREEN_INVADERS) {
        invaders_reset(screen_invaders);   // fresh game; also enables the task
    } else if (invaders_task_handle) {
        lv_task_set_prio(invaders_task_handle, LV_TASK_PRIO_OFF);
    }
}

void ui_switch_page_down()
{
    ui_update_backlight(true);

    current_screen++;
    current_screen %= NUM_SCREENS;
    ESP_LOGI("DISPLAY", "DISPLAY COUNTER: %d/%d", current_screen+1, NUM_SCREENS);

    ui_prepare_current_screen();
    lv_scr_load_anim(screens[current_screen], LV_SCR_LOAD_ANIM_OVER_TOP, 300, 0, false);

    restore_current_task();
}

void ui_switch_page_up()
{
    ui_update_backlight(true);

    current_screen--;
    current_screen = (NUM_SCREENS + (current_screen % NUM_SCREENS)) % NUM_SCREENS;
    ESP_LOGI("DISPLAY", "DISPLAY COUNTER: %d/%d", current_screen+1, NUM_SCREENS);

    ui_prepare_current_screen();
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
