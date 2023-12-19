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

// This file is based on https://bitbucket.org/chromiumembedded/cef/src/4664/tests/cefclient/browser/osr_renderer.cc
// for off-screen rendering.

// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
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

#include "flutter_webview_handler.h"

#include <GL/gl.h>

#include <iostream>
#include <sstream>
#include <string>

#include "flutter_linux_webview/flutter_webview_types.h"
#include "include/base/cef_callback.h"
#include "include/base/cef_logging.h"
#include "include/cef_app.h"
#include "include/cef_parser.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"
#include "subprocess/src/flutter_webview_process_messages.h"

// DCHECK on gl errors.
#if DCHECK_IS_ON()
#define VERIFY_NO_ERROR                                                      \
  {                                                                          \
    int _gl_error = glGetError();                                            \
    DCHECK(_gl_error == GL_NO_ERROR) << "glGetError returned " << _gl_error; \
  }
#else
#define VERIFY_NO_ERROR
#endif  // DCHECK_IS_ON()

FlutterWebviewHandler::FlutterWebviewHandler(
    WebviewId webview_id,
    const WebviewCreationParams& params,
    const std::function<void(WebviewId webview_id,
                             CefRefPtr<CefBrowser> browser)>& on_after_created,
    const std::function<void()>& on_browser_ready,
    const std::function<void(WebviewId webview_id,
                             CefRefPtr<CefBrowser> browser)>& on_before_close)
    : on_paint_begin_(params.on_paint_begin),
      on_paint_end_(params.on_paint_end),
      on_page_started_(params.on_page_started),
      on_page_finished_(params.on_page_finished),
      on_progress_(params.on_progress),
      on_web_resource_error_(params.on_web_resource_error),
      on_javascript_result_(params.on_javascript_result),
      on_after_created_(on_after_created),
      on_browser_ready_(on_browser_ready),
      on_before_close_(on_before_close),
      close_browser_cb_(nullptr),
      webview_id_(webview_id),
      browser_state_(BrowserState::kBeforeCreated),
      browser_(nullptr),
      native_texture_id_(params.native_texture_id),
      view_width_(params.width),
      view_height_(params.height) {}

bool FlutterWebviewHandler::OnBeforePopup(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    const CefString& target_url,
    const CefString& target_frame_name,
    WindowOpenDisposition target_disposition,
    bool user_gesture,
    const CefPopupFeatures& popupFeatures,
    CefWindowInfo& windowInfo,
    CefRefPtr<CefClient>& client,
    CefBrowserSettings& settings,
    CefRefPtr<CefDictionaryValue>& extra_info,
    bool* no_javascript_access) {
  CEF_REQUIRE_UI_THREAD();

  VLOG(1) << __func__ << ": Loading popup in the main window: target_url="
          << target_url.ToString().c_str();
  if (!target_url.empty()) {
    browser->GetMainFrame()->LoadURL(target_url);
  }

  // Block popup
  return true;
}

void FlutterWebviewHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();

  browser_ = browser;
  browser_state_ = BrowserState::kCreated;

  on_after_created_(webview_id_, browser);
}

void FlutterWebviewHandler::CloseBrowser(
    const std::function<void()>& close_browser_cb) {
  CEF_REQUIRE_UI_THREAD();
  VLOG(1) << __func__ << ": browser_=" << browser_;

  close_browser_cb_ = close_browser_cb;
  browser_state_ = BrowserState::kClosing;
  browser_->GetHost()->CloseBrowser(/* force_close= */ false);
  // Then, DoClose (overrides CefLifeSpanHandler::DoClose) will be called
}

bool FlutterWebviewHandler::DoClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  VLOG(1) << __func__;

  if (browser_state_ != BrowserState::kClosing) {
    LOG(WARNING) << __func__
                 << ": Closing a browser by any way other than calling "
                    "CloseBrowser is not allowed.";
    // Deny window.close();
    return true;
  }

  // Allow the close. For windowed browsers this will result in the OS close
  // event being sent.
  return false;
}

void FlutterWebviewHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  VLOG(1) << __func__;

  browser_ = nullptr;
  on_before_close_(webview_id_, browser);

  browser_state_ = BrowserState::kClosed;

  if (close_browser_cb_) {
    close_browser_cb_();
  }
}

