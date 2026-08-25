/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2026 sulfurLabs
 *
 * PROJECT: s4
 * FILE: layout.c
 *
 */

#include "ui16.h"
#include "ui16_priv.h"

#define UI16_MAX_WRAP_LINES 16

static int ui16__clampInt(int value, int min_value, int max_value)
{
    if (min_value >= 0 && value < min_value) value = min_value;
    if (max_value >= 0 && value > max_value) value = max_value;
    return value;
}

static void ui16__measureNode(ui16_node_t *node, const ui16_renderer_t *renderer)
{
    for (ui16_node_t *child = node->first_child; child; child = child->next_sibling)
    {
        ui16__measureNode(child, renderer);
    }

    int natural_width = 0;
    int natural_height = 0;

    if (
        (node->kind == UI16_NODE_LABEL || node->kind == UI16_NODE_BUTTON) &&
        node->text &&
        renderer && renderer->measureText
    ) {
        renderer->measureText(node->text, ui16__resolveFont(node), &natural_width, &natural_height);
    }
    else if (node->kind == UI16_NODE_IMAGE && node->text)
    {
        const bmp_image_t *img = ui16__imageLoad(node->text);
        if (img)
        {
            natural_width = img->width;
            natural_height = img->height;
        }
    }

    int is_row_layout = (node->style.layout == UI16_LAYOUT_ROW);
    int main_sum = 0;
    int cross_max = 0;
    int child_count = 0;

    for (ui16_node_t *child = node->first_child; child; child = child->next_sibling)
    {
        if (child->style.position == UI16_POSITION_ABSOLUTE) continue;

        int child_w = child->natural_width;
        int child_h = child->natural_height;

        if (child->style.width.kind == UI16_SIZE_PIXELS) child_w = child->style.width.value;
        if (child->style.height.kind == UI16_SIZE_PIXELS) child_h = child->style.height.value;
        child_w = ui16__clampInt(child_w, child->style.min_width, child->style.max_width);
        child_h = ui16__clampInt(child_h, child->style.min_height, child->style.max_height);

        int child_main = (is_row_layout ? child_w : child_h) + child->style.margin * 2;
        int child_cross = (is_row_layout ? child_h : child_w) + child->style.margin * 2;

        main_sum += child_main;

        if (child_cross > cross_max) cross_max = child_cross;

        child_count++;
    }

    if (child_count > 0)
    {
        if (child_count > 1) main_sum += node->style.gap * (child_count - 1);

        if (is_row_layout)
        {
            natural_width = main_sum;
            natural_height = cross_max;
        }
        else
        {
            natural_height = main_sum;
            natural_width = cross_max;
        }
    }

    natural_width += node->style.padding * 2;
    natural_height += node->style.padding * 2;

    natural_width = ui16__clampInt(natural_width, node->style.min_width, node->style.max_width);
    natural_height = ui16__clampInt(natural_height, node->style.min_height, node->style.max_height);

    node->natural_width = natural_width < 0 ? 0 : natural_width;
    node->natural_height = natural_height < 0 ? 0 : natural_height;
}

static int ui16__resolveSize(
    ui16_size_t node_size,
    int available_space,
    ui16_node_t *node,
    int is_width_axis
) {
    int value;

    switch (node_size.kind)
    {
        case UI16_SIZE_PIXELS:
            value = node_size.value;

            break;

        case UI16_SIZE_PERCENT:
            value = (available_space * node_size.value) / 100;
            break;

        case UI16_SIZE_AUTOSIZE:
            value = is_width_axis ? node->natural_width : node->natural_height;
            break;

        case UI16_SIZE_FILL:
            //
        default:
            value = available_space;
            break;
    }

    int min_value = is_width_axis ? node->style.min_width : node->style.min_height;
    int max_value = is_width_axis ? node->style.max_width : node->style.max_height;

    value = ui16__clampInt(value, min_value, max_value);
    if (value < 0) value = 0;

    return value;
}

typedef struct
{
    int main_size;
    int cross_size;
    int count;
    int fill_count;
} ui16__line_t;

