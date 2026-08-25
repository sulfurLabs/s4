/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2026 sulfurLabs
 *
 * PROJECT: s4
 * FILE: image_cache.c
 *
 */

#include "ui16_priv.h"
#include <bmp.h>

#define UI16_IMG_CACHE_MAX 8

typedef struct
{
    const char *path;
    bmp_image_t image;
    int loaded;
} ui16__img_entry_t;

static ui16__img_entry_t g_cache[UI16_IMG_CACHE_MAX];
static int g_cache_count = 0;

static int ui16__strEq(const char *a, const char *b)
{
    while (*a && *a == *b)
    {
        a++;
        b++;
    }

    return *a == *b;
}

const bmp_image_t *ui16__imageLoad(const char *path)
{
    if (!path || !path[0]) return 0;

    for (int i = 0; i < g_cache_count; i++)
    {
        if (
            ui16__strEq(g_cache[i].path, path)
        ) return g_cache[i].loaded ? &g_cache[i].image : 0;
    }

    if (g_cache_count >= UI16_IMG_CACHE_MAX) return 0;

    ui16__img_entry_t *entry = &g_cache[g_cache_count++];
    entry->path = path;
    entry->loaded = (bmp_load(path, &entry->image) == 0);

    return entry->loaded ? &entry->image : 0;
}