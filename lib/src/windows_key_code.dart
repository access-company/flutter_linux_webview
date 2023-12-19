// Copyright (c) 2023 ACCESS CO., LTD. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of ACCESS CO., LTD. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// This file is based on https://bitbucket.org/chromiumembedded/cef/src/4664/tests/cefclient/browser/browser_window_osr_gtk.cc

// Copyright (c) 2015 The Chromium Embedded Framework Authors. All rights
// reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the name Chromium Embedded
// Framework nor the names of its contributors may be used to endorse
// or promote products derived from this software without specific prior
// written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import 'package:flutter/services.dart';

// From ui/events/keycodes/keyboard_codes_posix.h.
enum KeyboardCode {
  VKEY_BACK(0x08),
  VKEY_TAB(0x09),
  VKEY_BACKTAB(0x0A),
  VKEY_CLEAR(0x0C),
  VKEY_RETURN(0x0D),
  VKEY_SHIFT(0x10),
  VKEY_CONTROL(0x11),
  VKEY_MENU(0x12),
  VKEY_PAUSE(0x13),
  VKEY_CAPITAL(0x14),
  VKEY_KANA(0x15),
  VKEY_HANGUL(0x15),
  VKEY_JUNJA(0x17),
  VKEY_FINAL(0x18),
  VKEY_HANJA(0x19),
  VKEY_KANJI(0x19),
  VKEY_ESCAPE(0x1B),
  VKEY_CONVERT(0x1C),
  VKEY_NONCONVERT(0x1D),
  VKEY_ACCEPT(0x1E),
  VKEY_MODECHANGE(0x1F),
  VKEY_SPACE(0x20),
  VKEY_PRIOR(0x21),
  VKEY_NEXT(0x22),
  VKEY_END(0x23),
  VKEY_HOME(0x24),
  VKEY_LEFT(0x25),
  VKEY_UP(0x26),
  VKEY_RIGHT(0x27),
  VKEY_DOWN(0x28),
  VKEY_SELECT(0x29),
  VKEY_PRINT(0x2A),
  VKEY_EXECUTE(0x2B),
  VKEY_SNAPSHOT(0x2C),
  VKEY_INSERT(0x2D),
  VKEY_DELETE(0x2E),
  VKEY_HELP(0x2F),
  VKEY_0(0x30),
  VKEY_1(0x31),
  VKEY_2(0x32),
  VKEY_3(0x33),
  VKEY_4(0x34),
  VKEY_5(0x35),
  VKEY_6(0x36),
  VKEY_7(0x37),
  VKEY_8(0x38),
  VKEY_9(0x39),
  VKEY_A(0x41),
  VKEY_B(0x42),
  VKEY_C(0x43),
  VKEY_D(0x44),
  VKEY_E(0x45),
  VKEY_F(0x46),
  VKEY_G(0x47),
  VKEY_H(0x48),
  VKEY_I(0x49),
  VKEY_J(0x4A),
  VKEY_K(0x4B),
  VKEY_L(0x4C),
  VKEY_M(0x4D),
  VKEY_N(0x4E),
  VKEY_O(0x4F),
  VKEY_P(0x50),
  VKEY_Q(0x51),
  VKEY_R(0x52),
  VKEY_S(0x53),
  VKEY_T(0x54),
  VKEY_U(0x55),
  VKEY_V(0x56),
  VKEY_W(0x57),
  VKEY_X(0x58),
  VKEY_Y(0x59),
  VKEY_Z(0x5A),
  VKEY_LWIN(0x5B),
  // VKEY_COMMAND = VKEY_LWIN
  // Provide the Mac name for convenience.
  VKEY_COMMAND(0x5B),
  VKEY_RWIN(0x5C),
  VKEY_APPS(0x5D),
  VKEY_SLEEP(0x5F),
  VKEY_NUMPAD0(0x60),
  VKEY_NUMPAD1(0x61),
  VKEY_NUMPAD2(0x62),
  VKEY_NUMPAD3(0x63),
  VKEY_NUMPAD4(0x64),
  VKEY_NUMPAD5(0x65),
  VKEY_NUMPAD6(0x66),
  VKEY_NUMPAD7(0x67),
  VKEY_NUMPAD8(0x68),
  VKEY_NUMPAD9(0x69),
  VKEY_MULTIPLY(0x6A),
  VKEY_ADD(0x6B),
  VKEY_SEPARATOR(0x6C),
  VKEY_SUBTRACT(0x6D),
  VKEY_DECIMAL(0x6E),
  VKEY_DIVIDE(0x6F),
  VKEY_F1(0x70),
  VKEY_F2(0x71),
  VKEY_F3(0x72),
  VKEY_F4(0x73),
  VKEY_F5(0x74),
  VKEY_F6(0x75),
  VKEY_F7(0x76),
  VKEY_F8(0x77),
  VKEY_F9(0x78),
  VKEY_F10(0x79),
  VKEY_F11(0x7A),
  VKEY_F12(0x7B),
  VKEY_F13(0x7C),
  VKEY_F14(0x7D),
  VKEY_F15(0x7E),
  VKEY_F16(0x7F),
  VKEY_F17(0x80),
  VKEY_F18(0x81),
  VKEY_F19(0x82),
  VKEY_F20(0x83),
  VKEY_F21(0x84),
  VKEY_F22(0x85),
  VKEY_F23(0x86),
  VKEY_F24(0x87),
  VKEY_NUMLOCK(0x90),
  VKEY_SCROLL(0x91),
  VKEY_LSHIFT(0xA0),
  VKEY_RSHIFT(0xA1),
  VKEY_LCONTROL(0xA2),
  VKEY_RCONTROL(0xA3),
  VKEY_LMENU(0xA4),
  VKEY_RMENU(0xA5),
  VKEY_BROWSER_BACK(0xA6),
  VKEY_BROWSER_FORWARD(0xA7),
  VKEY_BROWSER_REFRESH(0xA8),
  VKEY_BROWSER_STOP(0xA9),
  VKEY_BROWSER_SEARCH(0xAA),
  VKEY_BROWSER_FAVORITES(0xAB),
  VKEY_BROWSER_HOME(0xAC),
  VKEY_VOLUME_MUTE(0xAD),
  VKEY_VOLUME_DOWN(0xAE),
  VKEY_VOLUME_UP(0xAF),
  VKEY_MEDIA_NEXT_TRACK(0xB0),
  VKEY_MEDIA_PREV_TRACK(0xB1),
  VKEY_MEDIA_STOP(0xB2),
  VKEY_MEDIA_PLAY_PAUSE(0xB3),
  VKEY_MEDIA_LAUNCH_MAIL(0xB4),
  VKEY_MEDIA_LAUNCH_MEDIA_SELECT(0xB5),
  VKEY_MEDIA_LAUNCH_APP1(0xB6),
  VKEY_MEDIA_LAUNCH_APP2(0xB7),
  VKEY_OEM_1(0xBA),
  VKEY_OEM_PLUS(0xBB),
  VKEY_OEM_COMMA(0xBC),
  VKEY_OEM_MINUS(0xBD),
  VKEY_OEM_PERIOD(0xBE),
  VKEY_OEM_2(0xBF),
  VKEY_OEM_3(0xC0),
  VKEY_OEM_4(0xDB),
  VKEY_OEM_5(0xDC),
  VKEY_OEM_6(0xDD),
  VKEY_OEM_7(0xDE),
  VKEY_OEM_8(0xDF),
  VKEY_OEM_102(0xE2),
  // GTV KEYCODE_MEDIA_REWIND
  VKEY_OEM_103(0xE3),
  // GTV KEYCODE_MEDIA_FAST_FORWARD
  VKEY_OEM_104(0xE4),
  VKEY_PROCESSKEY(0xE5),
  VKEY_PACKET(0xE7),
  VKEY_DBE_SBCSCHAR(0xF3),
  VKEY_DBE_DBCSCHAR(0xF4),
  VKEY_ATTN(0xF6),
  VKEY_CRSEL(0xF7),
  VKEY_EXSEL(0xF8),
  VKEY_EREOF(0xF9),
  VKEY_PLAY(0xFA),
  VKEY_ZOOM(0xFB),
  VKEY_NONAME(0xFC),
  VKEY_PA1(0xFD),
  VKEY_OEM_CLEAR(0xFE),
  VKEY_UNKNOWN(0),

