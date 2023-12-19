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

#ifndef LINUX_SUBPROCESS_SRC_FLUTTER_WEBVIEW_PROCESS_MESSAGES_H_
#define LINUX_SUBPROCESS_SRC_FLUTTER_WEBVIEW_PROCESS_MESSAGES_H_

#include <string>

#include "include/cef_process_message.h"

namespace flutter_webview_process_messages {

const char kFrameMsg_RequestRunJavascript[] = "FrameMsg_RequestRunJavascript";
const char kFrameHostMsg_RunJavascriptResponse[] =
    "FrameHostMsg_RunJavascriptResponse";

// -----------------------------------------------------------------------------
// Messages sent from the brwoser to the renderer.

// Request to execute the javascript in the renderer. The result of the
// execution is returned with FrameHostMsg_RunJavascriptResponse.
//
// FrameMsg_RequestRunJavascript message format:
// 0.  js_run_id: int
//     The javascript execution ID, given by Dart side.
// 1.  javascript: string
//     The javascript string to be executed on the renderer process.
CefRefPtr<CefProcessMessage> Create_FrameMsg_RequestRunJavascript(
    int js_run_id,
    std::string javascript);
void Read_FrameMsg_RequestRunJavascript(CefRefPtr<CefProcessMessage> message,
                                        int* out_js_run_id,
                                        std::string* out_javascript);

// -----------------------------------------------------------------------------
// Messages sent from the renderer to the browser.

// Response to FrameMsg_RequestRunJavascript.
//
// FrameHostMsg_RunJavascriptResponse message format:
// 0.  js_run_id: int
//     The javascript execution ID.
// 1.  was_executed: bool
//     Whether the javascript was executed or not.
// 2.  is_exception: bool
//     Whether the result is an exception or not.
// 3.  js_result: string
//     The result of the javascript execution.
// 4.  is_undefined: bool
//     Whether the result is an undefined value.
CefRefPtr<CefProcessMessage> Create_FrameHostMsg_RunJavascriptResponse(
    int js_run_id,
    bool was_executed,
    bool is_exception,
    std::string js_result,
    bool is_undefined);
void Read_FrameHostMsg_RunJavascriptResponse(
    CefRefPtr<CefProcessMessage> message,
    int* out_js_run_id,
    bool* out_was_executed,
    bool* out_is_exception,
    std::string* out_result,
    bool* out_is_undefined);

}  // namespace flutter_webview_process_messages

#endif  // LINUX_SUBPROCESS_SRC_FLUTTER_WEBVIEW_PROCESS_MESSAGES_H_
