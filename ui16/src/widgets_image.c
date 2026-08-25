/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2026 sulfurLabs
 *
 * PROJECT: s4
 * FILE: widgets_image.c
 *
 */

#include "ui16.h"
#include "ui16_priv.h"

ui16_node_t *ui16__imageSimple(const char *path)
{
    return ui16__imageStyled(ui16__defaultStyle(), path);
}

ui16_node_t *ui16__imageStyled(ui16_style_t image_style, const char *path)
{
    return ui16__attachNode(UI16_NODE_IMAGE, image_style, path);
}