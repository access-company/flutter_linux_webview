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

#ifndef LINUX_INCLUDE_FLUTTER_LINUX_WEBVIEW_FLUTTER_WEBVIEW_TYPES_H_
#define LINUX_INCLUDE_FLUTTER_LINUX_WEBVIEW_FLUTTER_WEBVIEW_TYPES_H_

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <utility>

using WebviewId = int64_t;

struct WebviewError {
 public:
  static constexpr char kInvalidWebviewId[] = "Invalid Webview ID";
  static constexpr char kInvalidWebviewIdErrorMessage[] =
      "The browser specified by the webview id is not found.";
  static constexpr char kRuntimeError[] = "Runtime Error";
  static constexpr char kBadArgumentsError[] = "Bad Arguments";

  WebviewError(const std::string& code, const std::string& message);
  WebviewError();

  std::string code;
  std::string message;
};

template <typename T>
class Nullable {
 public:
  Nullable(const Nullable& other) = default;
  Nullable(Nullable&& other) = default;
  Nullable& operator=(const Nullable& other) {
    has_value_ = other.has_value_;
    value_ = other.value_;
    return *this;
  }
  Nullable& operator=(Nullable&& other) {
    has_value_ = other.has_value_;
    value_ = std::move(other.value_);
    return *this;
  }
  ~Nullable() = default;

  explicit Nullable(const T& value) : has_value_(true), value_(value) {}
  explicit Nullable(T&& value) : has_value_(true), value_(std::move(value)) {}
  Nullable() : has_value_(false), value_() {}

  T& value() { return value_; }
  const T& value() const { return value_; }
  bool is_null() const { return !has_value_; }

 private:
  bool has_value_;
  T value_;
};

struct WebviewCreationParams {
  using PageStartedCallback =
      std::function<void(WebviewId webview_id, const std::string& url)>;
  using PageFinishedCallback =
      std::function<void(WebviewId webview_id, const std::string& url)>;
  using PageLoadingCallback =
      std::function<void(WebviewId webview_id, int progress)>;

  using WebResourceErrorCallback =
      std::function<void(WebviewId webview_id,
                         int errorCode,
                         const std::string& description,
                         const std::string& failingUrl)>;

  using JavascriptResultCallback = std::function<void(WebviewId webview_id,
                                                      int js_run_id,
                                                      bool was_executed,
                                                      bool is_exception,
                                                      const std::string& result,
                                                      bool is_undefined)>;

  WebviewCreationParams(
      uint32_t native_texture_id,
      int width,
      int height,
      std::function<void(WebviewId webview_id)> on_paint_begin,
      std::function<void(WebviewId webview_id)> on_paint_end,
      std::string url,
      std::vector<uint8_t> background_color,
      PageStartedCallback on_page_started,
      PageFinishedCallback on_page_finished,
      PageLoadingCallback on_progress,
      WebResourceErrorCallback on_web_resource_error,
      JavascriptResultCallback on_javascript_result)
      : native_texture_id(native_texture_id),
        width(width),
        height(height),
        on_paint_begin(on_paint_begin),
        on_paint_end(on_paint_end),
        url(url),
        background_color(background_color),
        on_page_started(on_page_started),
        on_page_finished(on_page_finished),
        on_progress(on_progress),
        on_web_resource_error(on_web_resource_error),
        on_javascript_result(on_javascript_result) {}

  // ID of the OpenGL texture to which the browser rendering will be drawn.
  uint32_t native_texture_id;

  // initial width of the browser
  int width;

  // initial height of the browser
  int height;

  // Callback called at the beginning of CefRenderHandler::OnPaint, called on
  // the CEF UI thread.
  // Bind the context and surface here before GL drawing in OnPaint.
  std::function<void(WebviewId webview_id)> on_paint_begin;

  // Callback called at the end of CefRenderHandler::OnPaint, called on the CEF
  // UI thread.
  std::function<void(WebviewId webview_id)> on_paint_end;

  // Representation of the WebView.initialUrl property. If empty, about:blank is
  // loaded.
  std::string url;

  // Representation of the WebView.backgroundColor property. Represented by a
  // vector in the format {A, R, G, B}
  std::vector<uint8_t> background_color;

  // Representation of the WebView.onPageStarted callback, called on the CEF UI
  // thread. Called after a navigation has been committed and before the browser
  // begins loading contents in the main frame.
  PageStartedCallback on_page_started;

  // Representation of the WebView.onPageFinished callback, called on the CEF UI
  // thread. Called when the browser is done loading the main frame.
  PageFinishedCallback on_page_finished;

  // Representation of the WebView.onProgress callback, called on the CEF UI
  // thread. Called when the overall page loading progress has changed. The
  // progress ranges from 0 to 100.
  PageLoadingCallback on_progress;

  // Representation of the WebView.onWebResourceError callback, called on the
  // CEF UI thread. Called when a navigation fails or is canceled.
  WebResourceErrorCallback on_web_resource_error;

  // Callback called when the result of runJavascript is returned, called on the
  // CEF UI thread.
  JavascriptResultCallback on_javascript_result;
};

#endif  // LINUX_INCLUDE_FLUTTER_LINUX_WEBVIEW_FLUTTER_WEBVIEW_TYPES_H_
