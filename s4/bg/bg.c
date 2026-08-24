/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2026 sulfurLabs
 *
 * PROJECT: s4
 * FILE: bg.c
 *
 */

#include "bg.h"
#include "../compositor/comp.h"
#include "../compositor/scale.h" //bilineal scaling
#include "../../libbmp/bmp.h"
#include <stdio.h>
#include <stdlib.h>

static bmp_image_t g_bg;
static int g_bg_loaded = 0;

#define BG_ROW_MAX 4096
static unsigned int g_row_buf[BG_ROW_MAX];

void bg_init(int w, int h)
{
    printf(":: init background renderer\n");
    (void)w;
    (void)h;

    const char *path = BG_PATH;

    printf(":: bg: loading bg picture from path: %s\n", path);

    int res = bmp_load(path, &g_bg);

    printf(":: bg: bmp_load result: %d\n", res);

    if (res != 0) {
        printf(":: error: failed to unknown reason\n");
        g_bg_loaded = 0;
        return;
    }
    if (!g_bg.pixels) {
        printf(":: error: pixels pointer is NULL\n");
        g_bg_loaded = 0;
        return;
    }
    if (g_bg.width <= 0 || g_bg.height <= 0) {
        printf(":: error: return. %dx%d\n", g_bg.width, g_bg.height);
        g_bg_loaded = 0;
        return;
    }

    printf("   bg: resolution: %dx%d\n", g_bg.width, g_bg.height);
    printf("   bg: pixels ptr: %p\n", g_bg.pixels);


    int sw = comp_w();
    int sh = comp_h();

    printf(":: bg: analysing screen... %dx%d\n", sw, sh);

    if (g_bg.width > sw || g_bg.height > sh) {
        printf(":: warning: bg picture is too large...\n");
    }
    if (g_bg.width < sw || g_bg.height < sh) {
        printf(":: warning: bg picture is smaller than the screen...\n");
    }

    unsigned char *p = (unsigned char *)g_bg.pixels;
    printf("   bg: %02X %02X %02X %02X (if available)\n", p[0], p[1], p[2], p[3]);

    #if RENDERER_ENABLE_BACKGROUND_BS
        printf(":: bg: bilinear scaling enabled\n");
    #else
        printf(":: bg: bilinear scaling disabled\n");
    #endif

    g_bg_loaded = 1;

    //printf("done\n");
}

void bg_draw_full(void)
{
    int sw = comp_w();
    int sh = comp_h();

    if (sw <= 0 || sh <= 0) return;

    if (!g_bg_loaded || !g_bg.pixels) {
        comp_fill(0, 0, sw, sh, DT_BG);
        return;
    }

    #if RENDERER_ENABLE_BACKGROUND_BS

        unsigned int *scaled = malloc(
            (size_t)sw * (size_t)sh * sizeof(unsigned int)
        );

        if (!scaled)
        {
            printf(":: error: failed to allocate bilinear background buffer\n");
            comp_fill(0, 0, sw, sh, DT_BG);
            return;
        }

        scale_bilinear_region(
            g_bg.pixels,
            g_bg.width,
            g_bg.height,
            scaled,
            sw,
            sh,
            sw,
            0,
            0,
            sw,
            sh
        );

        for (int y = 0; y < sh; y++)
        {
            comp_put_pixels(
                0,
                y,
                sw,
                1,
                &scaled[y * sw]
            );
        }

        free(scaled);
    #else
        int bw = g_bg.width;
        int bh = g_bg.height;
        int rlen = sw < BG_ROW_MAX ? sw : BG_ROW_MAX;

        for (int y = 0; y < sh; y++)
        {
            int sy = (y * bh) / sh;

            for (int x = 0; x < rlen; x++)
            {
                int sx = (x * bw) / sw;
                g_row_buf[x] = g_bg.pixels[sy * bw + sx];
            }
            comp_put_pixels(0, y, rlen, 1, g_row_buf);
        }
    #endif
}

void bg_draw_rect(int x, int y, int w, int h)
{
    int sw = comp_w();
    int sh = comp_h();
    int dy, dx;

    if (sw <= 0 || sh <= 0)
    {
        printf(
            "[BG] bg_draw_rect: bailing out, invalid screen size sw=%d sh=%d \n(x=%d y=%d w=%d h=%d)\n",
            sw,
            sh,
            x,
            y,
            w,
            h
        );
        return;
    }
    if (w <= 0 || h <= 0)  return;

    if (!g_bg_loaded || !g_bg.pixels)
    {
        comp_fill(x, y, w, h, DT_BG);
        return;
    }

    #if RENDERER_ENABLE_BACKGROUND_BS == 1
        unsigned int *scaled = malloc(
            (size_t)w * (size_t)h * sizeof(unsigned int)
        );

        if (!scaled)
        {
            printf(":: error: failed to allocate bilinear rect buffer\n");
            comp_fill(x, y, w, h, DT_BG);
            return;
        }

        scale_bilinear_region(
            g_bg.pixels,
            g_bg.width,
            g_bg.height,
            scaled,
            sw,
            sh,
            w,
            x,
            y,
            x + w,
            y + h
        );

        for (int dy = 0; dy < h; dy++)
        {
            comp_put_pixels(
                x,
                y + dy,
                w,
                1,
                &scaled[dy * w]
            );
        }
        free(scaled);
    #else

        int bw = g_bg.width;
        int bh = g_bg.height;
        int rlen = w < BG_ROW_MAX ? w : BG_ROW_MAX;

        for (int dy = 0; dy < h; dy++)
        {
            int sy = ((y + dy) * bh) / sh;

            for (int dx = 0; dx < rlen; dx++)
            {
                int sx = ((x + dx) * bw) / sw;
                g_row_buf[dx] = g_bg.pixels[sy * bw + sx];
            }
            comp_put_pixels(x, y + dy, rlen, 1, g_row_buf);
        }
    #endif
}