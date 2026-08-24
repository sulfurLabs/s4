/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2026 sulfurLabs
 *
 * PROJECT: s4
 * FILE: taskbar.c
 *
 */

#include "taskbar.h"
#include "startmenu.h"
#include "entries.h"
#include "dt_taskbar.h"
#include "../compositor/comp.h"
#include "../cfg.h"
#include "../fonts/fonts.h"
#include "../ipc/ipc.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
/*
typedef struct
{
    char 	name[TB_ENTRY_NAMELEN];
    char 	exec[TB_ENTRY_EXECLEN];
} tb_entry_t;*/

static tb_widget_t  s_widgets[TB_WIDGET_MAX];
//static int	s_entry_count = 0;
static int      s_widget_count = 0;
static int	s_scr_w = 0;
static int	s_scr_h = 0;
static int	s_tb_y  = 0;
static tb_widget_t s_start_widget;

static int _slen(const char *s)
{
	int n = 0;
	while (s[n]) n++;

	return n;
}

static int _atoi(const char *s)
{
    int v = 0;
    int neg = 0;
    while (*s == ' ') s++;

    if (*s == '-')
    {
    	neg = 1;
    	s++;
    }

    while (
    	*s >= '0' &&
     	*s <= '9'
    ) v = v * 10 + (*s++ - '0');

    return neg ? -v : v;
}

static const char *_next_tok(const char **p, char *out, int outsz)
{
	int i = 0;

    while (**p == ' ') (*p)++;
    while (
    	**p &&
     	**p != ' ' &&
      	**p != '\n' &&
       	i < outsz - 1
    ) out[i++] = *(*p)++;

    out[i] = '\0';

    return out;
}



static int start_btn_x(void)
{
    return TB_BTN_PAD + TB_STARTBUTTON_PAD_LEFT;
}

static int btn_x(int i)
{
    return start_btn_x()
         + TB_START_W
         + TB_STARTBUTTON_PAD_RIGHT
         + TB_BTN_PAD
         + i * (TB_BTN_W + TB_BTN_PAD);
}

static int hit_start_btn(int mx, int my)
{
    int bx = start_btn_x();
    int by = s_tb_y + TB_BTN_VPAD;
    int bh = TB_H - TB_BTN_VPAD * 2;
    return
    	mx >= bx && mx < bx + TB_START_W &&
        my >= by && my < by + bh
    ;
}

static int hit_btn(int i, int mx, int my)
{
    int bx = btn_x(i);
    int by = s_tb_y + TB_BTN_VPAD;

    return
    	mx >= bx && mx < bx + TB_BTN_W &&
        my >= by && my < by + (TB_H - TB_BTN_VPAD * 2)
    ;
}

static int file_exists(const char *path)
{
    long fd = open(path, O_RDONLY);
    if (fd < 0) return 0;

    close((int)fd);

    return 1;
}

static void popup_pos(int i, int pw, int ph, int *out_x, int *out_y)
{
    int bx = btn_x(i);
    *out_x = bx;
    *out_y = s_tb_y - ph - TB_BTN_VPAD;

    if (*out_x + pw > s_scr_w) *out_x = s_scr_w - pw;
    if (*out_x < 0) *out_x = 0;
    if (*out_y < 0) *out_y = 0;
}

static void open_popup_window(int widget_idx, pid_t pid, int pw, int ph)
{
    int px;
    int py;
    popup_pos(widget_idx, pw, ph, &px, &py);

    ipc_request_open_window(pid, DT_POPUP, px, py, pw, ph, "");
}

