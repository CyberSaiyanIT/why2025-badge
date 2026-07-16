#include "invaders.h"
#include <string.h>

// ---------------------------------------------------------------------------
// Space Invaders — a compact badge mini-game.
// Controls: the two dial wheels move the laser cannon left/right; the cannon
// auto-fires (one shot on screen at a time, classic style). Long-press still
// switches screens. High score is persisted to a file on the SPIFFS
// filesystem (/data), like the other badge data.
// The aliens and cannon are the classic pixel-art sprites (lv_img objects,
// INDEXED_1BIT, defined in common/img_invaders.c); shots stay plain rects.
// Sprites/objects are moved with lv_obj_set_pos (like snake.c).
// The task priority is driven by ui.c (enabled on the invaders screen only).
// ---------------------------------------------------------------------------

// classic sprites (main/badge/common/img_invaders.c), pixel-doubled x2:
// 3 alien types x 2 animation frames (white) + the laser base (green).
LV_IMG_DECLARE(inv_sq_a); LV_IMG_DECLARE(inv_sq_b);   // squid   (top row)
LV_IMG_DECLARE(inv_cr_a); LV_IMG_DECLARE(inv_cr_b);   // crab    (2nd row)
LV_IMG_DECLARE(inv_oc_a); LV_IMG_DECLARE(inv_oc_b);   // octopus (lower rows)
LV_IMG_DECLARE(inv_cannon);

// which alien shape a grid row uses, for the given animation frame
static const lv_img_dsc_t *alien_sprite(int row, bool frame)
{
    switch (row) {
        case 0:  return frame ? &inv_sq_b : &inv_sq_a;
        case 1:  return frame ? &inv_cr_b : &inv_cr_a;
        default: return frame ? &inv_oc_b : &inv_oc_a;
    }
}

typedef struct {
    lv_obj_t *cannon;
    lv_obj_t *inv[INV_COUNT];
    bool      alive[INV_COUNT];
    int       alive_count;
    int       block_x, block_y;      // top-left of the invader block (px)
    int       cannon_y;              // ship row (computed from screen height)
    int8_t    dir;                   // +1 right, -1 left
    lv_obj_t *bullet;                // single player shot (NULL = none)
    lv_obj_t *bombs[MAX_BOMBS];      // enemy bombs
    int       score;
    uint32_t  high;
    lv_obj_t *lbl_score, *lbl_high, *lbl_msg, *msg_bg;
    bool      over;
    bool      frame;                 // invader animation frame (toggles each step)
    int       move_ctr;              // ticks since last invader jump
    int       speed;                 // ticks between invader jumps
} inv_game_t;

static inv_game_t g;

// ---- styles (init once) — only the shots and the game-over panel need them now;
//      the aliens and cannon are image sprites ----
static bool styles_done = false;
static lv_style_t st_bullet, st_bomb, st_msgbg;

