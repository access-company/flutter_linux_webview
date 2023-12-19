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

// This file is based on  https://bitbucket.org/chromiumembedded/cef/src/4664/include/internal/cef_types.h
// and translates some of the CEF types defined in it into Dart.

// Copyright (c) 2014 Marshall A. Greenblatt. All rights reserved.
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

// Translation of cef_mouse_button_type_t
///
/// Mouse button types.
///
enum CefMouseButtonType {
  MBT_LEFT(0),
  MBT_MIDDLE(1),
  MBT_RIGHT(2);

  final int value;
  const CefMouseButtonType(this.value);
}

// Translation of cef_event_flags_t
///
/// Supported event bit flags.
///
enum CefEventFlags {
  EVENTFLAG_NONE(0),
  EVENTFLAG_CAPS_LOCK_ON(1 << 0),
  EVENTFLAG_SHIFT_DOWN(1 << 1),
  EVENTFLAG_CONTROL_DOWN(1 << 2),
  EVENTFLAG_ALT_DOWN(1 << 3),
  EVENTFLAG_LEFT_MOUSE_BUTTON(1 << 4),
  EVENTFLAG_MIDDLE_MOUSE_BUTTON(1 << 5),
  EVENTFLAG_RIGHT_MOUSE_BUTTON(1 << 6),
  // Mac OS-X command key.
  EVENTFLAG_COMMAND_DOWN(1 << 7),
  EVENTFLAG_NUM_LOCK_ON(1 << 8),
  EVENTFLAG_IS_KEY_PAD(1 << 9),
  EVENTFLAG_IS_LEFT(1 << 10),
  EVENTFLAG_IS_RIGHT(1 << 11),
  EVENTFLAG_ALTER_DOWN(1 << 12),
  EVENTFLAG_IS_REPEAT(1 << 13);

  final int value;
  const CefEventFlags(this.value);
}

// Translation of cef_key_event_type_t
///
/// Key event types.
///
enum CefKeyEventType {
  ///
  /// Notification that a key transitioned from "up" to "down".
  ///
  KEYEVENT_RAWKEYDOWN(0),

  ///
  /// Notification that a key was pressed. This does not necessarily correspond
  /// to a character depending on the key and language. Use KEYEVENT_CHAR for
  /// character input.
  ///
  KEYEVENT_KEYDOWN(1),

  ///
  /// Notification that a key was released.
  ///
  KEYEVENT_KEYUP(2),

  ///
  /// Notification that a character was typed. Use this for text input. Key
  /// down events may generate 0, 1, or more than one character event depending
  /// on the key, locale, and operating system.
  ///
  KEYEVENT_CHAR(3);

  final int value;
  const CefKeyEventType(this.value);
}
