/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2026 sulfurLabs
 *
 * PROJECT: s4
 * FILE: input.c
 *
 */

#include "input.h"
#include "../win/win.h"
#include "../cursor/cursor.h"
#include "../ipc/ipc.h"
#include "../taskbar/taskbar.h"
#include "../taskbar/startmenu.h"
#include "../cfg.h"

#include <stdio.h>
#include <unistd.h>

#define KBD_MOD_SHIFT (1 << 0)
#define KBD_MOD_CTRL (1 << 1)

static int g_last_btn = 0;
static int g_last_rbtn = 0;
static int g_last_mbtn = 0;
static int g_drag_idx = -1;
static int g_drag_ox = 0;
static int g_drag_oy = 0;

static int g_resize_last_cw = 0;
static int g_resize_last_ch = 0;

// state
static int g_resize_idx  = -1;
static int g_resize_edge = RESIZE_NONE;
// why r coordinates so hard
static int g_resize_sx   	= 0;   // mouse x at resize start
static int g_resize_sy   	= 0;   // mouse y at resize start
static int g_resize_wx   	= 0;   // window x at resize start
static int g_resize_wy   	= 0;   // window y at resize start
static int g_resize_ww   	= 0;   // window w at resize start
static int g_resize_wh   	= 0;   // window h at resize start

// rubber band state
static int g_band_active = 0;
static int g_band_sx = 0;
static int g_band_sy = 0;

static pid_t g_focused_pid = 0;
static int g_focused_idx = -1;

static pid_t g_hover_pid = 0;
static int g_hover_btn = 0; // 1 close, 2 max
static int g_tb_hover_id = 0;

drag_info_t g_input_drag_prev;

static int g_scr_w = 1280, g_scr_h = 720;
void input_set_screen_size(int w, int h)
{
	printf(":: input: set screensize, w: %d h: %d\n", w, h);
	g_scr_w = w;
	g_scr_h = h;
}

// minimum window size
//#define WIN_MIN_W 80
//#define WIN_MIN_H 40

int win_get_resize_edge(int idx, int mx, int my)
{
    dt_win_t *w = win_get(idx);

    if (!w) return RESIZE_NONE;
    if (w->style & DT_NORESIZE) return RESIZE_NONE;

    int wx = w->x;
    int wy = w->y;
    int ww = w->w;
    int wh = w->h;
    int edge = RESIZE_NONE;

    // kinda looks ugly but is readable
    int on_left   = (mx >= wx                    && mx < wx + RESIZE_ZONE );
    int on_right  = (mx >= wx + ww - RESIZE_ZONE && mx < wx + ww          );
    int on_top    = (my >= wy                    && my < wy + RESIZE_ZONE );
    int on_bottom = (my >= wy + wh - RESIZE_ZONE && my < wy + wh          );

    // must actually be inside or on the window frame
    if (
    	mx <  wx      ||
    	mx >= wx + ww ||
     	my <  wy      ||
      	my >= wy + wh
    ) return RESIZE_NONE;

    if (on_left)   edge |= RESIZE_LEFT;
    if (on_right)  edge |= RESIZE_RIGHT;
    if (on_top)    edge |= RESIZE_TOP;
    if (on_bottom) edge |= RESIZE_BOTTOM;


    return edge;
}

static cur_type_t edge_to_cursor(int edge)
{
    int is_lr = (edge & RESIZE_LEFT) || (edge & RESIZE_RIGHT);
    int is_tb = (edge & RESIZE_TOP)  || (edge & RESIZE_BOTTOM);

    if (is_lr && !is_tb) return CUR_TYPE_HRESIZE;
    if (is_tb && !is_lr) return CUR_TYPE_VRESIZE;

    // diagonal
    int nwse =
		((edge & RESIZE_LEFT)    &&
		 (edge & RESIZE_TOP))    ||
		((edge & RESIZE_RIGHT)   &&
		 (edge & RESIZE_BOTTOM))
    ;

    if (nwse) return CUR_TYPE_DRESIZE_NWSE;

    return CUR_TYPE_DRESIZE_NESW;
}

void input_init(void)
{
	printf(":: init input system\n");
    g_last_btn = 0;
    g_drag_idx = -1;
    g_resize_idx = -1;
    g_resize_edge = RESIZE_NONE;
    g_band_active = 0;
    g_input_drag_prev.valid = 0;
    g_focused_pid = 0;
    g_focused_idx = -1;
}

void input_frame_begin(input_state_t *is)
{
	is->sel_px1 = is->sel_x1;
    is->sel_py1 = is->sel_y1;
}