static void style_rect(lv_style_t *s, lv_color_t col)
{
    lv_style_init(s);
    lv_style_set_bg_color(s, LV_STATE_DEFAULT, col);
    lv_style_set_bg_opa(s, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_style_set_border_width(s, LV_STATE_DEFAULT, 0);
    lv_style_set_radius(s, LV_STATE_DEFAULT, 0);
}

static void init_styles(void)
{
    if (styles_done) return;
    styles_done = true;
    style_rect(&st_bullet, LV_COLOR_YELLOW);
    style_rect(&st_bomb,   LV_COLOR_RED);
    style_rect(&st_msgbg,  LV_COLOR_BLACK);
    lv_style_set_border_width(&st_msgbg, LV_STATE_DEFAULT, 2);
    lv_style_set_border_color(&st_msgbg, LV_STATE_DEFAULT, LV_COLOR_WHITE);
}

// ---- helpers ----
static lv_obj_t *make_rect(lv_obj_t *parent, lv_style_t *st, int w, int h)
{
    lv_obj_t *o = lv_obj_create(parent, NULL);
    lv_obj_add_style(o, LV_OBJ_PART_MAIN, st);
    lv_obj_set_size(o, w, h);
    return o;
}

static int inv_x(int i) { return g.block_x + (i % INV_COLS) * INV_CELL; }
static int inv_y(int i) { return g.block_y + (i / INV_COLS) * INV_CELL; }

static bool overlap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh)
{
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

static uint32_t load_high(void)
{
    uint32_t v = 0;
    FILE *fp = fopen(HIGHSCORE_FILE, "r");
    if (fp) {
        if (fscanf(fp, "%u", &v) != 1) v = 0;
        fclose(fp);
    }
    return v;
}

static void save_high(uint32_t v)
{
    FILE *fp = fopen(HIGHSCORE_FILE, "w");
    if (fp) {
        fprintf(fp, "%u", (unsigned)v);
        fclose(fp);
    }
}

static void update_labels(void)
{
    lv_label_set_text_fmt(g.lbl_score, "SCORE %d", g.score);
    lv_label_set_text_fmt(g.lbl_high,  "HIGH %u", (unsigned)g.high);
    // re-anchor top-right so a growing number doesn't clip off the edge
    lv_obj_align(g.lbl_high, NULL, LV_ALIGN_IN_TOP_RIGHT, -4, 4);
}

static void reposition_invaders(void)
{
    for (int i = 0; i < INV_COUNT; i++)
        if (g.alive[i]) lv_obj_set_pos(g.inv[i], inv_x(i), inv_y(i));
}

// point each alive invader at its shape's current animation frame
static void animate_invaders(void)
{
    for (int i = 0; i < INV_COUNT; i++)
        if (g.alive[i]) lv_img_set_src(g.inv[i], alien_sprite(i / INV_COLS, g.frame));
}

static void spawn_wave(void)
{
    g.block_x = INV_START_X;
    g.block_y = INV_START_Y;
    g.dir = 1;
    g.frame = false;
    g.alive_count = INV_COUNT;
    for (int i = 0; i < INV_COUNT; i++) {
        g.alive[i] = true;
        lv_obj_set_hidden(g.inv[i], false);
    }
    animate_invaders();
    reposition_invaders();
}

static void game_over(void)
{
    g.over = true;
    if ((int)g.high < g.score) g.high = g.score;
    save_high(g.high);
    update_labels();
    lv_label_set_text_fmt(g.lbl_msg, "GAME OVER\nSCORE %d\nHIGH %u\n\npress a wheel\nto play again",
                          g.score, (unsigned)g.high);
    lv_obj_set_hidden(g.lbl_msg, false);
    // size the black panel to wrap the (now measured) label, center both,
    // and draw them on top of the invaders (bg first, then the text over it)
    lv_obj_set_size(g.msg_bg, lv_obj_get_width(g.lbl_msg) + 24,
                              lv_obj_get_height(g.lbl_msg) + 20);
    lv_obj_set_hidden(g.msg_bg, false);
    lv_obj_align(g.msg_bg, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align(g.lbl_msg, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_obj_move_foreground(g.msg_bg);
    lv_obj_move_foreground(g.lbl_msg);
}

// ---- public API ----

void invaders_reset(lv_obj_t *parent)
{
    init_styles();
    if (invaders_task_handle) lv_task_set_prio(invaders_task_handle, LV_TASK_PRIO_OFF);

    srand(lv_tick_get());

    lv_obj_clean(parent);        // drop any objects from a previous game
    memset(&g, 0, sizeof(g));

    // HUD
    g.lbl_score = lv_label_create(parent, NULL);
    lv_obj_set_style_local_text_color(g.lbl_score, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_obj_align(g.lbl_score, NULL, LV_ALIGN_IN_TOP_LEFT, 4, 4);

    g.lbl_high = lv_label_create(parent, NULL);
    lv_obj_set_style_local_text_color(g.lbl_high, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
    lv_obj_align(g.lbl_high, NULL, LV_ALIGN_IN_TOP_RIGHT, -4, 4);

    // black panel behind the game-over text (sized in game_over) + the label
    g.msg_bg = make_rect(parent, &st_msgbg, 10, 10);
    lv_obj_set_hidden(g.msg_bg, true);

    g.lbl_msg = lv_label_create(parent, NULL);
    lv_obj_set_style_local_text_color(g.lbl_msg, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_label_set_align(g.lbl_msg, LV_LABEL_ALIGN_CENTER);
    lv_obj_set_hidden(g.lbl_msg, true);

    // cannon (ship) sprite near the bottom — Y derived from the real screen
    // height, with a generous bottom margin so it clears any panel overscan
    // (the small green base sits at the very bottom otherwise and gets clipped).
    g.cannon_y = LV_VER_RES - CANNON_H - CANNON_BOTTOM_MARGIN;
    g.cannon = lv_img_create(parent, NULL);
    lv_img_set_src(g.cannon, &inv_cannon);
    lv_obj_set_size(g.cannon, CANNON_W, CANNON_H);
    // tint the white cannon sprite green at draw time (arcade base color)
    lv_obj_set_style_local_image_recolor(g.cannon, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_LIME);
    lv_obj_set_style_local_image_recolor_opa(g.cannon, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_obj_set_pos(g.cannon, (LV_HOR_RES - CANNON_W) / 2, g.cannon_y);

    // invader sprites (shape set per row in animate_invaders / spawn_wave)
    for (int i = 0; i < INV_COUNT; i++) {
        g.inv[i] = lv_img_create(parent, NULL);
        lv_img_set_src(g.inv[i], alien_sprite(i / INV_COLS, false));
    }

    g.bullet = NULL;
    g.score = 0;
    g.high = load_high();
    g.over = false;
    g.move_ctr = 0;
    g.speed = INV_SPEED_START;

    spawn_wave();
    update_labels();

    // leave the game paused; the first wheel input (move or fire) starts it,
    // like Snake — so arriving on the screen doesn't kick off immediately.
}

void invaders_move_cannon(int8_t dir)
{
    if (g.over) { invaders_reset(screen_invaders); return; }   // any press replays
    if (!g.cannon) return;
    // first input starts the (paused) game
    if (invaders_task_handle) lv_task_set_prio(invaders_task_handle, LV_TASK_PRIO_LOW);
    int x = lv_obj_get_x(g.cannon) + dir * CANNON_STEP;
    if (x < 0) x = 0;
    if (x > LV_HOR_RES - CANNON_W) x = LV_HOR_RES - CANNON_W;
    lv_obj_set_pos(g.cannon, x, lv_obj_get_y(g.cannon));
}

void invaders_fire(void)
{
    if (g.over || !g.cannon) return;
    // first input starts the (paused) game
    if (invaders_task_handle) lv_task_set_prio(invaders_task_handle, LV_TASK_PRIO_LOW);
    if (g.bullet) return;                           // one shot on screen at a time
    g.bullet = make_rect(screen_invaders, &st_bullet, BULLET_W, BULLET_H);
    lv_obj_set_pos(g.bullet, lv_obj_get_x(g.cannon) + CANNON_W / 2 - BULLET_W / 2, g.cannon_y - BULLET_H);
}

void invaders_task(lv_task_t *arg)
{
    if (g.over || !g.cannon) return;

    // 1) player bullet: advance, check invader hits
    if (g.bullet) {
        int bx = lv_obj_get_x(g.bullet);
        int by = lv_obj_get_y(g.bullet) - BULLET_SPEED;
        if (by + BULLET_H < 0) {
            lv_obj_del(g.bullet); g.bullet = NULL;
        } else {
            lv_obj_set_pos(g.bullet, bx, by);
            for (int i = 0; i < INV_COUNT; i++) {
                if (!g.alive[i]) continue;
                if (overlap(bx, by, BULLET_W, BULLET_H, inv_x(i), inv_y(i), INV_W, INV_H)) {
                    g.alive[i] = false;
                    lv_obj_set_hidden(g.inv[i], true);
                    g.alive_count--;
                    g.score += (INV_ROWS - (i / INV_COLS)) * 10;   // top rows worth more
                    if ((int)g.high < g.score) g.high = g.score;
                    lv_obj_del(g.bullet); g.bullet = NULL;
                    g.speed = INV_SPEED_START * g.alive_count / INV_COUNT;   // faster as they thin out
                    if (g.speed < INV_SPEED_MIN) g.speed = INV_SPEED_MIN;
                    update_labels();
                    break;
                }
            }
        }
    }
    // (no auto-fire: shots are fired manually via invaders_fire())

    // 2) invader block: jump every g.speed ticks
    if (++g.move_ctr >= g.speed) {
        g.move_ctr = 0;
        int nx = g.block_x + g.dir * INV_HSTEP;
        if (nx < INV_MARGIN || nx + INV_BLOCK_W > LV_HOR_RES - INV_MARGIN) {
            g.dir = -g.dir;
            g.block_y += INV_DROP;
        } else {
            g.block_x = nx;
        }
        g.frame = !g.frame;          // classic marching wobble
        animate_invaders();
        reposition_invaders();

        // reached the cannon line? -> lose
        for (int i = 0; i < INV_COUNT; i++)
            if (g.alive[i] && inv_y(i) + INV_H >= g.cannon_y) { game_over(); return; }

        // occasionally drop a bomb from a random alive invader
        if (g.alive_count > 0 && (rand() % BOMB_CHANCE) == 0) {
            int slot = -1;
            for (int b = 0; b < MAX_BOMBS; b++) if (!g.bombs[b]) { slot = b; break; }
            if (slot >= 0) {
                int pick = rand() % g.alive_count, idx = -1;
                for (int i = 0; i < INV_COUNT; i++)
                    if (g.alive[i] && pick-- == 0) { idx = i; break; }
                if (idx >= 0) {
                    g.bombs[slot] = make_rect(screen_invaders, &st_bomb, BOMB_W, BOMB_H);
                    lv_obj_set_pos(g.bombs[slot], inv_x(idx) + INV_W / 2, inv_y(idx) + INV_H);
                }
            }
        }

        // wave cleared -> next (endless), slightly harder
        if (g.alive_count == 0) {
            spawn_wave();
            if (g.speed > INV_SPEED_MIN) g.speed--;
        }
    }

    // 3) enemy bombs: advance, check cannon hit
    int cx = lv_obj_get_x(g.cannon), cy = lv_obj_get_y(g.cannon);
    for (int b = 0; b < MAX_BOMBS; b++) {
        if (!g.bombs[b]) continue;
        int x = lv_obj_get_x(g.bombs[b]);
        int y = lv_obj_get_y(g.bombs[b]) + BOMB_SPEED;
        if (y > LV_VER_RES) { lv_obj_del(g.bombs[b]); g.bombs[b] = NULL; continue; }
        lv_obj_set_pos(g.bombs[b], x, y);
        if (overlap(x, y, BOMB_W, BOMB_H, cx, cy, CANNON_W, CANNON_H)) { game_over(); return; }
    }
}
