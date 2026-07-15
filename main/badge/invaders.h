#ifndef _INVADERS_H
#define _INVADERS_H

#include <stdio.h>
#include <stdlib.h>

#include "ui.h"

// --- invader grid ---
#define INV_ROWS       4
#define INV_COLS       6
#define INV_COUNT      (INV_ROWS * INV_COLS)
#define INV_W          24          // invader sprite width, px (24x16 sprites)
#define INV_H          16          // invader sprite height, px
#define INV_CELL       32          // grid pitch (invader + gap), px
#define INV_BLOCK_W    ((INV_COLS - 1) * INV_CELL + INV_W)   // block width
#define INV_START_X    20          // initial block left
#define INV_START_Y    40          // initial block top (below the score line)
#define INV_HSTEP      6           // horizontal jump per move
#define INV_DROP       14          // vertical drop when hitting an edge
#define INV_MARGIN     6           // side margin the block bounces within

// --- cannon (the "ship", on the bottom row) ---
// NOTE: the LVGL coordinate space here is 320 wide x 240 tall (LV_HOR_RES x
// LV_VER_RES). The ship's Y is computed at runtime from LV_VER_RES, so it works
// regardless of resolution/orientation.
#define CANNON_W       32          // cannon sprite width, px (32x16 sprite, byte-aligned)
#define CANNON_H       16
#define CANNON_STEP    12          // move per wheel press
#define CANNON_BOTTOM_MARGIN 18    // gap below the cannon (clears panel overscan)

// --- shots ---
#define BULLET_W       3
#define BULLET_H       10
#define BULLET_SPEED   14          // px per tick (upward)
#define BOMB_W         3
#define BOMB_H         10
#define BOMB_SPEED     6           // px per tick (downward)
#define MAX_BOMBS      3

// --- persistence ---
// High score lives on the SPIFFS filesystem (mounted at /data), like the other
// badge data (settings.json, schedule.json) — survives reboots and firmware-only
// reflashes.
#define HIGHSCORE_FILE "/data/invaders_hi"

// --- pacing (task runs every INVADERS_TICK_MS) ---
#define INVADERS_TICK_MS   40
#define INV_SPEED_START    12      // ticks between invader jumps at wave start
#define INV_SPEED_MIN      2       // fastest
#define BOMB_CHANCE        6       // 1/N chance to drop a bomb per invader move

void invaders_reset(lv_obj_t *parent);   // (re)start a fresh game
void invaders_task(lv_task_t *arg);       // per-tick game step
void invaders_move_cannon(int8_t dir);    // -1 = left, +1 = right (wheel press)
void invaders_fire(void);                 // fire a shot (both-wheel press)

#endif // _INVADERS_H