static void draw_widget_content(
    tb_widget_t *wg,
    int bx, int by, int bw, int bh,
    int press,
    unsigned int text_bg
) {
    const char *label = (wg->type == TB_WIDGET_APP || wg->type == TB_WIDGET_START)
        ? wg->name
        : wg->text
    ;

    int show_icon = (wg->disp != TB_DISP_TEXT_ONLY) && wg->icon.loaded;
    int show_text = (wg->disp != TB_DISP_ICON_ONLY);
    int fw = font_w(FONT8X12_BOLD);
    int fh = font_h(FONT8X12_BOLD);
    int nlen = _slen(label);
    int tw = nlen * fw;

    const int icon_slot = MAX_APPICON_SIZE;

    if (show_icon)
    {
        const bmp_image_t *img = &wg->icon.image;

        float sx = (float)icon_slot / (float)img->width;
        float sy = (float)icon_slot / (float)img->height;
        float s  = (sx < sy) ? sx : sy;

        int draw_w = (int)(img->width * s);
        int draw_h = (int)(img->height * s);

        if (draw_w < 1) draw_w = 1;
        if (draw_h < 1) draw_h = 1;

        // if theres also text the icon is left, otherwise its centered
        int slot_x = show_text ? (bx + 4) : (bx + (bw - icon_slot) / 2);
        int slot_y = by + (bh - icon_slot) / 2;

        int ix = slot_x + (icon_slot - draw_w) / 2;
        int iy = slot_y + (icon_slot - draw_h) / 2;

        bmp_draw_scaled(&g_comp_target, img, ix, iy, draw_w, draw_h);
    }

    if (show_text)
    {
        int ty = by + (bh - fh) / 2 + (press ? 1 : 0);
        int tx;

        if (show_icon)
        {
            tx = bx + 4 + icon_slot + 4 + (press ? 1 : 0);
        }
        else
        {
            tx = bx + (bw - tw) / 2 + (press ? 1 : 0);
        }

        for (int ci = 0; ci < nlen; ci++)
        {
            unsigned char c = (unsigned char)label[ci] & 0x7Fu;

            for (int row = 0; row < fh; row++)
            {
                uint16_t bits = font_glyph(FONT8X12_BOLD, c, row);
                for (int col = 0; col < fw; col++)
                {
                    unsigned int color = (bits & (1u << col)) ? TB_WHITE : text_bg;

                    comp_set(tx + ci * fw + col, ty + row, color);
                }
            }
        }
    }
}

void taskbar_init(int scr_w, int scr_h)
{
	s_scr_w 	= scr_w;
    s_scr_h 	= scr_h;
    s_tb_y  	= scr_h - TB_H;

    //setup_entries();

    int entry_count = 0;
    tb_widget_t *entries = entries_get(&entry_count);

    for (
    	int i = 0;
     	i < entry_count && s_widget_count < TB_WIDGET_MAX;
      	i++
    ) s_widgets[s_widget_count++] = entries[i];

    {
        memset(&s_start_widget, 0, sizeof(s_start_widget));
        s_start_widget.type        = TB_WIDGET_START;
        s_start_widget.disp        = TB_DISP_ICON_ONLY;
        s_start_widget.icon_path   = SYSTEM "icons/start.tga";
        s_start_widget.icon.loaded = 0;
        s_start_widget.popup_pid   = -1;

        strncpy(s_start_widget.name, "start", TB_WIDGET_NAMELEN - 1);
        s_start_widget.name[TB_WIDGET_NAMELEN - 1] = '\0';
        strncpy(s_start_widget.text, "start", TB_WIDGET_TEXTLEN - 1);
        s_start_widget.text[TB_WIDGET_TEXTLEN - 1] = '\0';

        entries_load_icon(&s_start_widget);
    }

    startmenu_init(scr_w, scr_h);

    //TODO:
    // clock widget
}

int taskbar_y(void)
{
    return s_tb_y;
}

int taskbar_add_widget(const tb_widget_t *w)
{
    if (!w || s_widget_count >= TB_WIDGET_MAX) return -1;
    s_widgets[s_widget_count] = *w;
    return s_widget_count++;
}

static void draw_start_button(int y, int mx, int my, int btn_down)
{
    int bx    = start_btn_x();
    int by    = y + TB_BTN_VPAD + 3;
    int bh    = TB_START_H;
    int hov   = hit_start_btn(mx, my);
    /* also show as pressed when menu is open */
    int press = (hov && btn_down) || startmenu_is_open();

    //comp_fill(bx, by, TB_START_W, bh, TB_BUTTON_BG);

    if (hov || press)
    {
        //unsigned int col = press ? TB_BTN_TOP : TB_LIGHT;
        /*
        comp_fill(bx, by,                            TB_START_W,  TB_BORDER_W, col);
        comp_fill(bx, by + bh - TB_BORDER_W,         TB_START_W,  TB_BORDER_W, col);
        comp_fill(bx, by,                            TB_BORDER_W, bh,          col);
        comp_fill(bx + TB_START_W - TB_BORDER_W, by, TB_BORDER_W, bh,          col);*/

        comp_fill(bx, by, TB_START_W, bh, TB_BUTTON_BG);
        //comp_fill(bx, by, TB_START_W, 2,  TB_TOP_BORDER);
    }

    draw_widget_content(&s_start_widget, bx, by, TB_START_W, bh, press, TB_BACKGROUND);
}

