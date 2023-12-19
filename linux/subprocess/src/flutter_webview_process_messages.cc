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

#include "flutter_webview_process_messages.h"

#include <string>

namespace flutter_webview_process_messages {

CefRefPtr<CefProcessMessage> Create_FrameMsg_RequestRunJavascript(
    int js_run_id,
    std::string javascript) {
  CefRefPtr<CefProcessMessage> msg =
      CefProcessMessage::Create(kFrameMsg_RequestRunJavascript);
  CefRefPtr<CefListValue> args = msg->GetArgumentList();
  args->SetInt(0, js_run_id);
  args->SetString(1, javascript);
  return msg;
}

void Read_FrameMsg_RequestRunJavascript(CefRefPtr<CefProcessMessage> message,
                                        int* out_js_run_id,
                                        std::string* out_javascript) {
  message->GetArgumentList();
  CefRefPtr<CefListValue> args = message->GetArgumentList();
  *out_js_run_id = args->GetInt(0);
  *out_javascript = args->GetString(1);
}

CefRefPtr<CefProcessMessage> Create_FrameHostMsg_RunJavascriptResponse(
    int js_run_id,
    bool was_executed,
    bool is_exception,
    std::string js_result,
    bool is_undefined) {
  CefRefPtr<CefProcessMessage> msg =
      CefProcessMessage::Create(kFrameHostMsg_RunJavascriptResponse);
  CefRefPtr<CefListValue> args = msg->GetArgumentList();
  args->SetInt(0, js_run_id);
  args->SetBool(1, was_executed);
  args->SetBool(2, is_exception);
  args->SetString(3, js_result);
  args->SetBool(4, is_undefined);
  return msg;
}

void Read_FrameHostMsg_RunJavascriptResponse(
    CefRefPtr<CefProcessMessage> message,
    int* out_js_run_id,
    bool* out_was_executed,
    bool* out_is_exception,
    std::string* out_result,
    bool* out_is_undefined) {
  message->GetArgumentList();
  CefRefPtr<CefListValue> args = message->GetArgumentList();
  *out_js_run_id = args->GetInt(0);
  *out_was_executed = args->GetBool(1);
  *out_is_exception = args->GetBool(2);
  *out_result = args->GetString(3);
  *out_is_undefined = args->GetBool(4);
}

}  // namespace flutter_webview_process_messages
