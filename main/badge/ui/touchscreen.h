#ifndef _TOUCHSCREEN_H
#define _TOUCHSCREEN_H

#include <stdint.h>
#include <stdbool.h>

#define RESOLLUTION_12BIT 4096

typedef enum {
  MEASURE_TEMP0 = 0,
  MEASURE_AUX = 2,
  MEASURE_TEMP1 = 4,
  ACTIVATE_X = 8,
  ACTIVATE_Y = 9,
  ACTIVATE_YPLUS_X = 10,
  SETUP_COMMAND = 11,
  MEASURE_X = 12,
  MEASURE_Y = 13,
  MEASURE_Z1 = 14,
  MEASURE_Z2 = 15
} tsc2007_function;

typedef enum {
  POWERDOWN_IRQON = 0,
  ADON_IRQOFF = 1,
  ADOFF_IRQON = 2,
} tsc2007_power;

typedef enum {
  ADC_12BIT = 0,
  ADC_8BIT = 1,
} tsc2007_resolution;



typedef struct {
  uint16_t x, y, z;
  bool valid;
} point_t;


void ui_touchscreen_init();

#endif