void taskbar_draw(int mx, int my, int btn_down)
{
    int y  = s_tb_y;
    int w  = s_scr_w;

    // bar bg
    for (int dy = 0; dy < TB_H; dy++) comp_fill(0, y + dy, w, 1, TB_BACKGROUND);

    // top border line
    comp_fill(0, y, w, TB_TOP_BORDER_HEIGHT, TB_TOP_BORDER);

    draw_start_button(y, mx, my, btn_down);

    divider: {
        int divider_x = start_btn_x() + (TB_START_W + TB_STARTBUTTON_PAD_RIGHT);
        int divider_y = y + 4;
        int divider_h = TB_H - 8;

        comp_fill(divider_x, divider_y, 1, divider_h, TB_TOP_BORDER);
        #if RENDER_SCALING_ENABLED == 1
            comp_fill(divider_x + 1, divider_y, 1, divider_h, TB_LIGHT);
        #endif
    }

    for (int i = 0; i < s_widget_count; i++)
    {
        tb_widget_t *wg = &s_widgets[i];

        int bx  = btn_x(i);
        int by   = y + TB_BTN_VPAD;
        int bh  = TB_H - (TB_BTN_VPAD * 2);
        int hov = hit_btn(i, mx, my);
        int press = hov && btn_down;

        // label / updated label
        //const char *label = (wg->type == TB_WIDGET_APP) ? wg->name : wg->text;

        // button face
        //comp_fill(bx, by, TB_BTN_W, bh, TB_BUTTON_BG);

        // win95 style raised/pressed borders
       	/*{
		    if (!press)
		    {
				comp_fill(bx, by, TB_BTN_W, 1, TB_LIGHT);
        		comp_fill(bx, by + bh - 1, TB_BTN_W, 1, TB_LIGHT);
         		comp_fill(bx, by, 1, bh, TB_LIGHT);
          		comp_fill(bx + TB_BTN_W - 1, by, 1, bh, TB_LIGHT);
		    } else
		    {
				comp_fill(bx, by, TB_BTN_W, 1, TB_LIGHT);
        		comp_fill(bx, by + bh - 1, TB_BTN_W, 1, TB_LIGHT);
          		comp_fill(bx, by, 1, bh, TB_LIGHT);
           		comp_fill(bx + TB_BTN_W - 1, by, 1, bh, TB_LIGHT);
		    }
     	}*/

	    {
	        int show_border = hov || press;

	        if (show_border)
	        {
	            unsigned int col = press ? TB_BTN_TOP : TB_LIGHT;

	            /*comp_fill(bx, by,                          TB_BTN_W,    TB_BORDER_W, col);
	            comp_fill(bx, by + bh - TB_BORDER_W,       TB_BTN_W,    TB_BORDER_W, col);
	            comp_fill(bx, by,                          TB_BORDER_W, bh,          col);
	            comp_fill(bx + TB_BTN_W - TB_BORDER_W, by, TB_BORDER_W, bh,          col);*/

				comp_fill(bx, by, TB_BTN_W, bh, TB_BUTTON_BG);

				if (hov || press)
				{
				    comp_fill(bx, by, TB_BTN_W, 2, TB_TOP_BORDER);
				}
	        }
	    }

        unsigned int text_bg = (hov || press) ? TB_BUTTON_BG : TB_BACKGROUND;
        draw_widget_content(wg, bx, by, TB_BTN_W, bh, press, text_bg);
    }

    startmenu_draw(mx, my, btn_down);
}