  // POSIX specific VKEYs. Note that as of Windows SDK 7.1, 0x97-9F, 0xD8-DA,
  // and 0xE8 are unassigned.
  VKEY_WLAN(0x97),
  VKEY_POWER(0x98),
  VKEY_BRIGHTNESS_DOWN(0xD8),
  VKEY_BRIGHTNESS_UP(0xD9),
  VKEY_KBD_BRIGHTNESS_DOWN(0xDA),
  VKEY_KBD_BRIGHTNESS_UP(0xE8),

  // Windows does not have a specific key code for AltGr. We use the unused 0xE1
  // (VK_OEM_AX) code to represent AltGr, matching the behaviour of Firefox on
  // Linux.
  VKEY_ALTGR(0xE1),
  // Windows does not have a specific key code for Compose. We use the unused
  // 0xE6 (VK_ICO_CLEAR) code to represent Compose.
  VKEY_COMPOSE(0xE6);

  final int code;
  const KeyboardCode(this.code);
}

KeyboardCode getWindowsKeyCode(LogicalKeyboardKey key) {
  // NOTE: Could not find the way to use switch-case here.

  // VKEY_BACK = 0x08;
  if (key == LogicalKeyboardKey.backspace) {
    return KeyboardCode.VKEY_BACK;
  }
  // VKEY_TAB = 0x09;
  else if (key == LogicalKeyboardKey.tab) {
    return KeyboardCode.VKEY_TAB;
  }
  // VKEY_BACKTAB = 0x0A;
  //// NOTE: no match

  // VKEY_CLEAR = 0x0C;
  else if (key == LogicalKeyboardKey.clear) {
    return KeyboardCode.VKEY_CLEAR;
  }
  // VKEY_RETURN = 0x0D;
  else if (key == LogicalKeyboardKey.enter) {
    return KeyboardCode.VKEY_RETURN;
  }
  // VKEY_SHIFT = 0x10;
  else if (key == LogicalKeyboardKey.shift) {
    return KeyboardCode.VKEY_SHIFT;
  }
  // VKEY_CONTROL = 0x11;
  else if (key == LogicalKeyboardKey.control) {
    return KeyboardCode.VKEY_CONTROL;
  }
  // VKEY_MENU = 0x12;
  else if (key == LogicalKeyboardKey.alt) {
    return KeyboardCode.VKEY_MENU;
  }
  // VKEY_PAUSE = 0x13;
  else if (key == LogicalKeyboardKey.pause) {
    return KeyboardCode.VKEY_PAUSE;
  }
  // VKEY_CAPITAL = 0x14;
  else if (key == LogicalKeyboardKey.capsLock) {
    return KeyboardCode.VKEY_CAPITAL;
  }
  // VKEY_KANA = 0x15;
  else if (key == LogicalKeyboardKey.hiraganaKatakana) {
    // NOTE: not sure if it is an exact match.
    // other candidates:
    // - LogicalKeyboardKey.hiragana
    // - LogicalKeyboardKey.katakana
    return KeyboardCode.VKEY_KANA;
  }
  // VKEY_HANGUL = 0x15;
  else if (key == LogicalKeyboardKey.hangulMode) {
    return KeyboardCode.VKEY_HANGUL;
  }
  // VKEY_JUNJA = 0x17;
  else if (key == LogicalKeyboardKey.junjaMode) {
    return KeyboardCode.VKEY_JUNJA;
  }
  // VKEY_FINAL = 0x18;
  else if (key == LogicalKeyboardKey.finalMode) {
    return KeyboardCode.VKEY_FINAL;
  }
  // VKEY_HANJA = 0x19;
  else if (key == LogicalKeyboardKey.hanjaMode) {
    return KeyboardCode.VKEY_HANJA;
  }
  // VKEY_KANJI = 0x19;
  else if (key == LogicalKeyboardKey.kanjiMode) {
    return KeyboardCode.VKEY_KANJI;
  }
  // VKEY_ESCAPE = 0x1B;
  else if (key == LogicalKeyboardKey.escape) {
    return KeyboardCode.VKEY_ESCAPE;
  }
  // VKEY_CONVERT = 0x1C;
  else if (key == LogicalKeyboardKey.convert) {
    return KeyboardCode.VKEY_CONVERT;
  }
  // VKEY_NONCONVERT = 0x1D;
  else if (key == LogicalKeyboardKey.nonConvert) {
    return KeyboardCode.VKEY_NONCONVERT;
  }
  // VKEY_ACCEPT = 0x1E;
  else if (key == LogicalKeyboardKey.accept) {
    return KeyboardCode.VKEY_ACCEPT;
  }
  // VKEY_MODECHANGE = 0x1F;
  else if (key == LogicalKeyboardKey.modeChange) {
    return KeyboardCode.VKEY_MODECHANGE;
  }
  // VKEY_SPACE = 0x20;
  else if (key == LogicalKeyboardKey.space) {
    return KeyboardCode.VKEY_SPACE;
  }
  // VKEY_PRIOR = 0x21;
  else if (key == LogicalKeyboardKey.pageUp) {
    return KeyboardCode.VKEY_PRIOR;
  }
  // VKEY_NEXT = 0x22;
  else if (key == LogicalKeyboardKey.pageDown) {
    return KeyboardCode.VKEY_NEXT;
  }
  // VKEY_END = 0x23;
  else if (key == LogicalKeyboardKey.end) {
    return KeyboardCode.VKEY_END;
  }
  // VKEY_HOME = 0x24;
  else if (key == LogicalKeyboardKey.home) {
    return KeyboardCode.VKEY_HOME;
  }
  // VKEY_LEFT = 0x25;
  else if (key == LogicalKeyboardKey.arrowLeft) {
    return KeyboardCode.VKEY_LEFT;
  }
  // VKEY_UP = 0x26;
  else if (key == LogicalKeyboardKey.arrowUp) {
    return KeyboardCode.VKEY_UP;
  }
  // VKEY_RIGHT = 0x27;
  else if (key == LogicalKeyboardKey.arrowRight) {
    return KeyboardCode.VKEY_RIGHT;
  }
  // VKEY_DOWN = 0x28;
  else if (key == LogicalKeyboardKey.arrowDown) {
    return KeyboardCode.VKEY_DOWN;
  }
  // VKEY_SELECT = 0x29;
  else if (key == LogicalKeyboardKey.select) {
    return KeyboardCode.VKEY_SELECT;
  }
  // VKEY_PRINT = 0x2A;
  else if (key == LogicalKeyboardKey.print) {
    return KeyboardCode.VKEY_PRINT;
  }
  // VKEY_EXECUTE = 0x2B;
  else if (key == LogicalKeyboardKey.execute) {
    return KeyboardCode.VKEY_EXECUTE;
  }
  // VKEY_SNAPSHOT = 0x2C;
  else if (key == LogicalKeyboardKey.printScreen) {
    return KeyboardCode.VKEY_SNAPSHOT;
  }
  // VKEY_INSERT = 0x2D;
  else if (key == LogicalKeyboardKey.insert) {
    return KeyboardCode.VKEY_INSERT;
  }
  // VKEY_DELETE = 0x2E;
  else if (key == LogicalKeyboardKey.delete) {
    return KeyboardCode.VKEY_DELETE;
  }
  // VKEY_HELP = 0x2F;
  else if (key == LogicalKeyboardKey.help) {
    return KeyboardCode.VKEY_HELP;
  }
  // VKEY_0 = 0x30;
  else if (key == LogicalKeyboardKey.digit0) {
    return KeyboardCode.VKEY_0;
  }
  // VKEY_1 = 0x31;
  else if (key == LogicalKeyboardKey.digit1) {
    return KeyboardCode.VKEY_1;
  }
  // VKEY_2 = 0x32;
  else if (key == LogicalKeyboardKey.digit2) {
    return KeyboardCode.VKEY_2;
  }
  // VKEY_3 = 0x33;
  else if (key == LogicalKeyboardKey.digit3) {
    return KeyboardCode.VKEY_3;
  }
  // VKEY_4 = 0x34;
  else if (key == LogicalKeyboardKey.digit4) {
    return KeyboardCode.VKEY_4;
  }
  // VKEY_5 = 0x35;
  else if (key == LogicalKeyboardKey.digit5) {
    return KeyboardCode.VKEY_5;
  }
  // VKEY_6 = 0x36;
  else if (key == LogicalKeyboardKey.digit6) {
    return KeyboardCode.VKEY_6;
  }
  // VKEY_7 = 0x37;
  else if (key == LogicalKeyboardKey.digit7) {
    return KeyboardCode.VKEY_7;
  }
  // VKEY_8 = 0x38;
  else if (key == LogicalKeyboardKey.digit8) {
    return KeyboardCode.VKEY_8;
  }
  // VKEY_9 = 0x39;
  else if (key == LogicalKeyboardKey.digit9) {
    return KeyboardCode.VKEY_9;
  }
  // VKEY_A = 0x41;
  else if (key == LogicalKeyboardKey.keyA) {
    return KeyboardCode.VKEY_A;
  }
  // VKEY_B = 0x42;
  else if (key == LogicalKeyboardKey.keyB) {
    return KeyboardCode.VKEY_B;
  }
  // VKEY_C = 0x43;
  else if (key == LogicalKeyboardKey.keyC) {
    return KeyboardCode.VKEY_C;
  }
  // VKEY_D = 0x44;
  else if (key == LogicalKeyboardKey.keyD) {
    return KeyboardCode.VKEY_D;
  }
  // VKEY_E = 0x45;
  else if (key == LogicalKeyboardKey.keyE) {
    return KeyboardCode.VKEY_E;
  }
  // VKEY_F = 0x46;
  else if (key == LogicalKeyboardKey.keyF) {
    return KeyboardCode.VKEY_F;
  }
  // VKEY_G = 0x47;
  else if (key == LogicalKeyboardKey.keyG) {
    return KeyboardCode.VKEY_G;
  }
  // VKEY_H = 0x48;
  else if (key == LogicalKeyboardKey.keyH) {
    return KeyboardCode.VKEY_H;
  }
  // VKEY_I = 0x49;
  else if (key == LogicalKeyboardKey.keyI) {
    return KeyboardCode.VKEY_I;
  }
  // VKEY_J = 0x4A;
  else if (key == LogicalKeyboardKey.keyJ) {
    return KeyboardCode.VKEY_J;
  }
  // VKEY_K = 0x4B;
  else if (key == LogicalKeyboardKey.keyK) {
    return KeyboardCode.VKEY_K;
  }
  // VKEY_L = 0x4C;
  else if (key == LogicalKeyboardKey.keyL) {
    return KeyboardCode.VKEY_L;
  }
  // VKEY_M = 0x4D;
  else if (key == LogicalKeyboardKey.keyM) {
    return KeyboardCode.VKEY_M;
  }
  // VKEY_N = 0x4E;
  else if (key == LogicalKeyboardKey.keyN) {
    return KeyboardCode.VKEY_N;
  }
  // VKEY_O = 0x4F;
  else if (key == LogicalKeyboardKey.keyO) {
    return KeyboardCode.VKEY_O;
  }
  // VKEY_P = 0x50;
  else if (key == LogicalKeyboardKey.keyP) {
    return KeyboardCode.VKEY_P;
  }
  // VKEY_Q = 0x51;
  else if (key == LogicalKeyboardKey.keyQ) {
    return KeyboardCode.VKEY_Q;
  }
  // VKEY_R = 0x52;
  else if (key == LogicalKeyboardKey.keyR) {
    return KeyboardCode.VKEY_R;
  }
  // VKEY_S = 0x53;
  else if (key == LogicalKeyboardKey.keyS) {
    return KeyboardCode.VKEY_S;
  }
  // VKEY_T = 0x54;
  else if (key == LogicalKeyboardKey.keyT) {
    return KeyboardCode.VKEY_T;
  }
  // VKEY_U = 0x55;
  else if (key == LogicalKeyboardKey.keyU) {
    return KeyboardCode.VKEY_U;
  }
  // VKEY_V = 0x56;
  else if (key == LogicalKeyboardKey.keyV) {
    return KeyboardCode.VKEY_V;
  }
  // VKEY_W = 0x57;
  else if (key == LogicalKeyboardKey.keyW) {
    return KeyboardCode.VKEY_W;
  }
  // VKEY_X = 0x58;
  else if (key == LogicalKeyboardKey.keyX) {
    return KeyboardCode.VKEY_X;
  }
  // VKEY_Y = 0x59;
  else if (key == LogicalKeyboardKey.keyY) {
    return KeyboardCode.VKEY_Y;
  }
  // VKEY_Z = 0x5A;
  else if (key == LogicalKeyboardKey.keyZ) {
    return KeyboardCode.VKEY_Z;
  }
  // VKEY_LWIN = 0x5B;
  else if (key == LogicalKeyboardKey.metaLeft) {
    // NOTE: not sure if it is an exact match.
    // another candidate:
    //   LogicalKeyboardKey.meta
    return KeyboardCode.VKEY_LWIN;
  }
  // VKEY_COMMAND = VKEY_LWIN; // Provide the Mac name for convenience.
  else if (key == LogicalKeyboardKey.superKey) {
    return KeyboardCode.VKEY_LWIN;
  }
  // VKEY_RWIN = 0x5C;
  else if (key == LogicalKeyboardKey.metaRight) {
    // NOTE: not sure if it is an exact match.
    // another candidate:
    //   LogicalKeyboardKey.meta
    return KeyboardCode.VKEY_RWIN;
  }
  // VKEY_APPS = 0x5D;
  else if (key == LogicalKeyboardKey.appSwitch) {
    // NOTE: not sure if it is an exact match.
    // another candidate:
    //   LogicalKeyboardKey.mediaApps
    return KeyboardCode.VKEY_APPS;
  }
  // VKEY_SLEEP = 0x5F;
  else if (key == LogicalKeyboardKey.sleep) {
    return KeyboardCode.VKEY_SLEEP;
  }
  // VKEY_NUMPAD0 = 0x60;
  else if (key == LogicalKeyboardKey.numpad0) {
    return KeyboardCode.VKEY_NUMPAD0;
  }
  // VKEY_NUMPAD1 = 0x61;
  else if (key == LogicalKeyboardKey.numpad1) {
    return KeyboardCode.VKEY_NUMPAD1;
  }
  // VKEY_NUMPAD2 = 0x62;
  else if (key == LogicalKeyboardKey.numpad2) {
    return KeyboardCode.VKEY_NUMPAD2;
  }
  // VKEY_NUMPAD3 = 0x63;
  else if (key == LogicalKeyboardKey.numpad3) {
    return KeyboardCode.VKEY_NUMPAD3;
  }
  // VKEY_NUMPAD4 = 0x64;
  else if (key == LogicalKeyboardKey.numpad4) {
    return KeyboardCode.VKEY_NUMPAD4;
  }
  // VKEY_NUMPAD5 = 0x65;
  else if (key == LogicalKeyboardKey.numpad5) {
    return KeyboardCode.VKEY_NUMPAD5;
  }
  // VKEY_NUMPAD6 = 0x66;
  else if (key == LogicalKeyboardKey.numpad6) {
    return KeyboardCode.VKEY_NUMPAD6;
  }
  // VKEY_NUMPAD7 = 0x67;
  else if (key == LogicalKeyboardKey.numpad7) {
    return KeyboardCode.VKEY_NUMPAD7;
  }
  // VKEY_NUMPAD8 = 0x68;
  else if (key == LogicalKeyboardKey.numpad8) {
    return KeyboardCode.VKEY_NUMPAD8;
  }
  // VKEY_NUMPAD9 = 0x69;
  else if (key == LogicalKeyboardKey.numpad9) {
    return KeyboardCode.VKEY_NUMPAD9;
  }
  // VKEY_MULTIPLY = 0x6A;
  else if (key == LogicalKeyboardKey.numpadMultiply) {
    return KeyboardCode.VKEY_MULTIPLY;
  }
  // VKEY_ADD = 0x6B;
  else if (key == LogicalKeyboardKey.numpadAdd) {
    return KeyboardCode.VKEY_ADD;
  }
  // VKEY_SEPARATOR = 0x6C;
  else if (key == LogicalKeyboardKey.numpadEnter) {
    return KeyboardCode.VKEY_SEPARATOR;
  }
  // VKEY_SUBTRACT = 0x6D;
  else if (key == LogicalKeyboardKey.numpadSubtract) {
    return KeyboardCode.VKEY_SUBTRACT;
  }
  // VKEY_DECIMAL = 0x6E;
  else if (key == LogicalKeyboardKey.numpadDecimal) {
    return KeyboardCode.VKEY_DECIMAL;
  }
  // VKEY_DIVIDE = 0x6F;
  else if (key == LogicalKeyboardKey.numpadDivide) {
    return KeyboardCode.VKEY_DIVIDE;
  }
  // VKEY_F1 = 0x70;
  else if (key == LogicalKeyboardKey.f1) {
    return KeyboardCode.VKEY_F1;
  }
  // VKEY_F2 = 0x71;
  else if (key == LogicalKeyboardKey.f2) {
    return KeyboardCode.VKEY_F2;
  }
  // VKEY_F3 = 0x72;
  else if (key == LogicalKeyboardKey.f3) {
    return KeyboardCode.VKEY_F3;
  }
  // VKEY_F4 = 0x73;
  else if (key == LogicalKeyboardKey.f4) {
    return KeyboardCode.VKEY_F4;
  }
  // VKEY_F5 = 0x74;
  else if (key == LogicalKeyboardKey.f5) {
    return KeyboardCode.VKEY_F5;
  }
  // VKEY_F6 = 0x75;
  else if (key == LogicalKeyboardKey.f6) {
    return KeyboardCode.VKEY_F6;
  }
  // VKEY_F7 = 0x76;
  else if (key == LogicalKeyboardKey.f7) {
    return KeyboardCode.VKEY_F7;
  }
  // VKEY_F8 = 0x77;
  else if (key == LogicalKeyboardKey.f8) {
    return KeyboardCode.VKEY_F8;
  }
  // VKEY_F9 = 0x78;
  else if (key == LogicalKeyboardKey.f9) {
    return KeyboardCode.VKEY_F9;
  }
  // VKEY_F10 = 0x79;
  else if (key == LogicalKeyboardKey.f10) {
    return KeyboardCode.VKEY_F10;
  }
  // VKEY_F11 = 0x7A;
  else if (key == LogicalKeyboardKey.f11) {
    return KeyboardCode.VKEY_F11;
  }
  // VKEY_F12 = 0x7B;
  else if (key == LogicalKeyboardKey.f12) {
    return KeyboardCode.VKEY_F12;
  }
  // VKEY_F13 = 0x7C;
  else if (key == LogicalKeyboardKey.f13) {
    return KeyboardCode.VKEY_F13;
  }
  // VKEY_F14 = 0x7D;
  else if (key == LogicalKeyboardKey.f14) {
    return KeyboardCode.VKEY_F14;
  }
  // VKEY_F15 = 0x7E;
  else if (key == LogicalKeyboardKey.f15) {
    return KeyboardCode.VKEY_F15;
  }
  // VKEY_F16 = 0x7F;
  else if (key == LogicalKeyboardKey.f16) {
    return KeyboardCode.VKEY_F16;
  }
  // VKEY_F17 = 0x80;
  else if (key == LogicalKeyboardKey.f17) {
    return KeyboardCode.VKEY_F17;
  }
  // VKEY_F18 = 0x81;
  else if (key == LogicalKeyboardKey.f18) {
    return KeyboardCode.VKEY_F18;
  }
  // VKEY_F19 = 0x82;
  else if (key == LogicalKeyboardKey.f19) {
    return KeyboardCode.VKEY_F19;
  }
  // VKEY_F20 = 0x83;
  else if (key == LogicalKeyboardKey.f20) {
    return KeyboardCode.VKEY_F20;
  }
  // VKEY_F21 = 0x84;
  else if (key == LogicalKeyboardKey.f21) {
    return KeyboardCode.VKEY_F21;
  }
  // VKEY_F22 = 0x85;
  else if (key == LogicalKeyboardKey.f22) {
    return KeyboardCode.VKEY_F22;
  }
  // VKEY_F23 = 0x86;
  else if (key == LogicalKeyboardKey.f23) {
    return KeyboardCode.VKEY_F23;
  }
  // VKEY_F24 = 0x87;
  else if (key == LogicalKeyboardKey.f24) {
    return KeyboardCode.VKEY_F24;
  }
  // VKEY_NUMLOCK = 0x90;
  else if (key == LogicalKeyboardKey.numLock) {
    return KeyboardCode.VKEY_NUMLOCK;
  }
  // VKEY_SCROLL = 0x91;
  else if (key == LogicalKeyboardKey.scrollLock) {
    return KeyboardCode.VKEY_SCROLL;
  }
  // VKEY_LSHIFT = 0xA0;
  else if (key == LogicalKeyboardKey.shiftLeft) {
    return KeyboardCode.VKEY_LSHIFT;
  }
  // VKEY_RSHIFT = 0xA1;
  else if (key == LogicalKeyboardKey.shiftRight) {
    return KeyboardCode.VKEY_RSHIFT;
  }
  // VKEY_LCONTROL = 0xA2;
  else if (key == LogicalKeyboardKey.controlLeft) {
    return KeyboardCode.VKEY_LCONTROL;
  }
  // VKEY_RCONTROL = 0xA3;
  else if (key == LogicalKeyboardKey.controlRight) {
    return KeyboardCode.VKEY_RCONTROL;
  }
  // VKEY_LMENU = 0xA4;
  else if (key == LogicalKeyboardKey.altLeft) {
    return KeyboardCode.VKEY_LMENU;
  }
  // VKEY_RMENU = 0xA5;
  else if (key == LogicalKeyboardKey.altRight) {
    return KeyboardCode.VKEY_RMENU;
  }
  // VKEY_BROWSER_BACK = 0xA6;
  else if (key == LogicalKeyboardKey.browserBack) {
    return KeyboardCode.VKEY_BROWSER_BACK;
  }
  // VKEY_BROWSER_FORWARD = 0xA7;
  else if (key == LogicalKeyboardKey.browserForward) {
    return KeyboardCode.VKEY_BROWSER_FORWARD;
  }
  // VKEY_BROWSER_REFRESH = 0xA8;
  else if (key == LogicalKeyboardKey.browserRefresh) {
    return KeyboardCode.VKEY_BROWSER_REFRESH;
  }
  // VKEY_BROWSER_STOP = 0xA9;
  else if (key == LogicalKeyboardKey.browserStop) {
    return KeyboardCode.VKEY_BROWSER_STOP;
  }
  // VKEY_BROWSER_SEARCH = 0xAA;
  else if (key == LogicalKeyboardKey.browserSearch) {
    return KeyboardCode.VKEY_BROWSER_SEARCH;
  }
  // VKEY_BROWSER_FAVORITES = 0xAB;
  else if (key == LogicalKeyboardKey.browserFavorites) {
    return KeyboardCode.VKEY_BROWSER_FAVORITES;
  }
  // VKEY_BROWSER_HOME = 0xAC;
  else if (key == LogicalKeyboardKey.browserHome) {
    return KeyboardCode.VKEY_BROWSER_HOME;
  }
  // VKEY_VOLUME_MUTE = 0xAD;
  else if (key == LogicalKeyboardKey.audioVolumeMute) {
    return KeyboardCode.VKEY_VOLUME_MUTE;
  }
  // VKEY_VOLUME_DOWN = 0xAE;
  else if (key == LogicalKeyboardKey.audioVolumeDown) {
    return KeyboardCode.VKEY_VOLUME_DOWN;
  }
  // VKEY_VOLUME_UP = 0xAF;
  else if (key == LogicalKeyboardKey.audioVolumeUp) {
    return KeyboardCode.VKEY_VOLUME_UP;
  }
  // VKEY_MEDIA_NEXT_TRACK = 0xB0;
  else if (key == LogicalKeyboardKey.mediaTrackNext) {
    return KeyboardCode.VKEY_MEDIA_NEXT_TRACK;
  }
  // VKEY_MEDIA_PREV_TRACK = 0xB1;
  else if (key == LogicalKeyboardKey.mediaTrackPrevious) {
    return KeyboardCode.VKEY_MEDIA_PREV_TRACK;
  }
  // VKEY_MEDIA_STOP = 0xB2;
  else if (key == LogicalKeyboardKey.mediaStop) {
    return KeyboardCode.VKEY_MEDIA_STOP;
  }
  // VKEY_MEDIA_PLAY_PAUSE = 0xB3;
  else if (key == LogicalKeyboardKey.mediaPlayPause) {
    // NOTE: not sure if it is an exact match.
    // another candidate:
    //   LogicalKeyboardKey.mediaPause
    return KeyboardCode.VKEY_MEDIA_PLAY_PAUSE;
  }
  // VKEY_MEDIA_LAUNCH_MAIL = 0xB4;
  else if (key == LogicalKeyboardKey.launchMail) {
    return KeyboardCode.VKEY_MEDIA_LAUNCH_MAIL;
  }
  // VKEY_MEDIA_LAUNCH_MEDIA_SELECT = 0xB5;
  else if (key == LogicalKeyboardKey.launchMediaPlayer) {
    // NOTE: not sure if it is an exact match.
    // another candidate:
    //   LogicalKeyboardKey.launchMusicPlayer
    return KeyboardCode.VKEY_MEDIA_LAUNCH_MEDIA_SELECT;
  }
  // VKEY_MEDIA_LAUNCH_APP1 = 0xB6;
  else if (key == LogicalKeyboardKey.launchApplication1) {
    return KeyboardCode.VKEY_MEDIA_LAUNCH_APP1;
  }
  // VKEY_MEDIA_LAUNCH_APP2 = 0xB7;
  else if (key == LogicalKeyboardKey.launchApplication2) {
    return KeyboardCode.VKEY_MEDIA_LAUNCH_APP2;
  }
  // VKEY_OEM_1 = 0xBA;
  else if (key == LogicalKeyboardKey.colon) {
    return KeyboardCode.VKEY_OEM_1;
  }
  // VKEY_OEM_PLUS = 0xBB;
  else if (key == LogicalKeyboardKey.add) {
    return KeyboardCode.VKEY_OEM_PLUS;
  }
  // VKEY_OEM_COMMA = 0xBC;
  else if (key == LogicalKeyboardKey.comma) {
    return KeyboardCode.VKEY_OEM_COMMA;
  }
  // VKEY_OEM_MINUS = 0xBD;
  else if (key == LogicalKeyboardKey.minus) {
    return KeyboardCode.VKEY_OEM_MINUS;
  }
  // VKEY_OEM_PERIOD = 0xBE;
  else if (key == LogicalKeyboardKey.period) {
    return KeyboardCode.VKEY_OEM_PERIOD;
  }
  // VKEY_OEM_2 = 0xBF;
  else if (key == LogicalKeyboardKey.slash) {
    return KeyboardCode.VKEY_OEM_2;
  }
  // VKEY_OEM_3 = 0xC0;
  else if (key == LogicalKeyboardKey.backquote) {
    return KeyboardCode.VKEY_OEM_3;
  }
  // VKEY_OEM_4 = 0xDB;
  else if (key == LogicalKeyboardKey.bracketLeft) {
    return KeyboardCode.VKEY_OEM_4;
  }
  // VKEY_OEM_5 = 0xDC;
  else if (key == LogicalKeyboardKey.backslash) {
    return KeyboardCode.VKEY_OEM_5;
  }
  // VKEY_OEM_6 = 0xDD;
  else if (key == LogicalKeyboardKey.bracketRight) {
    return KeyboardCode.VKEY_OEM_6;
  }
  // VKEY_OEM_7 = 0xDE;
  else if (key == LogicalKeyboardKey.quoteSingle) {
    return KeyboardCode.VKEY_OEM_7;
  }
  // VKEY_OEM_8 = 0xDF;
  //// NOTE: unknown

  // VKEY_OEM_102 = 0xE2;
  //// NOTE: unknown

  // VKEY_OEM_103 = 0xE3; // GTV KEYCODE_MEDIA_REWIND
  else if (key == LogicalKeyboardKey.mediaRewind) {
    return KeyboardCode.VKEY_OEM_103;
  }
  // VKEY_OEM_104 = 0xE4; // GTV KEYCODE_MEDIA_FAST_FORWARD
  else if (key == LogicalKeyboardKey.mediaFastForward) {
    return KeyboardCode.VKEY_OEM_104;
  }
  // VKEY_PROCESSKEY = 0xE5;
  else if (key == LogicalKeyboardKey.process) {
    // Not sure.
    return KeyboardCode.VKEY_PROCESSKEY;
  }
  // VKEY_PACKET = 0xE7;
  //// NOTE: no match

  // VKEY_DBE_SBCSCHAR = 0xF3;
  else if (key == LogicalKeyboardKey.zenkakuHankaku) {
    // NOTE: not sure
    return KeyboardCode.VKEY_PROCESSKEY;
  }
  // VKEY_DBE_DBCSCHAR = 0xF4;
  else if (key == LogicalKeyboardKey.zenkakuHankaku) {
    // NOTE: not sure
    return KeyboardCode.VKEY_PROCESSKEY;
  }
  // VKEY_ATTN = 0xF6;
  else if (key == LogicalKeyboardKey.attn) {
    return KeyboardCode.VKEY_ATTN;
  }
  // VKEY_CRSEL = 0xF7;
  else if (key == LogicalKeyboardKey.crSel) {
    return KeyboardCode.VKEY_CRSEL;
  }
  // VKEY_EXSEL = 0xF8;
  else if (key == LogicalKeyboardKey.exSel) {
    return KeyboardCode.VKEY_EXSEL;
  }
  // VKEY_EREOF = 0xF9;
  else if (key == LogicalKeyboardKey.eraseEof) {
    return KeyboardCode.VKEY_EREOF;
  }
  // VKEY_PLAY = 0xFA;
  else if (key == LogicalKeyboardKey.play) {
    // NOTE: not sure if it is an exact match.
    // another candidate:
    //   LogicalKeyboardKey.mediaPlay
    return KeyboardCode.VKEY_PLAY;
  }
  // VKEY_ZOOM = 0xFB;
  else if (key == LogicalKeyboardKey.zoomToggle) {
    // NOTE: not sure if it is an exact match.
    // other candidates:
    // - LogicalKeyboardKey.zoomIn
    // - LogicalKeyboardKey.zoomOut
    return KeyboardCode.VKEY_ZOOM;
  }
  // VKEY_NONAME = 0xFC;
  //// NOTE: no match

  // VKEY_PA1 = 0xFD;
  //// NOTE: no match

  // VKEY_OEM_CLEAR = 0xFE;
  else if (key == LogicalKeyboardKey.clear) {
    return KeyboardCode.VKEY_OEM_CLEAR;
  }
  // VKEY_UNKNOWN = 0;
  //// NOTE: no match

  // VKEY_WLAN = 0x97;
  //// NOTE: no match

  // VKEY_POWER = 0x98;
  else if (key == LogicalKeyboardKey.power) {
    return KeyboardCode.VKEY_POWER;
  }
  // VKEY_BRIGHTNESS_DOWN = 0xD8;
  else if (key == LogicalKeyboardKey.brightnessDown) {
    return KeyboardCode.VKEY_BRIGHTNESS_DOWN;
  }
  // VKEY_BRIGHTNESS_UP = 0xD9;
  else if (key == LogicalKeyboardKey.brightnessUp) {
    return KeyboardCode.VKEY_BRIGHTNESS_UP;
  }
  // VKEY_KBD_BRIGHTNESS_DOWN = 0xDA;
  else if (key == LogicalKeyboardKey.brightnessDown) {
    return KeyboardCode.VKEY_KBD_BRIGHTNESS_DOWN;
  }
  // VKEY_KBD_BRIGHTNESS_UP = 0xE8;
  else if (key == LogicalKeyboardKey.brightnessUp) {
    return KeyboardCode.VKEY_KBD_BRIGHTNESS_UP;
  }
  // VKEY_ALTGR = 0xE1;
  else if (key == LogicalKeyboardKey.altGraph) {
    return KeyboardCode.VKEY_ALTGR;
  }
  // VKEY_COMPOSE = 0xE6;
  else if (key == LogicalKeyboardKey.compose) {
    return KeyboardCode.VKEY_COMPOSE;
  }

  return KeyboardCode.VKEY_UNKNOWN;
}

