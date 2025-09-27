#ifndef _ADMIN_HC
#define _ADMIN_H

#include <stdio.h>
#include <stdlib.h>

#include "../badge.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "freertos/FreeRTOS.h"
#include <lvgl.h>

#define ADMIN_STATE_OFF 0
#define ADMIN_STATE_AP 1
#define ADMIN_STATE_STA 2

void ui_admin_update_info();
void ui_send_wifi_event(int event);
void ui_admin_ap_start_event();
void ui_admin_ap_stop_event();
void ui_admin_sta_connected_event();
void ui_admin_sta_disconnected_event();
void ui_admin_sta_stop_event();
void ui_admin_show_ip();
void ui_admin_force_show_ip_labels();
void ui_admin_connection_progress(uint8_t cur, uint8_t max);
void ui_clear_sync();
void ui_list_all_netifs();
void ui_update_ip_info();
lv_obj_t *ui_screen_admin_init();
void ui_admin_button_up();
void ui_admin_button_down();
#endif