int taskbar_click(int mx, int my)
{
    if (startmenu_is_open())
    {
        int consumed = startmenu_click(mx, my);
        if (consumed) return 1;

        startmenu_close();
    }

    if (my < s_tb_y) return 0;

    if (hit_start_btn(mx, my))
    {
        startmenu_toggle(
        	start_btn_x(),
         	s_tb_y
        );
        return 1;
    }

    for (int i = 0; i < s_widget_count; i++)
    {
        if (!hit_btn(i, mx, my)) continue;

        tb_widget_t *wg = &s_widgets[i];

        if (wg->type == TB_WIDGET_APP)
        {
        	if (!file_exists(wg->exec)) return 1;
            pid_t pid = (pid_t)spawn(wg->exec);

            printf("[TASKBAR] spawn returned pid= %d \n", (int)pid);

            if (pid < 0)
            {
                printf("[TASKBAR] fork FAILED \n");
                return 1;
            }

            return 1;
        }

        if (wg->type == TB_WIDGET_LABEL || wg->type == TB_WIDGET_UPDATED_LABEL)
        {
            // if the app popup id is already opened then it does nothing so it doesnt show 2 times
            if (wg->popup_pid > 0) return 1;

            if (wg->exec[0] != '\0' && file_exists(wg->exec))
            {
                pid_t pid = (pid_t)spawn(wg->exec);
                if (pid < 0) return 1;
                //when app sends R cmd to /tmp/dt/tbcmd
                wg->popup_pid = pid;

                //L <name> <text> == update label text
                // R <name> <pid> <w> <h> == registers and opens popup window
                // U <name> == unregister like closes popup
            }
            return 1;
        }
    }
    return 0;
}

static int find_widget_by_name(const char *name)
{
    for (int i = 0; i < s_widget_count; i++)
    {
	    if (
			strncmp(
				s_widgets[i].name, name, TB_WIDGET_NAMELEN
			) == 0
		) return i;
    }

    return -1;
}

static void _tbcmd_clear(int n)
{

    int fd = open(DT_TB_CMD, O_WRONLY | O_CREAT);
    char clr[256];
    int chunk = n < 256 ? n : 256;

    if (fd < 0) return;
    if (n <= 0) return;

    for (int i = 0; i < chunk; i++) clr[i] = '\n';

    write(fd, clr, (unsigned)chunk);
    close(fd);
}

static void _process_tbcmd_line(const char *line)
{
    if (!line[0] || line[0] == '\n') return;

    char cmd = line[0];
    const char *p = line + 1;

    char tok1[TB_WIDGET_NAMELEN];
    char tok2[TB_WIDGET_TEXTLEN];
    char tok3[16];
    char tok4[16];

    if (cmd == 'L')
    {
        // L <name> <text>
        _next_tok(&p, tok1, sizeof(tok1));      // name
        _next_tok(&p, tok2, sizeof(tok2));      // text

        int idx = find_widget_by_name(tok1);
        if (idx < 0) return;

        tb_widget_t *wg = &s_widgets[idx];

        if (wg->type != TB_WIDGET_LABEL && wg->type != TB_WIDGET_UPDATED_LABEL) return;

        strncpy(wg->text, tok2, TB_WIDGET_TEXTLEN - 1);
        wg->text[TB_WIDGET_TEXTLEN - 1] = '\0';
        wg->dirty = 1;
    }
    else if (cmd == 'R')
    {
        // R <name> <pid> <w> <h>
        _next_tok(&p, tok1, sizeof(tok1));           // name
        _next_tok(&p, tok2, sizeof(tok2));           // pid
        _next_tok(&p, tok3, sizeof(tok3));           // w
        _next_tok(&p, tok4, sizeof(tok4));          // h

        int idx = find_widget_by_name(tok1);
        if (idx < 0) return;

        tb_widget_t *wg = &s_widgets[idx];
        pid_t  pid = (pid_t)_atoi(tok2);
        int    pw  = _atoi(tok3);
        int    ph  = _atoi(tok4);

        wg->popup_pid = pid;
        wg->popup_w   = pw;
        wg->popup_h   = ph;

        //
        open_popup_window(idx, pid, pw, ph);
    }
    else if (cmd == 'U')
    {
        // U <name>
        _next_tok(&p, tok1, sizeof(tok1));
        int idx = find_widget_by_name(tok1);
        if (idx < 0) return;

        s_widgets[idx].popup_pid    = -1;
        s_widgets[idx].popup_w      = 0;
        s_widgets[idx].popup_h       = 0;
    }
}

void taskbar_cmd_process(void)
{
    static char buf[4096];

    int fd = open(DT_TB_CMD, O_RDONLY);
    if (fd < 0) return;

    int n = (int)read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) return;

    buf[n] = '\0';
    if (buf[0] == '\0') return;

    _tbcmd_clear(n);

    const char *line = buf;
    while (*line)
    {
        _process_tbcmd_line(line);
        while (*line && *line != '\n') line++;
        if (*line == '\n') line++;
    }
}
