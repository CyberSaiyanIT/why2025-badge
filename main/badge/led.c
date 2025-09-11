#include "led.h"
#include "common/i2c.h"
#include "color.h"
#include "hsv.h"
#include "bt.h"

static const uint8_t led_order[] = {2, 6, 1, 0, 4, 5, 3};  // 0: center, 6: top
static led_strip_handle_t strip;
static bool easter_egg_active =
    false;  // Flag to block LED flashing during easter eggs

void led_init() {
  // setup aw9523b led drivers.
  // chip 1, address = 0x5a; chip 2, address = 0x5b.
  i2c_write_register(AW9523B_handle, 0x11, 0x01);
  i2c_write_register(AW9523B_handle, 0x12, 0x80);
  i2c_write_register(AW9523B_handle, 0x13, 0x80);

  led_strip_config_t strip_config = {
      .strip_gpio_num = LED_RMT_TX_GPIO,  // The GPIO that connected to the LED
                                          // strip's data line
      .max_leds = NUM_LEDS,               // The number of LEDs in the strip,
      .led_model =
          LED_MODEL_WS2812,  // LED strip model, it determines the bit timing
      .color_component_format =
          LED_STRIP_COLOR_COMPONENT_FMT_GRB,  // The color component format is
                                              // G-R-B
      .flags = {
          .invert_out = false,  // don't invert the output signal
      }};

  /// RMT backend specific configuration
  led_strip_rmt_config_t rmt_config = {
      .clk_src = RMT_CLK_SRC_DEFAULT,     // different clock source can lead to
                                          // different power consumption
      .resolution_hz = 10 * 1000 * 1000,  // RMT counter clock frequency: 10MHz
      .mem_block_symbols =
          64,  // the memory size of each RMT channel, in words (4 bytes)
      .flags = {
          .with_dma =
              false,  // DMA feature is available on chips like ESP32-S3/P4
      }};

  /// Create the LED strip object
  ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &strip));
}

static void led_rgb_color(uint8_t id, rgb_t color) {
  if (strip) {
    // Write RGB values to strip driver
    ESP_ERROR_CHECK(led_strip_set_pixel(strip, led_order[id], color.red,
                                        color.green, color.blue));
    ESP_ERROR_CHECK(led_strip_refresh(strip));
    // ESP_ERROR_CHECK(strip->set_pixel(strip, led_order[id], color.red,
    //                                color.green, color.blue));
    // Flush RGB values to LEDs
    // ESP_ERROR_CHECK(strip->refresh(strip, 100));
    vTaskDelay(10 / portTICK_PERIOD_MS);
  } else {
    ESP_LOGE(__FILE__, "Strip not initialized");
  }
}

static void led_rgb_off(uint8_t id) {
  rgb_t rgb_color = rgb_from_code(0);
  led_rgb_color(id, rgb_color);
}

static void led_all_on(rgb_t color) {
  for (int i = 0; i < 7; i++) {
    led_rgb_color(i, color);
  }
}

static void led_all_off() {
  // Clear LED strip (turn off all LEDs)
  ESP_ERROR_CHECK(led_strip_clear(strip));
  // ESP_ERROR_CHECK(strip->clear(strip, 100));
  vTaskDelay(10 / portTICK_PERIOD_MS);
}

static void led_set_by_badge_id(rgb_t color) {
  static bool round = false;
  ESP_LOGI(__FILE__, "set_leds_by_badge_id: device_id = %d",
           badge_obj.device_id);
  switch (badge_obj.device_id) {
    case 1:
      led_rgb_color(0, color);
      break;
    case 2:
      if (round) {
        led_rgb_color(1, color);
        led_rgb_color(3, color);
      } else {
        led_rgb_color(2, color);
        led_rgb_color(5, color);
      }
      break;
    case 3:
      if (round) {
        led_rgb_color(2, color);
        led_rgb_color(3, color);
        led_rgb_color(6, color);
      } else {
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
      led_all_on(color);
      break;
  }
  round = !round;
}

static void led_flash(int period, uint8_t fade_factor) {
  rgb_t color = rgb_from_code(MAGENTA_SAIYAN);
  color       = rgb_fade(color, fade_factor);
  ESP_LOGI(__FILE__, "Start flashing");
  led_set_by_badge_id(color);
  vTaskDelay(300 / portTICK_PERIOD_MS);
  led_all_off();
  ESP_LOGI(__FILE__, "All LEDs off");
  vTaskDelay(period / portTICK_PERIOD_MS);
}

void led_set_completed() {
  led_set_easter_egg_active(true);  // Block LED flashing

  rgb_t color = rgb_from_code(MAGENTA_SAIYAN);
  color       = rgb_fade(color, 0xf0);
  for (int i = 1; i < 7; i++) {
    led_rgb_color(i, color);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    led_all_off();
  }
  vTaskDelay(20 / portTICK_PERIOD_MS);
  led_all_on(color);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  for (int i = 0; i < 7; i++) {
    led_rgb_off(i);
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }

  led_set_easter_egg_active(false);  // Re-enable LED flashing
}

void led_rainbow() {
  led_set_easter_egg_active(true);  // Block LED flashing

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
  for (int i = 0; i < 7; i++) {
    // Create HSV color with full saturation and brightness
    hsv_t hsv_color = {
        .hue        = rainbow_hues[i],
        .saturation = 255,
        .value      = 200  // Slightly dimmed for better visibility
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
  for (int i = 0; i < 7; i++) {
    led_rgb_off(i);
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }

  ESP_LOGI(__FILE__, "Rainbow sequence completed");

  led_set_easter_egg_active(false);  // Re-enable LED flashing
}

void led_set_easter_egg_active(bool active) {
  easter_egg_active = active;
  ESP_LOGI(__FILE__, "Easter egg mode %s", active ? "ENABLED" : "DISABLED");
}

void led_task(void *arg) {
  ESP_LOGI(__FILE__, "Starting Led task");

  while (1) {
    ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());

    // Skip normal LED operations if easter egg is active
    if (easter_egg_active) {
      ESP_LOGI(__FILE__, "Easter egg active, skipping normal LED operations");
      vTaskDelay(1000 / portTICK_PERIOD_MS);  // Wait 1 second before checking again
      continue;
    }

    bool nearby_set = check_ble_set();
    if (nearby_set) {
      ESP_LOGI(__FILE__, "Set found");
      led_set_completed();
    } else {
      uint8_t nearby_count = count_ble_nodes();
      if (nearby_count > 0) {
        ESP_LOGI(__FILE__, "Badges around: %d", nearby_count);
        led_flash(5000, 0xf0);
      } else {
        ESP_LOGI(__FILE__, "It is just me around");
        led_flash(10000, 0xfa);
      }
    }
  }
}