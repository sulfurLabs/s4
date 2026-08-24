/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2026 sulfurLabs
 *
 * PROJECT: s4
 * FILE: tb_widget.h
 *
 */

#pragma once

#include <sys/types.h>
#include "../../libbmp/bmp.h"

// widget types
#define TB_WIDGET_NONE          0
#define TB_WIDGET_APP           1   // launch when click
#define TB_WIDGET_LABEL         2   // opens a popup window
#define TB_WIDGET_UPDATED_LABEL 3   // like LABEL but text/icon can be changed at runtime by an app
#define TB_WIDGET_START         4

typedef enum {
    TB_DISP_ICON_TEXT = 0,
    TB_DISP_ICON_ONLY = 1,
    TB_DISP_TEXT_ONLY = 2,
} tb_disp_t;

#define TB_WIDGET_MAX          16
#define TB_WIDGET_NAMELEN      32
#define TB_WIDGET_EXECLEN      128
#define TB_WIDGET_TEXTLEN      48   // displayed text (label / updated_label)

typedef struct {
    int          loaded;        // 0 = no icon yet
    // int          w, h;
    bmp_image_t image;
} tb_icon_t;

typedef struct {
    int          type;                      // TB_WIDGET_*
    char         name[TB_WIDGET_NAMELEN];
    char         text[TB_WIDGET_TEXTLEN];
    char         exec[TB_WIDGET_EXECLEN];
    tb_icon_t    icon;
    const char *icon_path;

    tb_disp_t disp;

    // popup state (LABEL / UPDATED_LABEL)
    pid_t        popup_pid;
    int          popup_w;
    int          popup_h;
    int          dirty;         // text changed, set 1
} tb_widget_t;
