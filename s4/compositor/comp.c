/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2026 sulfurLabs
 *
 * PROJECT: s4
 * FILE: comp.c
 *
 */

#include "comp.h"
#include "fb_backend.h"
#include "surface.h"
#include "scale.h"
#include <unistd.h>
#include <stdio.h>
#include "../cfg.h"

static fb_backend_t g_fb;
static surface_t    g_surface;
static unsigned int *g_buf_shadow = 0;

static void check_g_buf(const char *where)
{
    if (g_buf_shadow && g_surface.pixels != g_buf_shadow)
    {
        printf(
            "[COMP] !!! g_buf CHANGED at %s: was=%p now=%p\n",
            where,
            (void *)g_buf_shadow,
            (void *)g_surface.pixels
        );
    }
    g_buf_shadow = g_surface.pixels;
}

void comp_init(int fb_fd, int requested_internal_width, int requested_internal_height)
{
    printf(":: init compositor\n");

    if (!fb_backend_init(&g_fb, fb_fd)) return;

    #if RENDERER_SCALING_ENABLED
        printf(":: comp: rendered scaling is enabled!\n");

        if (!surface_alloc(&g_surface, requested_internal_width, requested_internal_height))
        {
            printf(
                ":: comp: internal_render_surface allocation failed\n"
                "   we will fall back to real framebuffer :(\n"
            );
            surface_alias(&g_surface, g_fb.pixels, g_fb.width, g_fb.height, g_fb.stride);
        }

	    //g_buf = g_internal_render_surface;
    #else
        (void)requested_internal_width;
        (void)requested_internal_height;
        surface_alias(&g_surface, g_fb.pixels, g_fb.width, g_fb.height, g_fb.stride);
    #endif

    g_buf_shadow = g_surface.pixels;
}

void comp_capture(void)
{
    check_g_buf(__func__);
}

void comp_fill(int x, int y, int w, int h, unsigned int color)
{
    check_g_buf(__func__);
    surface_fill(&g_surface, x, y, w, h, color);
}

unsigned int comp_get(int x, int y)
{
    check_g_buf(__func__);
    return surface_get(&g_surface, x, y);
}

unsigned int comp_BMP_target_get(void *ctx, int x, int y)
{
    (void)ctx;
    return comp_get(x, y);
}

void comp_set(int x, int y, unsigned int c)
{
    check_g_buf(__func__);
    surface_set(&g_surface, x, y, c);
}

void comp_BMP_target_set(void *ctx, int x, int y, unsigned int c)
{
    (void)ctx;
    comp_set(x, y, c);
}

//needs libbmp
const bmp_target_t g_comp_target =
{
    .set = comp_BMP_target_set,
    .get = comp_BMP_target_get,
    .ctx = NULL
};

void comp_put_row(int x, int y, const unsigned int *row, int len)
{
    check_g_buf(__func__);
    surface_put_row(&g_surface, x, y, row, len);
}

void comp_copy_rect(int src_x, int src_y, int dst_x, int dst_y, int w, int h)
{
    check_g_buf(__func__);
    surface_copy_rect(&g_surface, src_x, src_y, dst_x, dst_y, w, h);
}

void comp_put_pixels(int x, int y, int w, int h, const unsigned int *pixels)
{
    check_g_buf(__func__);
    surface_put_pixels(&g_surface, x, y, w, h, pixels);
}

void comp_flush_rect(int internal_x, int internal_y, int internal_width, int internal_height)
{
    check_g_buf(__func__);
    if (!g_surface.pixels || g_fb.fd < 0) return;

    #if RENDERER_SCALING_ENABLED
        if (g_surface.pixels != g_fb.pixels && g_fb.pixels)
        {
            int px0 = internal_x * g_fb.width / g_surface.width;
            int py0 = internal_y * g_fb.height / g_surface.height;
            int px1 = (internal_x + internal_width) * g_fb.width / g_surface.width + 1;
            int py1 = (internal_y + internal_height) * g_fb.height / g_surface.height + 1;

            if (px0 < 0) px0 = 0;
            if (py0 < 0) py0 = 0;
            if (px1 > g_fb.width) px1 = g_fb.width;
            if (py1 > g_fb.height) py1 = g_fb.height;

            scale_bilinear_region(
                g_surface.pixels,
                g_surface.width,
                g_surface.height,
                g_fb.pixels,
                g_fb.width,
                g_fb.height,
                g_fb.stride,
                px0,
                py0,
                px1,
                py1
            );

            fb_backend_flush_rect(
                &g_fb,
                (unsigned)px0,
                (unsigned)py0,
                (unsigned)(px1 - px0),
                (unsigned)(py1 - py0)
            );
            return;
        }
    #else
        fb_backend_flush_rect(
            &g_fb,
            (unsigned)internal_x,
            (unsigned)internal_y,
            (unsigned)internal_width,
            (unsigned)internal_height
        );
        return;
    #endif

    fb_backend_flush_all(&g_fb);
}

void comp_flush(void)
{
    comp_flush_rect(0, 0, g_surface.width, g_surface.height);
}

int comp_w(void)
{
    if (g_surface.width <= 0 || g_surface.height <= 0)
    {
        printf(
            "[COMP] !!! comp_w() called with corrupted state: w=%d h=%d pixels=%p fd=%d\n",
            g_surface.width, g_surface.height, (void *)g_surface.pixels, g_fb.fd
        );
    }
    return g_surface.width;
}

int comp_h(void)
{
    if (g_surface.width <= 0 || g_surface.height <= 0)
    {
        printf(
            "[COMP] !!! comp_h() called with corrupted state: w=%d h=%d pixels=%p fd=%d\n",
            g_surface.width, g_surface.height, (void *)g_surface.pixels, g_fb.fd
        );
    }
    return g_surface.height;
}
