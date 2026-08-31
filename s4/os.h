/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2026 sulfurLabs
 *
 * PROJECT: s4
 * FILE: os.h
 *
 */

#pragma once

#define DT_DIR    "/tmp/dt"
#define DT_CMD    "/tmp/dt/cmd"
#define DT_DIRTY  "/tmp/dt/dirty"
#define DT_CURSOR "/tmp/dt/cursor"

#ifdef __sulfur__
    #include <sys/sipc.h>
    #define GREETING "sulfurOS"
    #define SHM_DEV      "/dev/shm0"
    #define MOUSE_DEV    "/dev/mouse"
    #define KEYBOARD_DEV "/dev/kbd0"
    #define FRAMEBUFFER_DEV "/dev/fb0"

    //todo find current username
    #define APPPATH "/users/pc/applications/"
    #define SYSTEM "/users/pc/applications/desktop/" //the path to the desktop in the system
    #define CFGFILE   SYSTEM "desktop.ui16"
    #define ENTRYFILE SYSTEM "entries.json"
#elif __nullos__
    #define GREETING "NullOS"
    /* still unsupported because of missing devices
     * it will quit after opening framebuffer
     */
    #define SHM_DEV      "none"
    #define MOUSE_DEV    "none"
    #define KEYBOARD_DEV "none"  // fetch from stdin
    //TODO:
    // input layer under pollEvents
    #define FRAMEBUFFER_DEV "/dev/fb0"

    #define SYSTEM "/home/s4/" //the path to the desktop in the system
    #define CFGFILE   SYSTEM "desktop.ui16"
    #define ENTRYFILE SYSTEM "entries.json"
#elif __avory__
    #define GREETING "AvoryOS"
    #define SHM_DEV      "/dev/shm0"
    #define MOUSE_DEV    "/dev/mouse"
    #define KEYBOARD_DEV "/dev/kbd"
    #define FRAMEBUFFER_DEV "/dev/fb0"

    #define SYSTEM "/system/desktop/"
    #define CFGFILE   SYSTEM "desktop.ui16"
    #define ENTRYFILE SYSTEM "entries.json"
#elif __linux__
    #define GREETING "Linux"
    #define SHM_DEV      "/dev/shm"
    #define MOUSE_DEV    "/dev/mouse"
    #define KEYBOARD_DEV "/dev/input/" // idk i need a proper layer for that
    #define FRAMEBUFFER_DEV "/dev/fb0"

    #define SYSTEM "/system/desktop/"
    #define CFGFILE   SYSTEM "desktop.ui16"
    #define ENTRYFILE SYSTEM "entries.json"
#elif __has_include
    // idfk if this works... rn i cant test it cuz doccrOS isnt finished
    // but i think it works in newer gcc vrsions __has_include is supported ig.... :/
    #if __has_include(<emx/sinfo.h>)
        #include <emx/sinfo.h>
        #if defined(__EMEX__)
            #define GREETING "emexOS"
            #define MOUSE_DEV    "/dev/input/mouse0"
            #define KEYBOARD_DEV "/dev/input/keyboard0"
            #define FRAMEBUFFER_DEV "/dev/fb0"

            #define SYSTEM "/emr/system/desktop/"
            #define CFGFILE   SYSTEM "desktop.exui"
            #define ENTRYFILE SYSTEM "entries.json"
        #endif
    #endif
#else
    #error "Unsupported operating system"
#endif