bool FlutterWebviewHandler::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message) {
  CEF_REQUIRE_UI_THREAD();
  VLOG(1) << __func__ << ": webview_id_=" << webview_id_
          << ": Message received from the renderer!!: " << message->GetName();

  // Check the message name.
  const std::string& message_name = message->GetName();
  if (message_name ==
      flutter_webview_process_messages::kFrameHostMsg_RunJavascriptResponse) {
    int js_run_id;
    bool was_executed;
    bool is_exception;
    std::string js_result;
    bool is_undefined;
    flutter_webview_process_messages::Read_FrameHostMsg_RunJavascriptResponse(
        message, &js_run_id, &was_executed, &is_exception, &js_result,
        &is_undefined);

    VLOG(1) << "The browser process received js result";
    on_javascript_result_(webview_id_, js_run_id, was_executed, is_exception,
                          js_result, is_undefined);
    return true;
  }
  return false;
}

void FlutterWebviewHandler::SetViewRect(int width, int height) {
  CEF_REQUIRE_UI_THREAD();

  if (width <= 0 || height <= 0) {
    LOG(ERROR) << __func__ << ": width and height must be greater than 0.";
    return;
  }

  view_width_ = width;
  view_height_ = height;
}

void FlutterWebviewHandler::OnLoadingProgressChange(
    CefRefPtr<CefBrowser> browser,
    double progress) {
  CEF_REQUIRE_UI_THREAD();
  VLOG(1) << __func__ << ": progress=" << progress;

  // progress ranges from 0.0 to 1.0
  on_progress_(webview_id_, static_cast<int>(progress * 100));
}

void FlutterWebviewHandler::OnLoadStart(CefRefPtr<CefBrowser> browser,
                                        CefRefPtr<CefFrame> frame,
                                        TransitionType transition_type) {
  CEF_REQUIRE_UI_THREAD();
  VLOG(1) << __func__ << ": frame->IsMain()=" << frame->IsMain()
          << ", frame->GetURL()=" << frame->GetURL();

  if (frame->IsMain()) {
    if (browser_state_ == BrowserState::kCreated) {
      browser_state_ = BrowserState::kReady;
      on_browser_ready_();
    }
    on_page_started_(webview_id_, frame->GetURL().ToString());
  }
}

void FlutterWebviewHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame,
                                      int httpStatusCode) {
  CEF_REQUIRE_UI_THREAD();
  VLOG(1) << __func__ << ": frame->IsMain()=" << frame->IsMain()
          << ", frame->GetURL()=" << frame->GetURL()
          << ", httpStatusCode=" << httpStatusCode;

  if (frame->IsMain()) {
    on_page_finished_(webview_id_, frame->GetURL().ToString());
  }
}

void FlutterWebviewHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                                        CefRefPtr<CefFrame> frame,
                                        ErrorCode errorCode,
                                        const CefString& errorText,
                                        const CefString& failedUrl) {
  CEF_REQUIRE_UI_THREAD();
  VLOG(1) << __func__ << ": frame->IsMain()=" << frame->IsMain()
          << ", frame->GetURL()=" << frame->GetURL()
          << ", errorCode=" << errorCode << ", errorText=" << errorText
          << ", failedUrl=" << failedUrl;

  if (browser_state_ == BrowserState::kCreated) {
    browser_state_ = BrowserState::kReady;
    on_browser_ready_();
  }

  if (frame->IsMain()) {
    on_web_resource_error_(webview_id_, errorCode, errorText.ToString(),
                           failedUrl.ToString());
  }
}

void FlutterWebviewHandler::GetViewRect(CefRefPtr<CefBrowser> browser,
                                        CefRect& rect) {
  CEF_REQUIRE_UI_THREAD();

  rect.width = view_width_;
  rect.height = view_height_;
}