static int handle_one(mouse_state_t *ev, input_state_t *is)
{
	int mx = ev->abs_x;
	int my = ev->abs_y;
	int btn = ev->buttons & 1;
	int rbtn = ev->buttons & 2;
    int changed = 0;

    is->cx = mx;
    is->cy = my;

    int focus_changed_this_event = 0;

    if (!btn && g_drag_idx < 0 && g_resize_idx < 0 && !g_band_active)
    {
        cur_type_t hov_cur = CUR_TYPE_NORMAL;
        for (int i = DT_WIN_MAX - 1; i >= 0; i--)
        {
            dt_win_t *wn = win_get(i);
            if (!wn) continue;
            if (!win_hit(i, mx, my)) continue;

            int edge = win_get_resize_edge(i, mx, my);

            if (edge != RESIZE_NONE)
            {
                hov_cur = edge_to_cursor(edge);
            }
            break;
        }
        cur_set_type(hov_cur);

        int hov_idx = -1, hov_z = -1;
        for (int i = 0; i < DT_WIN_MAX; i++)
        {
            dt_win_t *wn = win_get(i);
            if (!wn) continue;
            if (win_hit(i, mx, my) && wn->z > hov_z) { hov_z = wn->z; hov_idx = i; }
        }

        pid_t new_hover_pid = 0;
        int new_hover_btn = 0;
        if (hov_idx >= 0)
        {
            dt_win_t *wn = win_get(hov_idx);
            new_hover_pid = wn ? wn->pid : 0;

            if (win_hit_close(hov_idx, mx, my)) new_hover_btn = 1;
            else if (win_hit_maximize(hov_idx, mx, my)) new_hover_btn = 2;
            else if (win_hit_minimize(hov_idx, mx, my)) new_hover_btn = 3;
        }

        if (new_hover_btn != g_hover_btn || new_hover_pid != g_hover_pid)
        {
            g_hover_btn = new_hover_btn;
            g_hover_pid = new_hover_pid;
            changed = 1;
        }
    }

    // button press
    if (btn && !g_last_btn)
    {
	    if (taskbar_click(mx, my)) {
	        g_last_btn = btn;
	        return 0;
	    }
        if (startmenu_is_open()) startmenu_close();
        int top_idx = -1, top_z = -1;
        for (int i = 0; i < DT_WIN_MAX; i++)
        {
            dt_win_t *wn = win_get(i);
            if (!wn) continue;
            if (win_hit(i, mx, my) && wn->z > top_z)
            {
                top_z = wn->z; top_idx = i;
            }
        }
        if (top_idx >= 0)
        {
            dt_win_t *tw = win_get(top_idx);
            if (tw && tw->pid != g_focused_pid)
            {
                ipc_clear_input(g_focused_pid);
                g_focused_pid = tw->pid;
                g_focused_idx = top_idx;
                focus_changed_this_event = 1;
            }

            win_focus(top_idx);
            changed = 1;

            int edge = win_get_resize_edge(top_idx, mx, my);

            if (win_hit_maximize(top_idx, mx, my))
            {
                dt_win_t *wn = win_get(top_idx);
                if (wn)
                {
                    g_input_drag_prev.valid = 1;
                    g_input_drag_prev.pid = wn->pid;
                    g_input_drag_prev.wx  = wn->x;
                    g_input_drag_prev.wy  = wn->y;
                    g_input_drag_prev.ww  = wn->w;
                    g_input_drag_prev.wh  = wn->h;
                }
                win_toggle_maximize(top_idx, g_scr_w, g_scr_h, TB_H);
                changed = 1;
            } else if (win_hit_close(top_idx, mx, my) || win_hit_minimize(top_idx, mx, my))
            {
                dt_win_t *wn = win_get(top_idx);
                if (wn) {
                    g_input_drag_prev.valid = 1;
                    g_input_drag_prev.pid   = wn->pid;
                    g_input_drag_prev.wx    = wn->x;
                    g_input_drag_prev.wy    = wn->y;
                    g_input_drag_prev.ww    = wn->w;
                    g_input_drag_prev.wh    = wn->h;
                    win_remove(wn->pid);

                    g_focused_pid = 0;
                    g_focused_idx = -1;
                    focus_changed_this_event = 1;
                }
            } else if (edge != RESIZE_NONE && !(win_get(top_idx) && (win_get(top_idx)->style & DT_NOMOVE) && edge == RESIZE_NONE))
            {
                #if DT_ENABLE_RESIZING && !ENABLE_TILING
                    // start resize
                    dt_win_t *wn = win_get(top_idx);
                    if (wn) {
                        g_resize_idx  = top_idx;
                        g_resize_edge = edge;
                        g_resize_sx   = mx;
                        g_resize_sy   = my;
                        g_resize_wx   = wn->x;
                        g_resize_wy   = wn->y;
                        g_resize_ww   = wn->w;
                        g_resize_wh   = wn->h;
                        g_resize_last_cw = wn->home_cw;
                        g_resize_last_ch = wn->home_ch;
                        cur_set_type(edge_to_cursor(edge));
                    }
                #endif
            } else if (win_hit_title(top_idx, mx, my))
            {
                #if !ENABLE_TILING
                    dt_win_t *wn = win_get(top_idx);
                    if (wn && !(wn->style & DT_NOMOVE) && !(wn->style & DT_NOTITLE))
                    {
                        g_drag_idx = top_idx;
                        g_drag_ox  = mx - wn->x;
                        g_drag_oy  = my - wn->y;
                    }
                #endif
            }
        } else
        {
            // click on empty desktop, start rubber band
            g_band_active = 1;
            g_band_sx = mx;
            g_band_sy = my;
            is->sel_active = 1;
            is->sel_x0 = mx;
            is->sel_y0 = my;
            is->sel_x1 = mx;
            is->sel_y1 = my;
            changed = 1;
        }
    }

    if (!btn)
    {
        if (g_resize_idx >= 0)
        {
            g_resize_idx  = -1;
            g_resize_edge = RESIZE_NONE;
            cur_set_type(CUR_TYPE_NORMAL);
            changed = 1;
        }
        if (g_band_active)
        {
            g_band_active = 0;
            is->sel_active = 0;
            changed = 1;
        }
        g_drag_idx = -1;
    }

    // drag move
    if (btn && g_drag_idx >= 0)
    {
        int nx = mx - g_drag_ox;
        int ny = my - g_drag_oy;
        if (nx < 0) nx = 0;
        if (ny < 0) ny = 0;

        win_move(g_drag_idx, nx, ny);
        changed = 1;
    }

    // resize drag
    if (btn && g_resize_idx >= 0)
    {
        dt_win_t *wn = win_get(g_resize_idx);
        if (wn)
        {
            int dx = mx - g_resize_sx;
            int dy = my - g_resize_sy;
            int nx = 	  g_resize_wx;
            int ny = 	  g_resize_wy;
            int nw = 	  g_resize_ww;
            int nh = 	  g_resize_wh;

            if (g_resize_edge & RESIZE_RIGHT)  nw = g_resize_ww + dx;
            if (g_resize_edge & RESIZE_BOTTOM) nh = g_resize_wh + dy;
            if (g_resize_edge & RESIZE_LEFT) {
                nw = g_resize_ww - dx;
                nx = g_resize_wx + dx;
            }
            if (g_resize_edge & RESIZE_TOP) {
                nh = g_resize_wh - dy;
                ny = g_resize_wy + dy;
            }

            if (nw < WIN_MIN_W) {
                if (g_resize_edge & RESIZE_LEFT) nx = g_resize_wx + g_resize_ww - WIN_MIN_W;
                nw = WIN_MIN_W;
            }
            if (nh < WIN_MIN_H) {
                if (g_resize_edge & RESIZE_TOP) ny = g_resize_wy + g_resize_wh - WIN_MIN_H;
                nh = WIN_MIN_H;
            }
            if (nx < 0) nx = 0;
            if (ny < 0) ny = 0;

            // save old rect for clearing
            g_input_drag_prev.valid = 1;
            g_input_drag_prev.pid   = wn->pid;
            g_input_drag_prev.wx    = wn->x;
            g_input_drag_prev.wy    = wn->y;
            g_input_drag_prev.ww    = wn->w;
            g_input_drag_prev.wh    = wn->h;

            wn->x = nx;
            wn->y = ny;
            wn->w = nw;
            wn->h = nh;

            // recompute content region
            unsigned int s = wn->style;
            if (s & DT_POPUP) {
                wn->home_cx = nx + 1;
                wn->home_cy = ny + 1;
                wn->home_cw = nw - 2;
                wn->home_ch = nh - 2;
            } else if (s & DT_NOTITLE) {
                wn->home_cx = nx + DT_BORDER;
                wn->home_cy = ny + DT_BORDER;
                wn->home_cw = nw - DT_BORDER * 2;
                wn->home_ch = nh - DT_BORDER * 2;
            } else {
                wn->home_cx = nx + DT_BORDER;
                wn->home_cy = ny + DT_TITLE_H + 1;
                wn->home_cw = nw - DT_BORDER * 2;
                wn->home_ch = nh - DT_TITLE_H - 1 - DT_BORDER;
            }
            wn->orig_cx = wn->home_cx;
            wn->orig_cy = wn->home_cy;
            wn->orig_cw = wn->home_cw;
            wn->orig_ch = wn->home_ch;

            if (wn->home_cw != g_resize_last_cw || wn->home_ch != g_resize_last_ch)
            {
                g_resize_last_cw = wn->home_cw;
                g_resize_last_ch = wn->home_ch;

                dt_event_t rev;
                if (ipc_make_resize_event(wn->home_cw, wn->home_ch, &rev)) ipc_dispatch_event(wn->pid, &rev);

                ipc_publish_window_size(wn->pid, wn->home_cw, wn->home_ch);
            }

            changed = 1;
        }
    }

    // rubber band update
    if (btn && g_band_active)
    {
        is->sel_active = 1;
        is->sel_x1 = mx;
        is->sel_y1 = my;
        changed = 1;
    }

    if (g_focused_idx >= 0 && !focus_changed_this_event)
    {
        dt_win_t *fw = win_get(g_focused_idx);
        if (fw && fw->pid == g_focused_pid)
        {
            unsigned char btns = 0;
            if (btn)  btns |= DT_BTN_LEFT;
            if (rbtn) btns |= DT_BTN_RIGHT;

            dt_event_t mev;
            if (
            	ipc_make_mouse_event(g_focused_idx, mx, my, btns, &mev)
            ) ipc_dispatch_event(g_focused_pid, &mev);
        }
    }

    g_last_btn = btn;
    return changed;
}

