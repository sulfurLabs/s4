/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2026 sulfurLabs
 *
 * PROJECT: s4
 * FILE: tga.h
 *
 */

#pragma once

#include "../../../libbmp/bmp.h"

int  tga_load(const char *path, bmp_image_t *img);
void tga_free(bmp_image_t *img);