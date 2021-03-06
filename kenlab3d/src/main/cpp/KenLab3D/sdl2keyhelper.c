/************************************************************************************
 * * The MIT License (MIT)
 **
 ** Copyright (c) 2015-2016 Serg "EXL" Koles
 **
 ** Permission is hereby granted, free of charge, to any person obtaining a copy
 ** of this software and associated documentation files (the "Software"), to deal
 ** in the Software without restriction, including without limitation the rights
 ** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 ** copies of the Software, and to permit persons to whom the Software is
 ** furnished to do so, subject to the following conditions:
 **
 ** The above copyright notice and this permission notice shall be included in all
 ** copies or substantial portions of the Software.
 **
 ** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 ** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 ** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 ** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 ** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 ** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 ** SOFTWARE.
 ************************************************************************************/

#include "sdl2keyhelper.h"

#ifndef ANDROID_NDK
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif // !ANDROID_NDK

int patchAndroidKeysDpadSDL2(int keyCode) {
    switch (keyCode) {
    case OLDK_w:
        return OLDK_UP;
    case OLDK_s:
        return OLDK_DOWN;
    case OLDK_a:
        return OLDK_LEFT;
    case OLDK_d:
        return OLDK_RIGHT;
    case SDLK_SELECT:
        return OLDK_RETURN;
    case OLDK_r:
    case SDLK_AC_BACK:
        return OLDK_ESCAPE;
    default:
        return keyCode;
    }
}

int getUpperChar(int smallCharKeyCode) {
    return smallCharKeyCode - 0x20;
}