int input_drain(int mfd, input_state_t *is)
{
    input_event_t ev;
    int got = 0;
    int mx = is->cx;
    int my = is->cy;
    int btn_left = g_last_btn;
    int btn_right  = g_last_rbtn;
    int btn_middle = g_last_mbtn;

    while ((int)read(mfd, &ev, sizeof(ev)) == (int)sizeof(ev))
    {
        //if (handle_one(&ev, is)) is->win_changed = 1;
        got = 1;
        if (ev.type == INPUT_EV_REL)
        {
            #if RENDERER_SCALING_ENABLED
                if (ev.code == INPUT_REL_X) mx += ev.value * RENDERER_SUPERSAMPLING_FACTOR;
                if (ev.code == INPUT_REL_Y) my += ev.value * RENDERER_SUPERSAMPLING_FACTOR;
            #else
                if (ev.code == INPUT_REL_X) mx += ev.value;
                if (ev.code == INPUT_REL_Y) my += ev.value;
            #endif
        } else if (ev.type == INPUT_EV_KEY)
        {
            if (ev.code == INPUT_BTN_LEFT)  btn_left = ev.value;
            else if (ev.code == INPUT_BTN_RIGHT)  btn_right  = ev.value;
            else if (ev.code == INPUT_BTN_MIDDLE) btn_middle = ev.value;
        }
    }

    if (!got) return 0;
    if (mx < 0) mx = 0;
    if (my < 0) my = 0;
    if (mx >= g_scr_w) mx = g_scr_w - 1;
    if (my >= g_scr_h) my = g_scr_h - 1;

    g_last_rbtn = btn_right;
    g_last_mbtn = btn_middle;

    mouse_state_t synth =
    {
    	.abs_x = mx,
     	.abs_y = my,
        .buttons =
            (btn_left   ? DT_BTN_LEFT   : 0) |
            (btn_right  ? DT_BTN_RIGHT  : 0) |
            (btn_middle ? DT_BTN_MIDDLE : 0)
    };

    if (handle_one(&synth, is)) is->win_changed = 1;

    return 1;
}

