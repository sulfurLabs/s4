#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include "cfg.h"
#include "compositor/comp.h"
#include "render/render_target.h"
#include "bg/bg.h"
#include "win/win.h"
#include "render/render.h"
#include "cursor/cursor.h"
#include "input/input.h"
#include "taskbar/taskbar.h"
#include "shm/shm_host.h"
#include "cmd/cmd.h"
#include "ipc/ipc.h"
#include "wm/wm.h"
#include "fonts/fonts.h"

#include "../libpsf/psf.h"

#include <sys/fb.h>
#include <sys/input.h>

//#define DT_WBUF_PREFIX "/tmp/dt/wbuf_"
//#define DT_DIRTY_PFX "/tmp/dt/dirty_"

extern drag_info_t g_input_drag_prev;

void _itoa(int v, char *out)
{
    dt_ipc_itoa(v, out);
}

static void render_band(input_state_t *is)
{
    /*
    * RUBBERBAND FIX
        * by @offihito
    */
    if (!is->sel_active) return;

    // compute actual rect (supports all drag directions)
    int x0 = is->sel_x0 < is->sel_x1 ? is->sel_x0 : is->sel_x1;
    int y0 = is->sel_y0 < is->sel_y1 ? is->sel_y0 : is->sel_y1;
    int x1 = is->sel_x0 > is->sel_x1 ? is->sel_x0 : is->sel_x1;
    int y1 = is->sel_y0 > is->sel_y1 ? is->sel_y0 : is->sel_y1;
    int w = x1 - x0;
    int h = y1 - y0;

    if (w < 2 || h < 2) return;

    rt_damage_mark(x0, y0, w + 1, h + 1);

    int sw = rt_width();
    int sh = rt_height();

    for (int ry = y0 + 1; ry < y1 && ry < sh; ry++)
    {
        for (int rx = x0 + 1; rx < x1 && rx < sw; rx++)
        {
            unsigned int bg = rt_get(rx, ry);
            unsigned int br = (bg >> 16) & 0xFF;
            unsigned int bg2 = (bg >> 8) & 0xFF;
            unsigned int bb = bg & 0xFF;

            br = (br * 4 + 128) / 5;
            bg2 = (bg2 * 4 + 128) / 5;
            bb = (bb * 4 + 128) / 5;

            rt_set(rx, ry, 0xFF000000u | (br << 16) | (bg2 << 8) | bb);
        }
    }

    // top and bottom edges
    for (int rx = x0; rx <= x1 && rx < sw; rx++) {
        if (y0 >= 0 && y0 < sh) rt_set(rx, y0, BAND_BORDER);
        if (y1 >= 0 && y1 < sh) rt_set(rx, y1, BAND_BORDER);
    }
    // left and right edges
    for (int ry = y0; ry <= y1 && ry < sh; ry++) {
        if (x0 >= 0 && x0 < sw) rt_set(x0, ry, BAND_BORDER);
        if (x1 >= 0 && x1 < sw) rt_set(x1, ry, BAND_BORDER);
    }
}

