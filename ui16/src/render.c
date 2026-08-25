/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2026 sulfurLabs
 *
 * PROJECT: s4
 * FILE: render.c
 *
 */

#include "ui16.h"
#include "ui16_priv.h"

#define UI16_BUTTON_DEFAULT_BG 0xFF3C3C3Cu
#define UI16_MAX_OVERLAY_CHILDREN 16

static unsigned int ui16__tint(unsigned int color, int delta)
{
    int a = (int)((color >> 24) & 0xFF);
    int r = (int)((color >> 16) & 0xFF) + delta;
    int g = (int)((color >> 8) & 0xFF) + delta;
    int b = (int)(color  & 0xFF) + delta;

    if (r < 0) r = 0;
    if (r > 255) r = 255;

    if (g < 0) g = 0;
    if (g > 255) g = 255;

    if (b < 0) b = 0;
    if (b > 255) b = 255;

    return ((unsigned int)a << 24) | ((unsigned int)r << 16) | ((unsigned int)g << 8) | (unsigned int)b;
}

static int ui16__nodeHit(ui16_node_t *node, int x, int y)
{
    if (x < 0 || y < 0) return 0;

    return
        x >= node->box_x && x < node->box_x + node->box_width &&
        y >= node->box_y && y < node->box_y + node->box_height
    ;
}

static void ui16__renderNode(ui16_node_t *node, const ui16_renderer_t *renderer);

static void ui16__renderSelf(ui16_node_t *node, const ui16_renderer_t *renderer)
{
    if (node->kind == UI16_NODE_BUTTON && renderer->drawRect)
    {
        unsigned int bg = node->style.background != 0 ? node->style.background : UI16_BUTTON_DEFAULT_BG;
        int hovered = ui16__nodeHit(node, ui16__mouseX(), ui16__mouseY());

        if (hovered && ui16__mouseDown()) bg = ui16__tint(bg, -30);
        else if (hovered) bg = ui16__tint(bg, 22);

        renderer->drawRect(node->box_x, node->box_y, node->box_width, node->box_height, bg, node->style.radius);
    }
    else if (node->style.background != 0 && renderer->drawRect)
    {
        renderer->drawRect(
            node->box_x,
            node->box_y,
            node->box_width,
            node->box_height,
            node->style.background,
            node->style.radius
        );
    }

    if (node->kind == UI16_NODE_IMAGE)
    {
        const bmp_image_t *img = ui16__imageLoad(node->text);
        if (img)
        {
            bmp_draw_scaled(
                &ui16__bmp_target,
                img,
                node->box_x,
                node->box_y,
                node->box_width,
                node->box_height
            );
        }
    }

    if (node->style.border_width > 0 && renderer->drawRect)
    {
        int bw = node->style.border_width;
        unsigned int bc = node->style.border_color;

        renderer->drawRect(node->box_x, node->box_y, node->box_width, bw, bc, 0);
        renderer->drawRect(node->box_x, node->box_y + node->box_height - bw, node->box_width, bw, bc, 0);
        renderer->drawRect(node->box_x, node->box_y, bw, node->box_height, bc, 0);
        renderer->drawRect(node->box_x + node->box_width - bw, node->box_y, bw, node->box_height, bc, 0);
    }

    if (
        (node->kind == UI16_NODE_LABEL || node->kind == UI16_NODE_BUTTON) &&
        node->text &&
        renderer->drawText
    ) {
        int text_x = node->box_x;
        int text_y = node->box_y;

        if (node->kind == UI16_NODE_BUTTON && renderer->measureText)
        {
            int text_w = 0;
            int text_h = 0;
            renderer->measureText(node->text, ui16__resolveFont(node), &text_w, &text_h);

            text_x = node->box_x + (node->box_width - text_w) / 2;
            text_y = node->box_y + (node->box_height - text_h) / 2;

            if (text_x < node->box_x) text_x = node->box_x;
            if (text_y < node->box_y) text_y = node->box_y;
        }

        renderer->drawText(text_x, text_y, node->text, node->style.color, ui16__resolveFont(node));
    }
}

static void ui16__renderOverlays(ui16_node_t *node, const ui16_renderer_t *renderer)
{
    ui16_node_t *overlays[UI16_MAX_OVERLAY_CHILDREN];
    int count = 0;
    int j;

    for (
        ui16_node_t *child = node->first_child;
        child && count < UI16_MAX_OVERLAY_CHILDREN;
        child = child->next_sibling
    ) {
        if (child->style.position == UI16_POSITION_ABSOLUTE) overlays[count++] = child;
    }

    for (int i = 1; i < count; i++)
    {
        ui16_node_t *key = overlays[i];
        j = i - 1;

        while (j >= 0 && overlays[j]->style.layer > key->style.layer)
        {
            overlays[j + 1] = overlays[j];
            j--;
        }
        overlays[j + 1] = key;
    }

    for (int i = 0; i < count; i++) ui16__renderNode(overlays[i], renderer);
}

static void ui16__renderNode(ui16_node_t *node, const ui16_renderer_t *renderer)
{
    ui16__renderSelf(node, renderer);

    for (ui16_node_t *child = node->first_child; child; child = child->next_sibling)
    {
        if (child->style.position == UI16_POSITION_ABSOLUTE) continue;
        ui16__renderNode(child, renderer);
    }

    ui16__renderOverlays(node, renderer);
}

void ui16__renderTree(const ui16_renderer_t *renderer)
{
    ui16_node_t *root_node = ui16__rootNode();

    if (!root_node || !renderer) return;

    ui16__renderNode(root_node, renderer);
}