int input_drain_keyboard(int kfd)
{
    input_event_t ev;
    int got = 0;

    while ((int)read(kfd, &ev, sizeof(ev)) == (int)sizeof(ev))
    {
        got = 1;

        if (
            ev.value != 0 &&
            (ev.modifiers & INPUT_MOD_SHIFT) &&
            (ev.code == 's' || ev.code == 'S' || ev.code == 0x1F)
        ) {
            //spawn("/system/bin/smenu.elf");
            continue;
        }

        #if ENABLE_TILING
            if (
                ev.value != 0 &&
                (ev.modifiers & INPUT_MOD_SHIFT) &&
                (ev.modifiers & INPUT_MOD_CTRL)  &&
                g_focused_pid > 0
            ) {
                int dx = 0, dy = 0;

                if (ev.code == INPUT_KEY_LEFT) dx = -1;
                else if (ev.code == INPUT_KEY_RIGHT) dx = 1;
                else if (ev.code == INPUT_KEY_UP) dy = -1;
                else if (ev.code == INPUT_KEY_DOWN) dy = 1;

                if (dx != 0 || dy != 0)
                {
                    win_tile_move(g_focused_pid, dx, dy, g_scr_w, g_scr_h, TB_H);
                    continue;
                }
            }
        #endif

        if (g_focused_pid <= 0 || g_focused_idx < 0) continue;

        dt_win_t *fw = win_get(g_focused_idx);
        if (!fw || fw->pid != g_focused_pid) continue;

        dt_event_t kev;
        if (ipc_make_key_event(
            (unsigned int)ev.code,
            ev.modifiers,
            (unsigned char)(ev.value != 0),
            &kev
        )) ipc_dispatch_event(g_focused_pid, &kev);
    }

    return got;
}
