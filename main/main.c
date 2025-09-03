#include <stdio.h>
#include <string.h>

#include "badge/badge.h"

void app_main() 
{
    usleep(10 * 1000000UL);
    ESP_LOGI(__FILE__, "MAIN START: free_heap_size = %lu\n", esp_get_free_heap_size());

    badge_init();
    led_init();
    
    // start bluetooth
    bt_init();
    xTaskCreate(bt_task, "bt_task", 4096, NULL, 6, NULL);

    // start wifi management
    //wifi_init();
    //xTaskCreate(wifi_task, "wifi_task", 4096, NULL, 5, NULL);

    // start ui to core 1.
    xTaskCreate(ui_task, "ui_task", 8192, NULL, 0, NULL);

    xTaskCreate(led_task, "led_task", 2048, NULL, 10, NULL);

    xTaskCreate(button_task, "button_task", 4096, NULL, 1, NULL);

    // Handle HTTP webserver start/stop
    static httpd_handle_t server = NULL;
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &disconnect_handler, &server));

    ESP_LOGI(__FILE__, "MAIN END: free_heap_size = %lu\n", esp_get_free_heap_size());
}