void FlutterWebviewHandler::OnPaint(CefRefPtr<CefBrowser> browser,
                                    PaintElementType type,
                                    const RectList& dirtyRects,
                                    const void* buffer,
                                    int width,
                                    int height) {
  CEF_REQUIRE_UI_THREAD();
  // Logics copied from cefclient/browser/osr_renderer.cc

  on_paint_begin_(webview_id_);

  DCHECK_NE(native_texture_id_, 0U);
  glBindTexture(GL_TEXTURE_2D, native_texture_id_);
  VERIFY_NO_ERROR;

  if (type == PET_VIEW) {
    int old_width = view_width_;
    int old_height = view_height_;

    // TODO(Ino): dispatch resizing?
    view_width_ = width;
    view_height_ = height;

    glPixelStorei(GL_UNPACK_ROW_LENGTH, view_width_);
    VERIFY_NO_ERROR;

    if (old_width != view_width_ || old_height != view_height_ ||
        (dirtyRects.size() == 1 &&
         dirtyRects[0] == CefRect(0, 0, view_width_, view_height_))) {
      // Update/resize the whole texture.
      glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
      VERIFY_NO_ERROR;
      glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
      VERIFY_NO_ERROR;
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, view_width_, view_height_, 0,
                   GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer);
      VERIFY_NO_ERROR;
    } else {
      // Update just the dirty rectangles.
      CefRenderHandler::RectList::const_iterator i = dirtyRects.begin();
      for (; i != dirtyRects.end(); ++i) {
        const CefRect& rect = *i;
        DCHECK(rect.x + rect.width <= view_width_);
        DCHECK(rect.y + rect.height <= view_height_);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, rect.x);
        VERIFY_NO_ERROR;
        glPixelStorei(GL_UNPACK_SKIP_ROWS, rect.y);
        glTexSubImage2D(GL_TEXTURE_2D, 0, rect.x, rect.y, rect.width,
                        rect.height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                        buffer);
        VERIFY_NO_ERROR;
      }
    }
  } else if (type == PET_POPUP && popup_rect_.width > 0 &&
             popup_rect_.height > 0) {
    int skip_pixels = 0, x = popup_rect_.x;
    int skip_rows = 0, y = popup_rect_.y;
    int w = width;
    int h = height;

    // Adjust the popup to fit inside the view.
    if (x < 0) {
      skip_pixels = -x;
      x = 0;
    }
    if (y < 0) {
      skip_rows = -y;
      y = 0;
    }
    if (x + w > view_width_)
      w -= x + w - view_width_;
    if (y + h > view_height_)
      h -= y + h - view_height_;

    // Update the popup rectangle.
    glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
    VERIFY_NO_ERROR;
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, skip_pixels);
    VERIFY_NO_ERROR;
    glPixelStorei(GL_UNPACK_SKIP_ROWS, skip_rows);
    VERIFY_NO_ERROR;
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_BGRA,
                    GL_UNSIGNED_INT_8_8_8_8_REV, buffer);
    VERIFY_NO_ERROR;
  }

  if (type == PET_VIEW && !popup_rect_.IsEmpty()) {
    browser->GetHost()->Invalidate(PET_POPUP);
  }

  on_paint_end_(webview_id_);
}

void FlutterWebviewHandler::OnPopupShow(CefRefPtr<CefBrowser> browser,
                                        bool show) {
  CEF_REQUIRE_UI_THREAD();

  if (!show) {
    // Clear the popup rectangle.
    ClearPopupRects();
  }
}

void FlutterWebviewHandler::OnPopupSize(CefRefPtr<CefBrowser> browser,
                                        const CefRect& rect) {
  CEF_REQUIRE_UI_THREAD();

  if (rect.width <= 0 || rect.height <= 0)
    return;
  original_popup_rect_ = rect;
  popup_rect_ = GetPopupRectInWebView(original_popup_rect_);
}

CefRect FlutterWebviewHandler::GetPopupRectInWebView(
    const CefRect& original_rect) {
  CefRect rc(original_rect);
  // if x or y are negative, move them to 0.
  if (rc.x < 0)
    rc.x = 0;
  if (rc.y < 0)
    rc.y = 0;
  // if popup goes outside the view, try to reposition origin
  if (rc.x + rc.width > view_width_)
    rc.x = view_width_ - rc.width;
  if (rc.y + rc.height > view_height_)
    rc.y = view_height_ - rc.height;
  // if x or y became negative, move them to 0 again.
  if (rc.x < 0)
    rc.x = 0;
  if (rc.y < 0)
    rc.y = 0;
  return rc;
}

void FlutterWebviewHandler::ClearPopupRects() {
  popup_rect_.Set(0, 0, 0, 0);
  original_popup_rect_.Set(0, 0, 0, 0);
}
