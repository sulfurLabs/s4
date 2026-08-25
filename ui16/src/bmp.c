/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2026 sulfurLabs
 *
 * PROJECT: s4
 * FILE: bmp.c
 *
 */

#include "ui16.h"
#include "ui16_priv.h"

#include <bmp.h>
#include "../../libbmp/bmp.h"

static void ui16__bmpTargetSet(void *ctx, int x, int y, unsigned int color)
{
    (void)ctx;
    ui16__setBufferPixel(x, y, color);
}

static unsigned int ui16__bmpTargetGet(void *ctx, int x, int y)
{
    (void)ctx;
    return ui16__getBufferPixel(x, y);
}

const bmp_target_t ui16__bmp_target =
{
    .set = ui16__bmpTargetSet,
    .get = ui16__bmpTargetGet,
    .ctx = NULL
};

void ui16_drawBmp(const char *path, int x, int y)
{
    bmp_image_t img;
    if (bmp_load(path, &img) != 0) return;

    bmp_draw(&ui16__bmp_target, &img, x, y);
    bmp_free(&img);
}

void ui16_drawBmpScaled(const char *path, int x, int y, int w, int h)
{
    bmp_image_t img;
    if (bmp_load(path, &img) != 0) return;

    bmp_draw_scaled(&ui16__bmp_target, &img, x, y, w, h);
    bmp_free(&img);
}