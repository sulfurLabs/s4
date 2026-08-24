/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2026 sulfurLabs
 *
 * PROJECT: s4
 * FILE: softrender.c
 *
 */

#include "ui16_priv.h"

static unsigned int *target_buffer = 0;
static int target_buffer_width = 0;
static int target_buffer_height = 0;

void ui16__setTargetBuffer(unsigned int *pixel_buffer, int buffer_width, int buffer_height)
{
    target_buffer = pixel_buffer;
    target_buffer_width = buffer_width;
    target_buffer_height = buffer_height;
}

int ui16__targetBufferWidth(void)
{
    return target_buffer_width;
}

int ui16__targetBufferHeight(void)
{
    return target_buffer_height;
}

static int ui16__stringLength(const char *text)
{
    int character_count = 0;
    while (text[character_count]) character_count++;
    return character_count;
}

static void ui16__softwareDrawRect(int x, int y, int w, int h, unsigned int color, int radius)
{
    (void)radius;
    // if the desktop runs on bilinear scaling then radius looks good but
    // otherwise it looks pixelish

    if (!target_buffer) return;

    for (int row = 0; row < h; row++)
    {
        int pixel_y = y + row;
        if (pixel_y < 0 || pixel_y >= target_buffer_height) continue;

        for (int col = 0; col < w; col++)
        {
            int pixel_x = x + col;
            if (pixel_x < 0 || pixel_x >= target_buffer_width) continue;

            target_buffer[pixel_y * target_buffer_width + pixel_x] = color;
        }
    }
}

static void ui16__setPixel(int x, int y, unsigned int color)
{
    if (!target_buffer) return;
    if (x < 0 || x >= target_buffer_width) return;
    if (y < 0 || y >= target_buffer_height) return;

    target_buffer[y * target_buffer_width + x] = color;
}

void ui16__setBufferPixel(int x, int y, unsigned int color)
{
    ui16__setPixel(x, y, color);
}

static unsigned int ui16__getPixel(int x, int y)
{
    if (!target_buffer) return 0;
    if (x < 0 || x >= target_buffer_width) return 0;
    if (y < 0 || y >= target_buffer_height) return 0;

    return target_buffer[y * target_buffer_width + x];
}

unsigned int ui16__getBufferPixel(int x, int y)
{
    ui16__getBufferPixel(x, y);
}

static void ui16__softwareDrawText(int x, int y, const char *text, unsigned int color, ui16_font_t font)
{
    if (!target_buffer) return;

    int cursor_x = x;
    int row_index = 0;
    int character_index = 0;
    int col_index = 0;

    for (character_index = 0; text[character_index]; character_index++)
    {
        unsigned char current_character = (unsigned char)text[character_index];

        for (row_index = 0; row_index < UI16_GLYPH_HEIGHT; row_index++)
        {
            unsigned short row_bits = ui16__glyphRow(font, current_character, row_index);

            for (col_index = 0; col_index < UI16_GLYPH_WIDTH; col_index++)
            {
                if (row_bits & (1u << col_index)) ui16__setPixel(cursor_x + col_index, y + row_index, color);
            }
        }

        cursor_x += UI16_GLYPH_WIDTH;
    }
}

static void ui16__softwareMeasureText(const char *text, ui16_font_t font, int *out_width, int *out_height)
{
    (void)font;

    int character_count = ui16__stringLength(text);

    *out_width = character_count * UI16_GLYPH_WIDTH;
    *out_height = UI16_GLYPH_HEIGHT;
}

static const ui16_renderer_t software_renderer_instance =
{
    .drawRect = ui16__softwareDrawRect,
    .drawText = ui16__softwareDrawText,
    .measureText = ui16__softwareMeasureText,
};

const ui16_renderer_t *ui16__softwareRenderer(void)
{
    return &software_renderer_instance;
}