/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2026 sulfurLabs
 *
 * PROJECT: s4
 * FILE: font.c
 *
 */

#include "ui16_priv.h"

#include "data/font8x12.h"
#include "data/font8x12_bold.h"

#include <psf.h>

static psf_font_t ui16_psf_font;
static const char *ui16_psf_path = 0;
static int ui16_psf_loaded = 0;

int ui16__psfLoad(const char *path)
{
    if (ui16_psf_loaded && ui16_psf_path == path) return 1;
    if (ui16_psf_loaded)
    {
        psf_free(&ui16_psf_font);
        ui16_psf_loaded = 0;
        ui16_psf_path = 0;
    }

    if (psf_load(path, &ui16_psf_font) != 0) return 0;

    ui16_psf_path = path;
    ui16_psf_loaded = 1;

    return 1;
}

int ui16__psfWidth(void)
{
    if (!ui16_psf_loaded) return 0;
    return psf_width(&ui16_psf_font);
}

int ui16__psfHeight(void)
{
    if (!ui16_psf_loaded) return 0;
    return psf_height(&ui16_psf_font);
}

uint32_t ui16__glyphRow(
    ui16_font_t font,
    unsigned char character,
    int row
) {
    if (font.kind == UI16_FONT_PSF)
    {
        int glyph = psf_glyph_index(
            &ui16_psf_font,
            (uint32_t)character
        );

        if (!ui16__psfLoad(font.path)) return 0;
        if (glyph < 0) return 0;

        return psf_glyph_row_bits(
            &ui16_psf_font,
            (uint32_t) glyph,
            row
        );
    }

    if (row < 0 || row >= UI16_GLYPH_HEIGHT) return 0;

    unsigned char glyph_index = character & 0x7F;

    if (font.kind == UI16_FONT_BOLD) return font_8x12_bold[glyph_index][row];

    return font_8x12[glyph_index][row];
}