KeyboardCode getWindowsKeyCodeWithoutLocation(KeyboardCode windowsKeyCode) {
  switch (windowsKeyCode) {
    case KeyboardCode.VKEY_LCONTROL:
    case KeyboardCode.VKEY_RCONTROL:
      return KeyboardCode.VKEY_CONTROL;
    case KeyboardCode.VKEY_LSHIFT:
    case KeyboardCode.VKEY_RSHIFT:
      return KeyboardCode.VKEY_SHIFT;
    case KeyboardCode.VKEY_LMENU:
    case KeyboardCode.VKEY_RMENU:
      return KeyboardCode.VKEY_MENU;
    default:
      return windowsKeyCode;
  }
}

// From content/browser/renderer_host/input/web_input_event_builders_gtk.cc.
// Gets the corresponding control character of a specified key code. See:
// http://en.wikipedia.org/wiki/Control_characters
// We emulate Windows behavior here.
int getControlCharacter(KeyboardCode windowsKeyCode, bool shift) {
  if (windowsKeyCode.code >= KeyboardCode.VKEY_A.code &&
      windowsKeyCode.code <= KeyboardCode.VKEY_Z.code) {
    // ctrl-A ~ ctrl-Z map to \x01 ~ \x1A
    return windowsKeyCode.code - KeyboardCode.VKEY_A.code + 1;
  }
  if (shift) {
    // following graphics chars require shift key to input.
    switch (windowsKeyCode) {
      // ctrl-@ maps to \x00 (Null byte)
      case KeyboardCode.VKEY_2:
        return 0;
      // ctrl-^ maps to \x1E (Record separator, Information separator two)
      case KeyboardCode.VKEY_6:
        return 0x1E;
      // ctrl-_ maps to \x1F (Unit separator, Information separator one)
      case KeyboardCode.VKEY_OEM_MINUS:
        return 0x1F;
      // Returns 0 for all other keys to avoid inputting unexpected chars.
      default:
        return 0;
    }
  } else {
    switch (windowsKeyCode) {
      // ctrl-[ maps to \x1B (Escape)
      case KeyboardCode.VKEY_OEM_4:
        return 0x1B;
      // ctrl-\ maps to \x1C (File separator, Information separator four)
      case KeyboardCode.VKEY_OEM_5:
        return 0x1C;
      // ctrl-] maps to \x1D (Group separator, Information separator three)
      case KeyboardCode.VKEY_OEM_6:
        return 0x1D;
      // ctrl-Enter maps to \x0A (Line feed)
      case KeyboardCode.VKEY_RETURN:
        return 0x0A;
      // Returns 0 for all other keys to avoid inputting unexpected chars.
      default:
        return 0;
    }
  }
}
