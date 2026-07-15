#ifndef _INVADERS_H
#define _INVADERS_H

#include <stdio.h>
#include <stdlib.h>

#include "ui.h"

// --- invader grid ---
#define INV_ROWS       4
#define INV_COLS       6
#define INV_COUNT      (INV_ROWS * INV_COLS)
#define INV_SIZE       18          // invader square, px
#define INV_CELL       32          // grid pitch (invader + gap), px
#define INV_BLOCK_W    ((INV_COLS - 1) * INV_CELL + INV_SIZE)   // block width
#define INV_START_X    20          // initial block left
#define INV_START_Y    40          // initial block top (below the score line)
#define INV_HSTEP      6           // horizontal jump per move
#define INV_DROP       14          // vertical drop when hitting an edge
#define INV_MARGIN     6           // side margin the block bounces within

// --- cannon ---
#define CANNON_W       24
#define CANNON_H       10
#define CANNON_Y       300         // near the bottom (screen is 320 tall)
#define CANNON_STEP    12          // move per wheel press

// --- shots ---
#define BULLET_W       3
#define BULLET_H       10
#define BULLET_SPEED   14          // px per tick (upward)
#define BOMB_W         3
#define BOMB_H         10
#define BOMB_SPEED     6           // px per tick (downward)
#define MAX_BOMBS      3

// --- pacing (task runs every INVADERS_TICK_MS) ---
#define INVADERS_TICK_MS   40
#define INV_SPEED_START    12      // ticks between invader jumps at wave start
#define INV_SPEED_MIN      2       // fastest
#define BOMB_CHANCE        6       // 1/N chance to drop a bomb per invader move

void invaders_reset(lv_obj_t *parent);   // (re)start a fresh game
void invaders_task(lv_task_t *arg);       // per-tick game step
void invaders_move_cannon(int8_t dir);    // -1 = left, +1 = right (wheel press)

#endif // _INVADERS_H
