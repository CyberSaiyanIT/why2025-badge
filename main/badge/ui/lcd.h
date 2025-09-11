#pragma once
#ifndef _LCD_H
#define _LCD_H


// Using SPI2 in the example
#define LCD_HOST SPI2_HOST

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your
/// LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define LCD_PIXEL_CLOCK_HZ (62.5 * 1000 * 1000)

#define PIN_NUM_SCLK     6
#define PIN_NUM_MOSI     7
#define PIN_NUM_MISO     2
#define PIN_NUM_LCD_DC   4
#define PIN_NUM_LCD_RST  3
#define PIN_NUM_LCD_CS   10
#define PIN_NUM_BK_LIGHT -1

// The pixel number in horizontal and vertical
#define LCD_H_RES 320
#define LCD_V_RES 240

// Bit number used to represent command and parameter
#define LCD_CMD_BITS        8
#define LCD_PARAM_BITS      8

#define LVGL_DRAW_BUF_LINES 120  // number of display lines in each draw buffer
#define BYTES_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB888))
#define LVGL_BUF_SIZE LCD_H_RES * LCD_V_RES / 10 * BYTES_PER_PIXEL

void ui_lcd_init();

#endif