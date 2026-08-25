/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2026 sulfurLabs
 *
 * PROJECT: s4
 * FILE: cursor.c
 *
 */

#include "cursor.h"
#include "../compositor/comp.h"
#include "../cfg.h"

#include <sys/fb.h>
#include <unistd.h>

// all sprite data comes from data.h now

static cur_type_t g_cur_type = CUR_TYPE_NORMAL;

// returns active sprite
static const unsigned int *cur_px_active(void)
{
    switch (g_cur_type)
    {
        case CUR_TYPE_HRESIZE: return cur_hresize_px;
        case CUR_TYPE_VRESIZE: return cur_vresize_px;
        case CUR_TYPE_DRESIZE_NWSE: return cur_dresize_nwse_px;
        case CUR_TYPE_DRESIZE_NESW: return cur_dresize_nesw_px;
        default: return cur_normal_px;
    }
}

int cur_w(void) {
	return g_cur_type == CUR_TYPE_NORMAL ? CUR_NORMAL_W : CUR_RESIZE_W;
}

int cur_h(void) {
	return g_cur_type == CUR_TYPE_NORMAL ? CUR_NORMAL_H : CUR_RESIZE_H;
}

void cur_set_type(cur_type_t type) {
	g_cur_type = type;
}

// bg save buffer big enough for largest cursor
static unsigned int bg_save[CUR_RESIZE_W * CUR_RESIZE_H];
static int bg_save_w = 0;
static int bg_save_h = 0;
static int bg_valid = 0;
static int old_x = 0;
static int old_y = 0;
static int g_fd  = -1;
static int g_scr_w = 0;
static int g_scr_h = 0;

static void clamp(int *x, int *y)
{
    int cw = cur_w();
    int ch = cur_h();
    if (*x < 0) *x = 0;
    if (*y < 0) *y = 0;
    if (*x + cw > g_scr_w) *x = g_scr_w - cw;
    if (*y + ch > g_scr_h) *y = g_scr_h - ch;
}

void cur_init(int fb_fd, int w, int h)
{
    g_fd = fb_fd;
    g_scr_w = w;
    g_scr_h = h;
    bg_save_w = 0;
    bg_save_h = 0;
    bg_valid = 0;
    old_x = 0;
    old_y = 0;
}

static void flush_rect(int x, int y, int w, int h)
{
    comp_flush_rect(x, y, w, h);
}

void cur_undo_from_backbuf(void)
{
    if (!bg_valid) return;
    comp_put_pixels(
        old_x,
        old_y,
        bg_save_w,
        bg_save_h,
        bg_save
    );
}

void cur_bake(int x, int y)
{
    int cw = cur_w();
    int ch = cur_h();
    const unsigned int *px = cur_px_active();

    clamp(&x, &y);

    for (int row = 0; row < ch; row++)
    {
        for (int col = 0; col < cw; col++)
        {
            bg_save[row * cw + col] = comp_get(x + col, y + row);
        }
    }

    for (int row = 0; row < ch; row++)
    {
        for (int col = 0; col < cw; col++)
        {
            unsigned int c = px[row * cw + col];
            if (c >> 24) comp_set(x + col, y + row, c);
        }
    }

    old_x = x;
    old_y = y;
    bg_save_w = cw;
    bg_save_h = ch;
    bg_valid = 1;
}

void cur_erase_fb(void)
{
    if (!bg_valid || g_fd < 0) return;
    comp_put_pixels(
        old_x,
        old_y,
        bg_save_w,
        bg_save_h,
        bg_save
    );
    flush_rect(
        old_x,
        old_y,
        bg_save_w,
        bg_save_h
    );
    bg_valid = 0;
}

void cur_draw_fb(int x, int y)
{
    if (g_fd < 0) return;

    int cw = cur_w();
    int ch = cur_h();
    const unsigned int *px = cur_px_active();

    clamp(&x, &y);

    for (int row = 0; row < ch; row++)
    {
        for (int col = 0; col < cw; col++)
        {
            bg_save[row * cw + col] = comp_get(x + col, y + row);
        }
    }

    for (int row = 0; row < ch; row++)
    {
        for (int col = 0; col < cw; col++)
        {
            unsigned int c = px[row * cw + col];
            if (c >> 24) comp_set(x + col, y + row, c);
        }
    }

    flush_rect(x, y, cw, ch);

    old_x = x;
    old_y = y;
    bg_save_w = cw;
    bg_save_h = ch;
    bg_valid = 1;
}

int cur_valid(void) { return bg_valid; }
int cur_last_x(void) { return old_x; }
int cur_last_y(void) { return old_y; }