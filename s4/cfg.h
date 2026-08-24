#pragma once
#include "os.h"

//#define ICONS SYSTEM "resources/icons/"
#define ICONS SYSTEM "icons/"

// scaling settings
#define RENDERER_SCALING_ENABLED 0
#define RENDERER_SUPERSAMPLING_FACTOR 2
#define RENDERER_ENABLE_BACKGROUND_BS 0
#define DARK_MODE 1
#define DT_ENABLE_RESIZING 1
#define ENABLE_TILING 0

#define DT_BG 0xFF008080u  // win95/98 teal ig its a fallback

#define WINDOW_TITLE_FONT "/system/fonts/terminus/ter-powerline-v12n.psf"

// rubber band selection colors
#define BAND_BORDER 0xCCA8A5A5u   // border
#define BAND_FILL   0x33A8A5A5u   // the content

//taskbar
#define TB_ENTRY_NAMELEN 32
#define TB_ENTRY_EXECLEN 64
#define TB_MAX_ENTRIES 10

#if RENDERER_SCALING_ENABLED
    // taskbar
    #define TB_H 60
    #define TB_BTN_W 40
    #define TB_BTN_H 40
    #define TB_BTN_PAD 5
    #define TB_BTN_VPAD 8
    #define TB_BORDER_W 5
    #define TB_START_W 34
    #define TB_START_H 34
    #define TB_STARTBUTTON_PAD_LEFT 8
    #define TB_STARTBUTTON_PAD_RIGHT 10
    #define TB_TOP_BORDER_HEIGHT 2

    #define BUTTON_TOP_BORDER_HEIGHT 4

    #define MAX_APPICON_SIZE 34

    // minimum window size
    #define WIN_MIN_W 120
    #define WIN_MIN_H 60

    // titlebar config
    #define DT_TITLE_H  26 // tot. inc. high
    #define DT_TITLE_PB 6  // pad. top
    #define DT_BORDER   2  // border width

    #undef RENDERER_ENABLE_BACKGROUND_BS
    #define RENDERER_ENABLE_BACKGROUND_BS 1
#else
    // taskbar
    #define TB_H 40
    #define TB_BTN_W 26
    #define TB_BTN_H 26
    #define TB_BTN_PAD 3
    #define TB_BTN_VPAD 6
    #define TB_BORDER_W 3
    #define TB_START_W 22
    #define TB_START_H 22
    #define TB_STARTBUTTON_PAD_LEFT 5
    #define TB_STARTBUTTON_PAD_RIGHT 7
    #define TB_TOP_BORDER_HEIGHT 1

    #define BUTTON_TOP_BORDER_HEIGHT 2

    #define MAX_APPICON_SIZE 21

    // minimum window size
    #define WIN_MIN_W 80
    #define WIN_MIN_H 40

    // titlebar config
    #define DT_TITLE_H  21 // tot. inc. high
    #define DT_TITLE_PB 5  // pad. top
    #define DT_BORDER   1  // border width
#endif

#define POWEROFF_LAUNCHPAD_PATH "/bin/poweroff.elf"
#define REBOOT_LAUNCHPAD_PATH "/bin/reboot.elf"
#define DEFAULT_ICON SYSTEM "icons/exec.bmp"
#define STARTBUTTON_ICON SYSTEM "icons/start.bmp"
#define SMENU_PATH SYSTEM "smenu.elf"

#if DARK_MODE == 1
    #define TB_BACKGROUND  0xFF202020u
    #define TB_TOP_BORDER  0xFF707070u
    #define TB_BUTTON_BG   0xFF404040u
    #define TB_BLACK       0xFF000000u
    #define TB_WHITE       0xFFFFFFFFu
    #define TB_LIGHT       0xFF808080u
    #define TB_SHADOW      0xFF606060u
    #define TB_BTN_TOP     0xFF6A89A7u
    #define TB_FACE        0xFFD4D0C8u
    #define TB_TRANSPARENT 0x00FFFFFFu

    #define WIN_BLACK           0xFF000000u
    #define WIN_WHITE           0xFF404040u
    #define WIN_FACE            0xFFD4D0C8u
    #define WIN_BUTTON          0xFFD4D0C8u
    #define WIN_FOCUSED_BG      0xFF5F5F5Fu
    #define WIN_UNFOCUSED_BG    0xFF5F5F5Fu
#elif DARK_MODE == 0
    #define TB_BACKGROUND  0xFFDFDFDFu
    #define TB_TOP_BORDER  0xFF8F8F8Fu
    #define TB_BUTTON_BG   0xFFBFBFBFu
    #define TB_BLACK       0xFFFFFFFFu
    #define TB_WHITE       0xFF000000u
    #define TB_LIGHT       0xFF7F7F7Fu
    #define TB_SHADOW      0xFF9F9F9Fu
    #define TB_BTN_TOP     0xFF957658u
    #define TB_FACE        0xFF2B2F37u
    #define TB_TRANSPARENT 0x00000000u

    #define WIN_BLACK           0xFF000000u
    #define WIN_WHITE           0xFFFFFFFFu
    #define WIN_FACE            0xFFD4D0C8u
    #define WIN_BUTTON          0xFFD4D0C8u
    #define WIN_FOCUSED_BG      0xFFA0A0A0u
    #define WIN_UNFOCUSED_BG    0xFF000000u
#endif

#define SM_W          180
#define SM_H          80
#define SM_MARGIN     2

#define SM_BTN_H      (TB_H - TB_BTN_VPAD * 2)
#define SM_BTN_W      ((SM_W - TB_BTN_PAD * 3) / 2)
#define SM_BTN_PAD    TB_BTN_PAD

#define SM_BTN_ROW_Y  (SM_H - SM_BTN_H - SM_BTN_PAD)

// window title bar
#define DT_TITLE_ACT 0xFFB0D0C0u  // green; when focused
#define DT_TITLE_INA 0xFF808080u  // grey ; when unfocused
//#define DT_TITLE_INA 0xFF4F2F3Fu //red
#define DT_TITLE_TXT 0xFFFFFFFFu  // text of titlebar

#define DT_CLOSE_X  5  // x offset
#define DT_CLOSE_Y  5  // y offset
#define DT_CLOSE_SZ 13 // square size

#define DT_MAX_X (DT_CLOSE_X + DT_CLOSE_SZ + 4)
#define DT_MAX_Y DT_CLOSE_Y
#define DT_MAX_SZ DT_CLOSE_SZ

// not minimize this button will put the window into taskbar, like fully fully minimize
#if ENABLE_TILING
    #define DT_MIN_X (DT_CLOSE_X + DT_CLOSE_SZ + 4)
#else
    #define DT_MIN_X (DT_MAX_X + DT_MAX_SZ + 4)
#endif

#define DT_MIN_Y DT_CLOSE_Y
#define DT_MIN_SZ DT_CLOSE_SZ

// font rendering
#define DT_FW 8
#define DT_FH 12
