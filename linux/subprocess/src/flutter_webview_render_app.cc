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

#include "flutter_webview_render_app.h"

#include <string>

#include "flutter_webview_process_messages.h"
#include "include/base/cef_logging.h"
#include "include/cef_browser.h"
#include "include/wrapper/cef_helpers.h"

namespace {

CefRefPtr<CefV8Value> JSONStringify(CefRefPtr<CefV8Context> context,
                                    CefRefPtr<CefV8Value> in_value) {
  auto global = context->GetGlobal();
  auto json = global->GetValue("JSON");
  if (!json) {
    return nullptr;
  }
  auto stringify = json->GetValue("stringify");
  if (!stringify) {
    return nullptr;
  }
  CefV8ValueList args;
  args.push_back(in_value);
  return stringify->ExecuteFunction(json, args);
}

}  // namespace

FlutterWebviewRenderApp::FlutterWebviewRenderApp() {}

void FlutterWebviewRenderApp::OnContextCreated(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefV8Context> context) {
  VLOG(1) << "CefRenderProcessHandler::OnContextCreated called";
}

bool FlutterWebviewRenderApp::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message) {
  const std::string& message_name = message->GetName();

  if (message_name ==
      flutter_webview_process_messages::kFrameMsg_RequestRunJavascript) {
    int js_run_id;
    std::string javascript;
    flutter_webview_process_messages::Read_FrameMsg_RequestRunJavascript(
        message, &js_run_id, &javascript);

    VLOG(1) << "The renderer process received the JS to execute: js_run_id="
            << js_run_id << ", javascript=" << javascript;

    bool was_executed = false;
    bool is_exception = false;
    std::string result = "";
    bool is_undefined = false;

    // run v8
    CefRefPtr<CefV8Context> context = frame->GetV8Context();
    if (context.get() && context->Enter()) {
      CefRefPtr<CefV8Exception> exception;
      CefRefPtr<CefV8Value> retval;

      was_executed = true;

      if (!context->Eval(javascript, frame->GetURL(), 1, retval, exception)) {
        // exception
        is_exception = true;
        result = exception->GetMessage();
      } else {
        CefRefPtr<CefV8Value> v8result = JSONStringify(context, retval);
        if (!v8result) {
          is_exception = true;
          result = "JSON.stringify() failed";
        } else {
          if (v8result->IsUndefined()) {
            VLOG(1) << "V8result: (undefined)";
            is_exception = false;
            result = "undefined";
            is_undefined = true;
          } else if (v8result->IsString()) {
            VLOG(1) << "V8result: " << v8result->GetStringValue();
            is_exception = false;
            result = v8result->GetStringValue();
            is_undefined = false;
          }
        }
      }
      context->Exit();
    }

    CefRefPtr<CefProcessMessage> response = flutter_webview_process_messages::
        Create_FrameHostMsg_RunJavascriptResponse(
            js_run_id, was_executed, is_exception, result, is_undefined);
    frame->SendProcessMessage(PID_BROWSER, response);
    return true;
  }

  return false;
}
