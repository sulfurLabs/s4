/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Copyright (c) 2026 sulfurLabs
 *
 * PROJECT: s4
 * FILE: keymap.c
 *
 */

#include "ui16.h"
#include <sys/input.h>

char ui16_keyToChar(unsigned int keycode, int shift)
{
    switch (keycode)
    {
        case INPUT_KEY_A: return shift ? 'A' : 'a';
        case INPUT_KEY_B: return shift ? 'B' : 'b';
        case INPUT_KEY_C: return shift ? 'C' : 'c';
        case INPUT_KEY_D: return shift ? 'D' : 'd';
        case INPUT_KEY_E: return shift ? 'E' : 'e';
        case INPUT_KEY_F: return shift ? 'F' : 'f';
        case INPUT_KEY_G: return shift ? 'G' : 'g';
        case INPUT_KEY_H: return shift ? 'H' : 'h';
        case INPUT_KEY_I: return shift ? 'I' : 'i';
        case INPUT_KEY_J: return shift ? 'J' : 'j';
        case INPUT_KEY_K: return shift ? 'K' : 'k';
        case INPUT_KEY_L: return shift ? 'L' : 'l';
        case INPUT_KEY_M: return shift ? 'M' : 'm';
        case INPUT_KEY_N: return shift ? 'N' : 'n';
        case INPUT_KEY_O: return shift ? 'O' : 'o';
        case INPUT_KEY_P: return shift ? 'P' : 'p';
        case INPUT_KEY_Q: return shift ? 'Q' : 'q';
        case INPUT_KEY_R: return shift ? 'R' : 'r';
        case INPUT_KEY_S: return shift ? 'S' : 's';
        case INPUT_KEY_T: return shift ? 'T' : 't';
        case INPUT_KEY_U: return shift ? 'U' : 'u';
        case INPUT_KEY_V: return shift ? 'V' : 'v';
        case INPUT_KEY_W: return shift ? 'W' : 'w';
        case INPUT_KEY_X: return shift ? 'X' : 'x';
        case INPUT_KEY_Y: return shift ? 'Y' : 'y';
        case INPUT_KEY_Z: return shift ? 'Z' : 'z';
        case INPUT_KEY_0: return shift ? ')' : '0';
        case INPUT_KEY_1: return shift ? '!' : '1';
        case INPUT_KEY_2: return shift ? '@' : '2';
        case INPUT_KEY_3: return shift ? '#' : '3';
        case INPUT_KEY_4: return shift ? '$' : '4';
        case INPUT_KEY_5: return shift ? '%' : '5';
        case INPUT_KEY_6: return shift ? '^' : '6';
        case INPUT_KEY_7: return shift ? '&' : '7';
        case INPUT_KEY_8: return shift ? '*' : '8';
        case INPUT_KEY_9: return shift ? '(' : '9';

        case INPUT_KEY_SPACE: return ' ';
        case INPUT_KEY_MINUS: return shift ? '_' : '-';

        case INPUT_KEY_DOT: return shift ? '>' : '.';
        case INPUT_KEY_COMMA: return shift ? '<' : ',';
        case INPUT_KEY_EQUAL: return shift ? '+' : '=';
        case INPUT_KEY_LBRACKET: return shift ? '{' : '[';
        case INPUT_KEY_RBRACKET: return shift ? '}' : ']';
        case INPUT_KEY_BACKSLASH: return shift ? '|' : '\\';
        case INPUT_KEY_SEMICOLON: return shift ? ':' : ';';
        case INPUT_KEY_APOSTROPHE: return shift ? '"' : '\'';
        case INPUT_KEY_GRAVE: return shift ? '~' : '`';
        case INPUT_KEY_SLASH: return shift ? '?' : '/';
        case INPUT_KEY_ENTER:
        case INPUT_KEY_KP_ENTER: return '\n';
        case INPUT_KEY_BACKSPACE: return '\b';
        case INPUT_KEY_TAB: return '\t';

        default: return 0;
    }
}