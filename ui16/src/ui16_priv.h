/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2026 sulfurLabs
 *
 * PROJECT: s4
 * FILE: ui16_priv.h
 *
 */

#pragma once

#include "../include/ui16.h"

#include <bmp.h>
#include <stdlib.h>

typedef struct
{
    void (*drawRect)(int x, int y, int w, int h, unsigned int color, int radius);
    void (*drawText)(int x, int y, const char *text, unsigned int color, ui16_font_t font);
    void (*measureText)(const char *text, ui16_font_t font, int *out_width, int *out_height);
} ui16_renderer_t;

const ui16_renderer_t *ui16__softwareRenderer(void);

ui16_node_t *ui16__attachNode(ui16_node_kind_t node_kind, ui16_style_t node_style, const char *node_text);
ui16_node_t *ui16__currentParent(void);
ui16_node_t *ui16__rootNode(void);

void *ui16__alloc(unsigned long byte_count);
void ui16__setCurrentParent(ui16_node_t *parent_node);
void ui16__computeLayout(int screen_width ,int screen_height, const ui16_renderer_t *renderer);
void ui16__renderTree(const ui16_renderer_t *renderer);
void ui16__setTargetBuffer(unsigned int *pixel_buffer, int buffer_width, int buffer_height);
void ui16__inputEndFrame(void);

void ui16__setBufferPixel(int x, int y, unsigned int color);
unsigned int ui16__getBufferPixel(int x, int y);

extern const bmp_target_t ui16__bmp_target;

int ui16__targetBufferWidth(void);
int ui16__targetBufferHeight(void);
int ui16__mouseX(void);
int ui16__mouseY(void);
int ui16__mouseDown(void);

ui16_style_t ui16__genericStyle(void);
ui16_font_t ui16__resolveFont(ui16_node_t *node);

#define UI16_GLYPH_WIDTH 8
#define UI16_GLYPH_HEIGHT 12

unsigned short ui16__glyphRow(ui16_font_t font, unsigned char character, int row);