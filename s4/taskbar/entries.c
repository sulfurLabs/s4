/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2026 sulfurLabs
 *
 * PROJECT: s4
 * FILE: entries.c
 *
 */

#include "entries.h"
#include "../bg/bmp/bmp.h"
#include "../bg/tga/tga.h"
#include "../cfg.h"
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

//all app entries till we have a parser so it reads from a config
static tb_widget_t s_entries[] =
{
    {
        .type          = TB_WIDGET_APP,
        .name          = "login",
        .text          = "login",
        .exec          = SYSTEM "login.elf",
        .icon_path     = ICONS "exec.bmp",
        .icon          = { .loaded = 0 },
        .disp          = TB_DISP_ICON_ONLY,
        .popup_pid     = -1,
        .popup_w       = 0,
        .popup_h       = 0,
        .dirty         = 0,
    },
    {
        .type          = TB_WIDGET_APP,
        .name          = "DOOM",
        .text          = "DOOM",
        .exec          = "/bin/doomgeneric.elf",
        .icon_path     = ICONS "doom.tga",
        .icon          = { .loaded = 0 },
        .disp          = TB_DISP_ICON_ONLY,
        .popup_pid     = -1,
        .popup_w       = 0,
        .popup_h       = 0,
        .dirty         = 0,
    },
    {
        .type          = TB_WIDGET_APP,
        .name          = "template",
        .text          = "ui16",
        .exec          = "/bin/template.elf",
        .icon_path     = ICONS "template.bmp",
        .icon          = { .loaded = 0 },
        .disp          = TB_DISP_ICON_ONLY,
        .popup_pid     = -1,
        .popup_w       = 0,
        .popup_h       = 0,
        .dirty         = 0,
    },
    {
        .type          = TB_WIDGET_APP,
        .name          = "welcome",
        .text          = "welcome",
        .exec          = "/system/desktop/welcome.elf",
        .icon_path     = ICONS "welcome.bmp",
        .icon          = { .loaded = 0 },
        .disp          = TB_DISP_ICON_ONLY,
        .popup_pid     = -1,
        .popup_w       = 0,
        .popup_h       = 0,
        .dirty         = 0,
    },
    {
        .type          = TB_WIDGET_APP,
        .name          = "gears",
        .text          = "gears",
        .exec          = "/system/desktop/gears.elf",
        .icon_path     = ICONS "exec.bmp",
        .icon          = { .loaded = 0 },
        .disp          = TB_DISP_ICON_TEXT,
        .popup_pid     = -1,
        .popup_w       = 0,
        .popup_h       = 0,
        .dirty         = 0,
    },
};

#define ENTRIES_COUNT (int)(sizeof(s_entries) / sizeof(s_entries[0]))

static int does_my_file_exists(const char *path)
{
    if (!path || path[0] == '\0') return 0;

    int fd = open(path, O_RDONLY);
    if (fd < 0) return 0;

    close(fd);
    return 1;
}

static int count_opaque_pixels(const bmp_image_t *img)
{
    if (!img->pixels) return 0;

    int n = img->width * img->height;
    int i;
    int opaque = 0;

    for (i = 0; i < n; i++)
    {
        if (img->pixels[i] >> 24) opaque++;
    }

    return opaque;
}

void entries_load_icon(tb_widget_t *entry)
{
    const char *path;

    if (entry->icon_path && entry->icon_path[0] != '\0')
    {
        path = entry->icon_path;
    }
    else
    {
        // automatic by app name
        static char auto_path[256];

        snprintf(
            auto_path,
            sizeof(auto_path),
            ICONS "%s.tga",
            entry->name
        );

        path = auto_path;
    }

    if (does_my_file_exists(path) && tga_load(path, &entry->icon.image) == 0)
    {
        entry->icon.loaded = 1;

        int opaque = count_opaque_pixels(&entry->icon.image);
        int total  = entry->icon.image.width * entry->icon.image.height;

        printf(
            ":: icon: '%s': %s(%dx%d) : %d/%d (alpha>0)\n\n",
            entry->name,
            path,
            entry->icon.image.width,
            entry->icon.image.height,
            opaque,
            total
        );

        if (opaque == 0)
        {
            printf(
                ":: icon: '%s': alpha channel is complete 0\n",
                entry->name
            );
        }

        return;
    }

    printf(":: icon: tga: '%s' not working (path: %s). bmp fallback\n", entry->name, path);

    {
        static char bmp_fallback[256];
        int plen = 0;
        while (path[plen]) plen++;

        if (plen > 4 && plen < (int)sizeof(bmp_fallback))
        {
            strncpy(bmp_fallback, path, sizeof(bmp_fallback) - 1);
            bmp_fallback[sizeof(bmp_fallback) - 1] = '\0';
            bmp_fallback[plen - 3] = 'b';
            bmp_fallback[plen - 2] = 'm';
            bmp_fallback[plen - 1] = 'p';

            if (bmp_load(bmp_fallback, &entry->icon.image) == 0)
            {
                entry->icon.loaded = 1;
                printf(":: icon: bmpfallback: '%s' (%s), no alpha channel now\n", entry->name, bmp_fallback);
                return;
            }
        }
    }

    printf(":: icon: '%s': not found, trying exec.tga, then exec.bmp...\n\n", entry->name);

    if (tga_load(ICONS "exec.tga", &entry->icon.image) == 0)
    {
        entry->icon.loaded = 1;
        return;
    }

    if (bmp_load(ICONS "exec.bmp", &entry->icon.image) == 0)
    {
        entry->icon.loaded = 1;
        return;
    }

    printf(":: icon: '%s': all failed, no icon for today :D\n", entry->name);
}

tb_widget_t *entries_get(int *out_count)
{
    if (out_count) *out_count = ENTRIES_COUNT;


    static int initialized = 0;

    if (!initialized)
    {
        for (int i = 0; i < ENTRIES_COUNT; i++) entries_load_icon(&s_entries[i]);

        initialized = 1;
    }


    return s_entries;
}

// to load icons is implemented but not used xd
// used when the user wants to change a icon per app/when running
void taskbar_load_entry_icon(int index)
{
    if (index < 0 || index >= ENTRIES_COUNT) return;
    //i dont want to have icons yet cuz bmp doesnt support transparency but in some time i add png ig
    entries_load_icon(&s_entries[index]);
}