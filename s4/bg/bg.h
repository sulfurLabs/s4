/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2026 sulfurLabs
 *
 * PROJECT: s4
 * FILE: bg.h
 *
 */

#pragma once

#include "../cfg.h"
#define BG_PATH SYSTEM "resources/wallpaper/1.bmp"

void bg_init(int w, int h);
void bg_draw_full(void);
void bg_draw_rect(int x, int y, int w, int h);
