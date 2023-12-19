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

// This file is based on https://bitbucket.org/chromiumembedded/cef/src/4664/tests/cefclient/browser/osr_renderer.h
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

#ifndef LINUX_FLUTTER_WEBVIEW_HANDLER_H_
#define LINUX_FLUTTER_WEBVIEW_HANDLER_H_

#include <GL/gl.h>

#include <functional>
#include <set>

#include "flutter_linux_webview/flutter_webview_types.h"
#include "include/cef_client.h"

class FlutterWebviewHandler : public CefClient,
                              public CefDisplayHandler,
                              public CefLifeSpanHandler,
                              public CefLoadHandler,
                              public CefRenderHandler {
 public:
  FlutterWebviewHandler(
      WebviewId webview_id,
      const WebviewCreationParams& params,
      const std::function<void(WebviewId webview_id,
                               CefRefPtr<CefBrowser> browser)>&
          on_after_created,
      const std::function<void()>& on_browser_ready,
      const std::function<void(WebviewId webview_id,
                               CefRefPtr<CefBrowser> browser)>&
          on_before_close);

  // CefClient methods:
  virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() override {
    return this;
  }
  virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override {
    return this;
  }
  virtual CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
  virtual CefRefPtr<CefRenderHandler> GetRenderHandler() override {
    return this;
  }
  virtual bool OnProcessMessageReceived(
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      CefProcessId source_process,
      CefRefPtr<CefProcessMessage> message) override;

  // CefDisplayHandler methods:
  virtual void OnLoadingProgressChange(CefRefPtr<CefBrowser> browser,
                                       double progress) override;

  // CefLifeSpanHandler methods:
  virtual bool OnBeforePopup(CefRefPtr<CefBrowser> browser,
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
                             bool* no_javascript_access) override;
  virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
  virtual bool DoClose(CefRefPtr<CefBrowser> browser) override;
  virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

  // CefLoadHandler methods:
  virtual void OnLoadStart(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> frame,
                           TransitionType transition_type) override;
  virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         int httpStatusCode) override;
  virtual void OnLoadError(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> frame,
                           ErrorCode errorCode,
                           const CefString& errorText,
                           const CefString& failedUrl) override;

  // CefRenderHandler methods:
  virtual void GetViewRect(CefRefPtr<CefBrowser> browser,
                           CefRect& rect) override;
  virtual void OnPaint(CefRefPtr<CefBrowser> browser,
                       PaintElementType type,
                       const RectList& dirtyRects,
                       const void* buffer,
                       int width,
                       int height) override;
  virtual void OnPopupShow(CefRefPtr<CefBrowser> browser, bool show) override;
  virtual void OnPopupSize(CefRefPtr<CefBrowser> browser,
                           const CefRect& rect) override;

  // Close this browser asynchronously. |close_browser_cb| is called just
  // before a browser is destroyed.
  void CloseBrowser(const std::function<void()>& close_browser_cb);

  // Set the OSR resolution
  void SetViewRect(int width, int height);

 private:
  enum class BrowserState {
    kBeforeCreated,
    kCreated,
    // Currently, "ready" means when main_frame->GetURL() returns a meaningful
    // value.
    kReady,
    kClosing,
    kClosed
  };

  void ClearPopupRects();
  CefRect GetPopupRectInWebView(const CefRect& original_rect);

  std::function<void(WebviewId webview_id)> on_paint_begin_;
  std::function<void(WebviewId webview_id)> on_paint_end_;

  // callbacks for methods of the WebViewPlatformCallbacksHandler class
  WebviewCreationParams::PageStartedCallback on_page_started_;
  WebviewCreationParams::PageFinishedCallback on_page_finished_;
  WebviewCreationParams::PageLoadingCallback on_progress_;
  WebviewCreationParams::WebResourceErrorCallback on_web_resource_error_;

  WebviewCreationParams::JavascriptResultCallback on_javascript_result_;
  std::function<void(WebviewId webview_id, CefRefPtr<CefBrowser> browser)>
      on_after_created_;
  std::function<void()> on_browser_ready_;
  std::function<void(WebviewId webview_id, CefRefPtr<CefBrowser> browser)>
      on_before_close_;
  std::function<void()> close_browser_cb_;

  WebviewId webview_id_;
  BrowserState browser_state_;
  CefRefPtr<CefBrowser> browser_;
  GLuint native_texture_id_;
  int view_width_;
  int view_height_;
  CefRect popup_rect_;
  CefRect original_popup_rect_;

  // Include the default reference counting implementation.
  IMPLEMENT_REFCOUNTING(FlutterWebviewHandler);
};

#endif  // LINUX_FLUTTER_WEBVIEW_HANDLER_H_
