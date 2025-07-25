#include "led.h"

static const char *TAG = "strip_ws2812";

static const uint8_t led_order[] = {2, 6, 1, 0, 4, 5, 3}; // 0: center, 6: top
static led_strip_t *strip;

static void i2c_register_write(uint8_t addr, uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data};
    i2c_master_write_to_device(0, addr, write_buf, sizeof(write_buf), 
                                   I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

static void i2c_master_init()
{
    int i2c_master_port = I2C_MASTER_NUM;
    
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
}

void led_init()
{
    i2c_master_init();
    // setup aw9523b led drivers.
    // chip 1, address = 0x5a; chip 2, address = 0x5b.
    i2c_register_write(0x5a, 0x11, 0x03);
    i2c_register_write(0x5a, 0x12, 0x80);
    i2c_register_write(0x5a, 0x13, 0x80);    

    strip = led_strip_init(LED_RMT_TX_CHANNEL, LED_RMT_TX_GPIO, NUM_LEDS);
}

void set_screen_led_backlight(uint8_t brigtness)
{
    i2c_register_write(0x5a, 0x20, brigtness);
    i2c_register_write(0x5a, 0x21, brigtness);
    i2c_register_write(0x5a, 0x22, brigtness);
    i2c_register_write(0x5a, 0x23, brigtness);
}

static void led_rgb_color(uint8_t id, rgb_t color){
    if (strip) {
        // Write RGB values to strip driver
        ESP_ERROR_CHECK(strip->set_pixel(strip, led_order[id], color.red, color.green, color.blue));
        // Flush RGB values to LEDs
        ESP_ERROR_CHECK(strip->refresh(strip, 100));
        vTaskDelay(10 / portTICK_PERIOD_MS);
    } else {
        ESP_LOGE(TAG, "Strip not initialized");
    }
}

static void led_rgb_off(uint8_t id){
    rgb_t rgb_color = rgb_from_code(0);
    led_rgb_color(id, rgb_color);
}

static void all_on(rgb_t color){
    for(int i=0; i<7; i++){
        led_rgb_color(i, color);
    }
}

static void all_off(){
    // Clear LED strip (turn off all LEDs)
    ESP_ERROR_CHECK(strip->clear(strip, 100));
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

static void set_leds_by_badge_id(rgb_t color){
    static bool round = false;
    ESP_LOGI(__FILE__, "set_leds_by_badge_id: device_id = %d", badge_obj.device_id);
    switch(badge_obj.device_id){
        case 1:
            led_rgb_color(0, color);
            break;
        case 2:
            if (round)
            {
                led_rgb_color(1, color);
                led_rgb_color(3, color);
            }
            else
            {
                led_rgb_color(2, color);
                led_rgb_color(5, color);
            }
            break;
        case 3:
            if (round)
            {
                led_rgb_color(2, color);
                led_rgb_color(3, color);
                led_rgb_color(6, color);
            }
            else
            {
                led_rgb_color(4, color);
                led_rgb_color(5, color);
                led_rgb_color(1, color);
            }
            break;
        case 4:
            led_rgb_color(1, color);
            led_rgb_color(2, color);
            led_rgb_color(3, color);
            led_rgb_color(5, color);
            break;
        case 5:
            led_rgb_color(0, color);
            led_rgb_color(1, color);
            led_rgb_color(2, color);
            led_rgb_color(3, color);
            led_rgb_color(5, color);
            break;
        case 6:
            led_rgb_color(1, color);
            led_rgb_color(2, color);
            led_rgb_color(3, color);
            led_rgb_color(4, color);
            led_rgb_color(5, color);
            led_rgb_color(6, color);
            break;
        case 7:
            all_on(color);
            break;
    }
    round = !round;
}

void flash(int period, uint8_t fade_factor) {
    rgb_t color = rgb_from_code(MAGENTA_SAIYAN);
    color = rgb_fade(color, fade_factor);
    ESP_LOGI(__FILE__, "Start flashing");
    set_leds_by_badge_id(color);
    vTaskDelay(300 / portTICK_PERIOD_MS);
    all_off();
    ESP_LOGI(__FILE__, "All LEDs off");
    vTaskDelay(period / portTICK_PERIOD_MS);
}

void set_completed(){
    rgb_t color = rgb_from_code(MAGENTA_SAIYAN);
    color = rgb_fade(color, 0xf0);
    for(int i=1; i<7;i++){
        led_rgb_color(i, color);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        all_off();
    }
    vTaskDelay(20 / portTICK_PERIOD_MS);
    all_on(color);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    for(int i=0; i<7;i++){
        led_rgb_off(i);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

void led_task(void* arg) 
{
    while(1){
        ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());
        bool nearby_set = check_ble_set();
        if(nearby_set)
        {
            ESP_LOGI(__FILE__, "Set found");
            set_completed();
        } else {
            uint8_t nearby_count = count_ble_nodes();
            if(nearby_count > 0)
            {
                ESP_LOGI(__FILE__, "Badges around: %d", nearby_count);
                flash(5000, 0xf0);
            } else {
                ESP_LOGI(__FILE__, "It is just me around");
                flash(10000, 0xfa);
            }
        }

    }
}