#include "led.h"
#include "color.h"
#include "hsv.h"
#include "esp_random.h"

static const char *TAG = "strip_ws2812";

static const uint8_t led_order[] = {2, 6, 1, 0, 4, 5, 3}; // 0: center, 6: top
static led_strip_t *strip;
static bool easter_egg_active = false; // Flag to block LED flashing during easter eggs
static bool rainbow_loop_active = false; // Continuous rainbow animation, driven by led_task

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
    i2c_register_write(0x5a, 0x11, 0x01);
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
    set_easter_egg_active(true); // Block LED flashing
    
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
    
    set_easter_egg_active(false); // Re-enable LED flashing
}

void rainbow() {
    set_easter_egg_active(true); // Block LED flashing
    
    // Define rainbow colors using HSV for better color representation
    uint8_t rainbow_hues[7] = {
        HUE_RED,     // LED 0 (center) - Red
        HUE_ORANGE,  // LED 1 - Orange  
        HUE_YELLOW,  // LED 2 - Yellow
        HUE_GREEN,   // LED 3 - Green
        HUE_AQUA,    // LED 4 - Aqua/Cyan
        HUE_BLUE,    // LED 5 - Blue
        HUE_PURPLE   // LED 6 (top) - Purple
    };
    
    ESP_LOGI(__FILE__, "Starting rainbow sequence");
    
    // Light up each LED one at a time with rainbow colors
    for(int i = 0; i < 7; i++) {
        // Create HSV color with full saturation and brightness
        hsv_t hsv_color = {
            .hue = rainbow_hues[i],
            .saturation = 255,
            .value = 200  // Slightly dimmed for better visibility
        };
        
        // Convert HSV to RGB
        rgb_t rgb_color = hsv2rgb_rainbow(hsv_color);
        
        // Light up the current LED
        led_rgb_color(i, rgb_color);
        
        ESP_LOGI(__FILE__, "Rainbow LED %d lit with hue %d", i, rainbow_hues[i]);
        
        // Wait before lighting the next LED
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
    
    // Keep the rainbow on for a moment
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    // Turn off all LEDs one by one
    for(int i = 0; i < 7; i++) {
        led_rgb_off(i);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
    
    ESP_LOGI(__FILE__, "Rainbow sequence completed");
    
    set_easter_egg_active(false); // Re-enable LED flashing
}

// ---------------------------------------------------------------------------
// Rainbow-screen animations. Short-pressing either wheel while on the Rainbow
// screen cycles to the next one (rainbow_next_animation(), wrapping from the
// last back to the first); ui.c reads rainbow_current_animation_name() to
// update the on-screen label. Every step() below does at most a handful of
// LED writes per call and paces itself with its own tick counter -- driven
// from led_task's own loop at a fixed base cadence (RAINBOW_TICK_MS), so
// nothing here ever blocks the UI, unlike the old looped-from-the-UI-task bug.
// ---------------------------------------------------------------------------

#define RAINBOW_TICK_MS 60   // base cadence led_task drives every animation at

// ---- 1. Rainbow Chase: light one LED at a time (classic ROYGBIV hues) until
// all 7 are lit, hold them together, turn them all off, then restart.
typedef enum { CHASE_LIGHTING, CHASE_HOLD, CHASE_OFF } chase_phase_t;
#define CHASE_STEP_TICKS 5   // ~300ms per LED light-up
#define CHASE_HOLD_TICKS 25  // ~1.5s with all 7 lit together
#define CHASE_OFF_TICKS  10  // ~0.6s all off before restarting

static chase_phase_t chase_phase;
static uint8_t chase_lit;
static uint8_t chase_wait;

static void chase_reset(void) { chase_phase = CHASE_LIGHTING; chase_lit = 0; chase_wait = 0; }

static void chase_step(void) {
    static const uint8_t hues[NUM_LEDS] = {
        HUE_RED, HUE_ORANGE, HUE_YELLOW, HUE_GREEN, HUE_AQUA, HUE_BLUE, HUE_PURPLE
    };
    switch (chase_phase) {
        case CHASE_LIGHTING:
            if (++chase_wait < CHASE_STEP_TICKS) return;
            chase_wait = 0;
            {
                hsv_t hsv_color = { .hue = hues[chase_lit], .saturation = 255, .value = 220 };
                led_rgb_color(chase_lit, hsv2rgb_rainbow(hsv_color));
            }
            if (++chase_lit >= NUM_LEDS) chase_phase = CHASE_HOLD;
            break;
        case CHASE_HOLD:
            if (++chase_wait < CHASE_HOLD_TICKS) return;
            chase_wait = 0;
            for (int i = 0; i < NUM_LEDS; i++) led_rgb_off(i);
            chase_phase = CHASE_OFF;
            break;
        case CHASE_OFF:
            if (++chase_wait < CHASE_OFF_TICKS) return;
            chase_wait = 0;
            chase_lit = 0;
            chase_phase = CHASE_LIGHTING;
            break;
    }
}

// ---- 2. Color Wave: all 7 LEDs lit continuously, hues flowing smoothly
// around the ring over time (a rotating rainbow, not a static one).
#define WAVE_STEP_TICKS 2   // ~120ms between shifts -- clearly visible motion
#define WAVE_HUE_STEP   10

static uint8_t wave_phase;
static uint8_t wave_wait;

static void wave_reset(void) { wave_phase = 0; wave_wait = 0; }

static void wave_step(void) {
    if (++wave_wait < WAVE_STEP_TICKS) return;
    wave_wait = 0;
    for (int i = 0; i < NUM_LEDS; i++) {
        hsv_t hsv_color = { .hue = (uint8_t)(wave_phase + i * (256 / NUM_LEDS)), .saturation = 255, .value = 220 };
        led_rgb_color(i, hsv2rgb_rainbow(hsv_color));
    }
    wave_phase += WAVE_HUE_STEP;
}

// ---- 3. Strobe Party: all 7 LEDs flash together, cycling to a new hue each
// flash. Kept to a moderate ~2Hz rate (not a rapid strobe) since this is worn
// at night around other people.
#define STROBE_ON_TICKS  4  // ~240ms on
#define STROBE_OFF_TICKS 4  // ~240ms off
#define STROBE_HUE_STEP  40

static bool strobe_on;
static uint8_t strobe_hue;
static uint8_t strobe_wait;

static void strobe_reset(void) { strobe_on = false; strobe_hue = 0; strobe_wait = 0; }

static void strobe_step(void) {
    uint8_t limit = strobe_on ? STROBE_ON_TICKS : STROBE_OFF_TICKS;
    if (++strobe_wait < limit) return;
    strobe_wait = 0;
    strobe_on = !strobe_on;
    if (strobe_on) {
        hsv_t hsv_color = { .hue = strobe_hue, .saturation = 255, .value = 255 };
        rgb_t rgb_color = hsv2rgb_rainbow(hsv_color);
        for (int i = 0; i < NUM_LEDS; i++) led_rgb_color(i, rgb_color);
        strobe_hue += STROBE_HUE_STEP;
    } else {
        for (int i = 0; i < NUM_LEDS; i++) led_rgb_off(i);
    }
}

// ---- 4. Twinkle Sparkle: random LEDs flicker on in random vivid colors for a
// short random lifetime, then fade out -- fairy-light twinkle effect.
#define TWINKLE_STEP_TICKS   2   // update every ~120ms
#define TWINKLE_SPAWN_CHANCE 90  // out of 255, per idle LED per update

static uint8_t twinkle_life[NUM_LEDS];
static uint8_t twinkle_wait;

static void twinkle_reset(void) {
    for (int i = 0; i < NUM_LEDS; i++) twinkle_life[i] = 0;
    twinkle_wait = 0;
}

static void twinkle_step(void) {
    if (++twinkle_wait < TWINKLE_STEP_TICKS) return;
    twinkle_wait = 0;
    for (int i = 0; i < NUM_LEDS; i++) {
        if (twinkle_life[i] > 0) {
            if (--twinkle_life[i] == 0) led_rgb_off(i);
            continue;
        }
        if ((esp_random() & 0xff) < TWINKLE_SPAWN_CHANCE) {
            hsv_t hsv_color = { .hue = (uint8_t)(esp_random() & 0xff), .saturation = 255, .value = 255 };
            led_rgb_color(i, hsv2rgb_rainbow(hsv_color));
            twinkle_life[i] = 3 + (esp_random() % 6); // lit for ~360-960ms
        }
    }
}

// ---- 5. Comet Spin: one bright "comet head" orbits the ring with a fading
// two-LED trail behind it, hue drifting slowly lap over lap.
#define COMET_STEP_TICKS 2  // ~120ms per step -> one lap in ~840ms

static uint8_t comet_pos;
static uint8_t comet_hue;
static uint8_t comet_wait;

static void comet_reset(void) { comet_pos = 0; comet_hue = 0; comet_wait = 0; }

static void comet_step(void) {
    if (++comet_wait < COMET_STEP_TICKS) return;
    comet_wait = 0;
    hsv_t hsv_color = { .hue = comet_hue, .saturation = 255, .value = 255 };
    rgb_t head = hsv2rgb_rainbow(hsv_color);
    for (int i = 0; i < NUM_LEDS; i++) {
        int back = (comet_pos - i + NUM_LEDS) % NUM_LEDS; // distance behind the head
        if (back == 0) led_rgb_color(i, head);
        else if (back == 1) led_rgb_color(i, rgb_scale(head, 130));
        else if (back == 2) led_rgb_color(i, rgb_scale(head, 60));
        else led_rgb_off(i);
    }
    comet_pos = (comet_pos + 1) % NUM_LEDS;
    comet_hue += 5;
}

// ---- dispatch table ----
typedef struct {
    const char *name;
    void (*step)(void);
    void (*reset)(void);
} rainbow_anim_t;

static const rainbow_anim_t rainbow_anims[] = {
    { "Rainbow Chase",   chase_step,   chase_reset },
    { "Color Wave",      wave_step,    wave_reset },
    { "Strobe Party",    strobe_step,  strobe_reset },
    { "Twinkle Sparkle", twinkle_step, twinkle_reset },
    { "Comet Spin",      comet_step,   comet_reset },
};
#define NUM_RAINBOW_ANIMS (sizeof(rainbow_anims) / sizeof(rainbow_anims[0]))

static uint8_t rainbow_anim_index = 0;

static void rainbow_animate(void) {
    rainbow_anims[rainbow_anim_index].step();
}

void rainbow_next_animation(void) {
    rainbow_anim_index = (rainbow_anim_index + 1) % NUM_RAINBOW_ANIMS;
    for (int i = 0; i < NUM_LEDS; i++) led_rgb_off(i);   // clear before the next one starts painting
    rainbow_anims[rainbow_anim_index].reset();
    ESP_LOGI(__FILE__, "Rainbow animation -> %s", rainbow_anims[rainbow_anim_index].name);
}

const char* rainbow_current_animation_name(void) {
    return rainbow_anims[rainbow_anim_index].name;
}

void set_rainbow_loop_active(bool active) {
    rainbow_loop_active = active;
    if (active) {
        // always restart from the first animation when entering the screen
        rainbow_anim_index = 0;
        rainbow_anims[rainbow_anim_index].reset();
    } else {
        // Leaving the rainbow screen: clear the LEDs rather than leaving them
        // stuck on whatever colors the loop last painted.
        for (int i = 0; i < NUM_LEDS; i++) led_rgb_off(i);
    }
    ESP_LOGI(__FILE__, "Rainbow loop %s (%s)", active ? "STARTED" : "STOPPED",
             rainbow_anims[rainbow_anim_index].name);
}

void set_easter_egg_active(bool active) {
    easter_egg_active = active;
    ESP_LOGI(__FILE__, "Easter egg mode %s", active ? "ENABLED" : "DISABLED");
}

void led_task(void* arg)
{
    while(1){
        // Rainbow screen loop takes priority over everything else here. If
        // led_task is mid-flash() when this flips on, it won't take effect
        // until that call returns (flash() blocks for several seconds); from
        // the next loop iteration onward it preempts the normal LED behavior.
        if (rainbow_loop_active) {
            rainbow_animate();
            vTaskDelay(RAINBOW_TICK_MS / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());

        // Skip normal LED operations if easter egg is active
        if(easter_egg_active) {
            ESP_LOGI(__FILE__, "Easter egg active, skipping normal LED operations");
            vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait 1 second before checking again
            continue;
        }
        
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