static void ui16__layoutChildren(ui16_node_t *parent_node, const ui16_renderer_t *renderer)
{
    int content_x = parent_node->box_x + parent_node->style.padding;
    int content_y = parent_node->box_y + parent_node->style.padding;
    int content_width = parent_node->box_width - parent_node->style.padding * 2;
    int content_height = parent_node->box_height - parent_node->style.padding * 2;

    if (content_width < 0) content_width = 0;
    if (content_height < 0) content_height = 0;

    int is_row_layout = (parent_node->style.layout == UI16_LAYOUT_ROW);
    int main_axis_total = is_row_layout ? content_width : content_height;
    int wrap_enabled = parent_node->style.wrap ? 1 : 0;

    ui16__line_t lines[UI16_MAX_WRAP_LINES];
    for (int i = 0; i < UI16_MAX_WRAP_LINES; i++)
    {
        lines[i].main_size = 0;
        lines[i].cross_size = 0;
        lines[i].count = 0;
        lines[i].fill_count = 0;
    }
    int line_count = 1;

    for (ui16_node_t *child = parent_node->first_child; child; child = child->next_sibling)
    {
        if (child->style.position == UI16_POSITION_ABSOLUTE)
        {
            child->line_index = -1;
            continue;
        }

        ui16_size_t main_size_style = is_row_layout ? child->style.width : child->style.height;
        ui16_size_t cross_size_style = is_row_layout ? child->style.height : child->style.width;

        int child_main;
        if (main_size_style.kind == UI16_SIZE_FILL)
        {
            if (wrap_enabled)
            {
                int min_v = is_row_layout ? child->style.min_width : child->style.min_height;
                int max_v = is_row_layout ? child->style.max_width : child->style.max_height;

                child_main = ui16__clampInt(
                    is_row_layout ? child->natural_width : child->natural_height,
                    min_v,
                    max_v
                );
            }
            else
            {
                child_main = 0;
            }
        }
        else
        {
            child_main = ui16__resolveSize(
                main_size_style,
                main_axis_total,
                child,
                is_row_layout
            );
        }

        int child_cross = ui16__resolveSize(cross_size_style, is_row_layout ? content_height : content_width, child, !is_row_layout);
        int total_child_main = child_main + child->style.margin * 2;
        int total_child_cross = child_cross + child->style.margin * 2;
        int line_index = line_count - 1;
        int would_be_main = (lines[line_index].count > 0)
            ? lines[line_index].main_size + parent_node->style.gap + total_child_main
            : total_child_main
        ;

        if (
            wrap_enabled &&
            lines[line_index].count > 0 &&
            would_be_main > main_axis_total &&
            line_count < UI16_MAX_WRAP_LINES
        ) {
            line_count++;
            line_index++;
            would_be_main = total_child_main;
        }

        child->line_index = line_index;
        lines[line_index].main_size = would_be_main;
        lines[line_index].count++;

        if (main_size_style.kind == UI16_SIZE_FILL && !wrap_enabled) lines[line_index].fill_count++;
        if (total_child_cross > lines[line_index].cross_size) lines[line_index].cross_size = total_child_cross;
    }

    int line_cross_offset[UI16_MAX_WRAP_LINES];
    int running_cross = 0;
    for (int i = 0; i < line_count; i++)
    {
        line_cross_offset[i] = running_cross;
        running_cross += lines[i].cross_size;
        if (i < line_count - 1) running_cross += parent_node->style.gap;
    }


    int line_running_main[UI16_MAX_WRAP_LINES];
    int line_extra_gap[UI16_MAX_WRAP_LINES];
    int line_fill_share[UI16_MAX_WRAP_LINES];

    for (int i = 0; i < line_count; i++)
    {
        int free_space = main_axis_total - lines[i].main_size;
        if (free_space < 0) free_space = 0;

        line_fill_share[i] = (lines[i].fill_count > 0) ? (free_space / lines[i].fill_count) : 0;
        line_extra_gap[i] = 0;
        line_running_main[i] = 0;

        if (lines[i].fill_count == 0 && lines[i].count > 0)
        {
            switch (parent_node->style.justify_content)
            {
                case UI16_JUSTIFY_CENTER:
                    line_running_main[i] = free_space / 2;
                    break;

                case UI16_JUSTIFY_END:
                    line_running_main[i] = free_space;
                    break;

                case UI16_JUSTIFY_SPACE_BETWEEN:
                    line_extra_gap[i] = (lines[i].count > 1) ? (free_space / (lines[i].count - 1)) : 0;
                    break;

                case UI16_JUSTIFY_SPACE_AROUND:
                    line_extra_gap[i] = free_space / lines[i].count;
                    line_running_main[i] = line_extra_gap[i] / 2;
                    break;

                case UI16_JUSTIFY_SPACE_EVENLY:
                    line_extra_gap[i] = free_space / (lines[i].count + 1);
                    line_running_main[i] = line_extra_gap[i];
                    break;

                default:
                    break; // justify start
            }
        }
    }

    for (ui16_node_t *child = parent_node->first_child; child; child = child->next_sibling)
    {
        if (child->style.position == UI16_POSITION_ABSOLUTE) continue;

        int li = child->line_index;

        ui16_size_t main_size_style = is_row_layout ? child->style.width : child->style.height;
        ui16_size_t cross_size_style = is_row_layout ? child->style.height : child->style.width;

        int resolved_main;
        if (main_size_style.kind == UI16_SIZE_FILL)
        {
            if (wrap_enabled)
            {
                int min_v = is_row_layout ? child->style.min_width : child->style.min_height;
                int max_v = is_row_layout ? child->style.max_width : child->style.max_height;
                resolved_main = ui16__clampInt(is_row_layout ? child->natural_width : child->natural_height, min_v, max_v);
            } else
            {
                resolved_main = line_fill_share[li];
            }
        }
        else
        {
            resolved_main = ui16__resolveSize(main_size_style, main_axis_total, child, is_row_layout);
        }

        int cross_available = lines[li].cross_size - child->style.margin * 2;
        if (cross_available < 0) cross_available = 0;

        int resolved_cross;
        if (
            parent_node->style.align_items == UI16_ALIGN_STRETCH &&
            cross_size_style.kind == UI16_SIZE_AUTOSIZE
        ) {
            resolved_cross = cross_available;
        }
        else
        {
            resolved_cross = ui16__resolveSize(cross_size_style, is_row_layout ? content_height : content_width, child, !is_row_layout);
        }

        int cross_offset;
        switch (parent_node->style.align_items)
        {
            case UI16_ALIGN_CENTER:
                cross_offset = (cross_available - resolved_cross) / 2;
                break;

            case UI16_ALIGN_END:
                cross_offset = cross_available - resolved_cross;
                break;

            default: cross_offset = 0;
                break;
        }
        if (cross_offset < 0) cross_offset = 0;

        int main_pos = line_running_main[li] + child->style.margin;
        int cross_pos = line_cross_offset[li] + cross_offset + child->style.margin;

        if (is_row_layout)
        {
            child->box_x = content_x + main_pos;
            child->box_y = content_y + cross_pos;
            child->box_width = resolved_main;
            child->box_height = resolved_cross;
        }
        else
        {
            child->box_x = content_x + cross_pos;
            child->box_y = content_y + main_pos;
            child->box_width = resolved_cross;
            child->box_height = resolved_main;
        }

        line_running_main[li] += resolved_main + child->style.margin * 2 + parent_node->style.gap + line_extra_gap[li];

        if (child->first_child) ui16__layoutChildren(child, renderer);
    }

    for (ui16_node_t *child = parent_node->first_child; child; child = child->next_sibling)
    {
        if (child->style.position != UI16_POSITION_ABSOLUTE) continue;

        int resolved_w = ui16__resolveSize(child->style.width, content_width, child, 1);
        int resolved_h = ui16__resolveSize(child->style.height, content_height, child, 0);

        child->box_x = content_x + child->style.left;
        child->box_y = content_y + child->style.top;
        child->box_width  = resolved_w;
        child->box_height = resolved_h;

        if (child->first_child) ui16__layoutChildren(child, renderer);
    }
}

void ui16__computeLayout(int screen_width, int screen_height, const ui16_renderer_t *renderer)
{
    ui16_node_t *root_node = ui16__rootNode();

    ui16__measureNode(root_node, renderer);

    root_node->box_x = 0;
    root_node->box_y = 0;
    root_node->box_width = ui16__resolveSize(root_node->style.width, screen_width, root_node, 1);
    root_node->box_height = ui16__resolveSize(root_node->style.height, screen_height, root_node, 0);

    ui16__layoutChildren(root_node, renderer);
}