int main(void)
{
    printf("\n:: starting s4 for " GREETING "...\n");
    printf(":: mkdir " DT_DIR "...\n");
    mkdir(DT_DIR, 0);
    mkdir(SYSTEM, 0);

    printf(":: reading framebuffer... (" FRAMEBUFFER_DEV ")\n");
    int fb = open(FRAMEBUFFER_DEV, O_RDWR);
    printf(":: reading mouse... (" MOUSE_DEV ")\n");
    int mfd = open(MOUSE_DEV, O_RDONLY);
    printf(":: reading keyboard... (" KEYBOARD_DEV ")\n");
    int kfd = open(KEYBOARD_DEV, O_RDONLY);

    if (fb < 0 || mfd < 0) return 1;

    fb_info_t info;
    printf(":: reading fbinfo...\n");
    ioctl(fb, FB_IOCTL_GET_INFO, &info);

    int scr_w = (int)info.width;
    int scr_h = (int)info.height;

    //TODO:
    // look for real display size
    if (scr_w <= 0) scr_w = 1024;
    if (scr_h <= 0) scr_h = 768;

    //TODO:
    // variable resolution

    int internal_w = scr_w;
    int internal_h = scr_h;
    #if RENDERER_SCALING_ENABLED
        internal_w = scr_w * RENDERER_SUPERSAMPLING_FACTOR;
        internal_h = scr_h * RENDERER_SUPERSAMPLING_FACTOR;
    #endif

    printf(":: internal size: %d x %d\n", internal_w, internal_h);

    input_set_screen_size(internal_w, internal_h);
    cmd_set_screen_size(internal_w, internal_h);
    ipc_set_screen_size(internal_w, internal_h);

    shm_host_init();
    ipc_init();
    input_init();
    comp_init(fb, internal_w, internal_h);
    bg_init(internal_w, internal_h);
    cur_init(fb, internal_w, internal_h);
    taskbar_init(internal_w, internal_h);

    fonts_deco_load(WINDOW_TITLE_FONT);

    bg_draw_full();
    comp_flush();

    printf(
        "loading s4 was a success!\n"
        "Welcome to s4!\n\n"
    );

    ioctl(fb, FB_IOCTL_VT_DISABLE, 0);

    input_state_t is;

    is.cx = internal_w / 2;
    is.cy = internal_h / 2;

    is.win_changed = 0;
    is.sel_active = 0;
    is._sel_was_active = 0;

    is.sel_x0 = 0;
    is.sel_y0 = 0;

    is.sel_x1 = 0;
    is.sel_y1 = 0;

    is.sel_px1 = 0;
    is.sel_py1 = 0;

    cur_draw_fb(is.cx, is.cy);
    ipc_publish_cursor(is.cx, is.cy);

    int poll_tick = 0;
    int first_frame = 1;

    #define POLL_INTERVAL 1

    for (;;)
    {
        int need_full = 0;
        int need_cur  = 0;
        cmd_result_t cr;
        if (first_frame)
        {
            need_full = 1;
            first_frame = 0;
        }

        cr.count = 0;
        cr.win_changed = 0;
        rt_damage_begin();
        poll_tick++;

        if (poll_tick >= POLL_INTERVAL)
        {
            poll_tick = 0;
            cmd_process(&cr);
        }

        if (wm_poll_client_damage()) need_full = 1;
        //int content_refreshed = refresh_dirty_win_bufs();
        //if (content_refreshed > 0) need_full = 1;
        //if (cr.win_changed) need_full = 1;

        input_frame_begin(&is);

        if (input_drain(mfd, &is))
        {
            need_cur = 1;
            ipc_publish_cursor(is.cx, is.cy);
        }
        input_drain_keyboard(kfd);

        int struct_changed = cr.win_changed || is.win_changed;
        if (cr.win_changed || is.win_changed)
        {
            need_full = 1;
            is.win_changed = 0;
        }

        int was_active = is._sel_was_active;
        is._sel_was_active = is.sel_active;

        if (is.sel_active || was_active) need_full = 1;

        if (need_full)
        {
            //refresh_win_bufs();
            comp_capture();

            {
                int px = cur_last_x();
                int py = cur_last_y();

                rt_damage_mark(px, py, cur_w(), cur_h());
            }

            cur_undo_from_backbuf();

            // rubberband fix; @offihito
            if (was_active)
            {
                int x0 = is.sel_x0 < is.sel_px1 ? is.sel_x0 : is.sel_px1;
                int y0 = is.sel_y0 < is.sel_py1 ? is.sel_y0 : is.sel_py1;
                int x1 = is.sel_x0 > is.sel_px1 ? is.sel_x0 : is.sel_px1;
                int y1 = is.sel_y0 > is.sel_py1 ? is.sel_y0 : is.sel_py1;

                wm_clear_rect_dirty(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
            }

            wm_sync_home_to_current();
            wm_clear_cmd_rects(&cr);
            wm_clear_prev_drag_rect();
            render_band(&is);

            if (struct_changed || is.sel_active || was_active)
            {
                render_all(is.cx, is.cy);
                for (int i = 0; i < DT_WIN_MAX; i++)
                {
                    dt_win_t *wn = win_get(i);
                    if (wn) rt_damage_mark(wn->x, wn->y, wn->w, wn->h);
                }
            }
            else
            {
                int x0, y0, x1, y1;
                rt_damage_get(&x0, &y0, &x1, &y1);
                render_all_in_rect(
                    x0, y0,
                    x1 - x0,
                    y1 - y0,
                    is.cx,
                    is.cy
                );
            }

            taskbar_draw(is.cx, is.cy, 0);
            rt_damage_mark(0, taskbar_y(), rt_width(), TB_H);
            cur_bake(is.cx, is.cy);
            rt_damage_mark(cur_last_x(), cur_last_y(), cur_w(), cur_h());

            {
                int fx, fy, fx1, fy1;
                rt_damage_get(&fx, &fy, &fx1, &fy1);
                int fw = fx1 - fx;
                int fh = fy1 - fy;
                if (fx < 0) { fw += fx; fx = 0; }
                if (fy < 0) { fh += fy; fy = 0; }
                if (fx + fw > rt_width())  fw = rt_width()  - fx;
                if (fy + fh > rt_height()) fh = rt_height() - fy;
                if (fw > 0 && fh > 0) rt_flush_rect(fx, fy, fw, fh);
            }
        }
        else if (need_cur)
        {
            cur_erase_fb();
            cur_draw_fb(is.cx, is.cy);
        }

        if (!need_full && !need_cur)
        {
            yield();
        }
    }
}