int getOldAsciiKeyCode(int newKeyCode) {
    switch (newKeyCode) {
    default:
        return newKeyCode;
    case SDLK_UNKNOWN:
        return OLDK_UNKNOWN;
    case SDLK_BACKSPACE:
        return OLDK_BACKSPACE;
    case SDLK_TAB:
        return OLDK_TAB;
    case SDLK_CLEAR:
        return OLDK_CLEAR;
    case SDLK_RETURN:
        return OLDK_RETURN;
    case SDLK_PAUSE:
        return OLDK_PAUSE;
    case SDLK_ESCAPE:
        return OLDK_ESCAPE;
    case SDLK_SPACE:
        return OLDK_SPACE;
    case SDLK_EXCLAIM:
        return OLDK_EXCLAIM;
    case SDLK_QUOTEDBL:
        return OLDK_QUOTEDBL;
    case SDLK_HASH:
        return OLDK_HASH;
    case SDLK_DOLLAR:
        return OLDK_DOLLAR;
    case SDLK_AMPERSAND:
        return OLDK_AMPERSAND;
    case SDLK_QUOTE:
        return OLDK_QUOTE;
    case SDLK_LEFTPAREN:
        return OLDK_LEFTPAREN;
    case SDLK_RIGHTPAREN:
        return OLDK_RIGHTPAREN;
    case SDLK_ASTERISK:
        return OLDK_ASTERISK;
    case SDLK_PLUS:
        return OLDK_PLUS;
    case SDLK_COMMA:
        return OLDK_COMMA;
    case SDLK_MINUS:
        return OLDK_MINUS;
    case SDLK_PERIOD:
        return OLDK_PERIOD;
    case SDLK_SLASH:
        return OLDK_SLASH;
    case SDLK_0:
        return OLDK_0;
    case SDLK_1:
        return OLDK_1;
    case SDLK_2:
        return OLDK_2;
    case SDLK_3:
        return OLDK_3;
    case SDLK_4:
        return OLDK_4;
    case SDLK_5:
        return OLDK_5;
    case SDLK_6:
        return OLDK_6;
    case SDLK_7:
        return OLDK_7;
    case SDLK_8:
        return OLDK_8;
    case SDLK_9:
        return OLDK_9;
    case SDLK_COLON:
        return OLDK_COLON;
    case SDLK_SEMICOLON:
        return OLDK_SEMICOLON;
    case SDLK_LESS:
        return OLDK_LESS;
    case SDLK_EQUALS:
        return OLDK_EQUALS;
    case SDLK_GREATER:
        return OLDK_GREATER;
    case SDLK_QUESTION:
        return OLDK_QUESTION;
    case SDLK_AT:
        return OLDK_AT;
    case SDLK_LEFTBRACKET:
        return OLDK_LEFTBRACKET;
    case SDLK_BACKSLASH:
        return OLDK_BACKSLASH;
    case SDLK_RIGHTBRACKET:
        return OLDK_RIGHTBRACKET;
    case SDLK_CARET:
        return OLDK_CARET;
    case SDLK_UNDERSCORE:
        return OLDK_UNDERSCORE;
    case SDLK_BACKQUOTE:
        return OLDK_BACKQUOTE;
    case SDLK_a:
        return OLDK_a;
    case SDLK_b:
        return OLDK_b;
    case SDLK_c:
        return OLDK_c;
    case SDLK_d:
        return OLDK_d;
    case SDLK_e:
        return OLDK_e;
    case SDLK_f:
        return OLDK_f;
    case SDLK_g:
        return OLDK_g;
    case SDLK_h:
        return OLDK_h;
    case SDLK_i:
        return OLDK_i;
    case SDLK_j:
        return OLDK_j;
    case SDLK_k:
        return OLDK_k;
    case SDLK_l:
        return OLDK_l;
    case SDLK_m:
        return OLDK_m;
    case SDLK_n:
        return OLDK_n;
    case SDLK_o:
        return OLDK_o;
    case SDLK_p:
        return OLDK_p;
    case SDLK_q:
        return OLDK_q;
    case SDLK_r:
        return OLDK_r;
    case SDLK_s:
        return OLDK_s;
    case SDLK_t:
        return OLDK_t;
    case SDLK_u:
        return OLDK_u;
    case SDLK_v:
        return OLDK_v;
    case SDLK_w:
        return OLDK_w;
    case SDLK_x:
        return OLDK_x;
    case SDLK_y:
        return OLDK_y;
    case SDLK_z:
        return OLDK_z;
    case SDLK_DELETE:
        return OLDK_DELETE;
    case SDLK_KP_0:
        return OLDK_KP0;
    case SDLK_KP_1:
        return OLDK_KP1;
    case SDLK_KP_2:
        return OLDK_KP2;
    case SDLK_KP_3:
        return OLDK_KP3;
    case SDLK_KP_4:
        return OLDK_KP4;
    case SDLK_KP_5:
        return OLDK_KP5;
    case SDLK_KP_6:
        return OLDK_KP6;
    case SDLK_KP_7:
        return OLDK_KP7;
    case SDLK_KP_8:
        return OLDK_KP8;
    case SDLK_KP_9:
        return OLDK_KP9;
    case SDLK_KP_PERIOD:
        return OLDK_KP_PERIOD;
    case SDLK_KP_DIVIDE:
        return OLDK_KP_DIVIDE;
    case SDLK_KP_MULTIPLY:
        return OLDK_KP_MULTIPLY;
    case SDLK_KP_MINUS:
        return OLDK_KP_MINUS;
    case SDLK_KP_PLUS:
        return OLDK_KP_PLUS;
    case SDLK_KP_ENTER:
        return OLDK_KP_ENTER;
    case SDLK_KP_EQUALS:
        return OLDK_KP_EQUALS;
    case SDLK_UP:
        return OLDK_UP;
    case SDLK_DOWN:
        return OLDK_DOWN;
    case SDLK_RIGHT:
        return OLDK_RIGHT;
    case SDLK_LEFT:
        return OLDK_LEFT;
    case SDLK_INSERT:
        return OLDK_INSERT;
    case SDLK_HOME:
        return OLDK_HOME;
    case SDLK_END:
        return OLDK_END;
    case SDLK_PAGEUP:
        return OLDK_PAGEUP;
    case SDLK_PAGEDOWN:
        return OLDK_PAGEDOWN;
    case SDLK_F1:
        return OLDK_F1;
    case SDLK_F2:
        return OLDK_F2;
    case SDLK_F3:
        return OLDK_F3;
    case SDLK_F4:
        return OLDK_F4;
    case SDLK_F5:
        return OLDK_F5;
    case SDLK_F6:
        return OLDK_F6;
    case SDLK_F7:
        return OLDK_F7;
    case SDLK_F8:
        return OLDK_F8;
    case SDLK_F9:
        return OLDK_F9;
    case SDLK_F10:
        return OLDK_F10;
    case SDLK_F11:
        return OLDK_F11;
    case SDLK_F12:
        return OLDK_F12;
    case SDLK_F13:
        return OLDK_F13;
    case SDLK_F14:
        return OLDK_F14;
    case SDLK_F15:
        return OLDK_F15;
    case SDLK_CAPSLOCK:
        return OLDK_CAPSLOCK;
    case SDLK_RSHIFT:
        return OLDK_RSHIFT;
    case SDLK_LSHIFT:
        return OLDK_LSHIFT;
    case SDLK_RCTRL:
        return OLDK_RCTRL;
    case SDLK_LCTRL:
        return OLDK_LCTRL;
    case SDLK_RALT:
        return OLDK_RALT;
    case SDLK_LALT:
        return OLDK_LALT;
    case SDLK_MODE:
        return OLDK_MODE;
    case SDLK_HELP:
        return OLDK_HELP;
    case SDLK_SYSREQ:
        return OLDK_SYSREQ;
    case SDLK_MENU:
        return OLDK_MENU;
    case SDLK_POWER:
        return OLDK_POWER;
    case SDLK_UNDO:
        return OLDK_UNDO;
    }
}
