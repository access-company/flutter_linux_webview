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

#ifndef LINUX_FLUTTER_WEBVIEW_CONTROLLER_H_
#define LINUX_FLUTTER_WEBVIEW_CONTROLLER_H_

#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "flutter_linux_webview/flutter_webview_types.h"
#include "flutter_webview_handler.h"
#include "include/cef_render_handler.h"

// Provides the API to control a WebView. Unless otherwise indicated in the
// comments, the methods of this class must be called on the CEF UI and the
// callbacks are called on that thread.
class FlutterWebviewController {
 public:
  // Represents a callback that is called with null |error| when the execution
  // of a method completes or with |error| when an error occurs.
  using DoneCBVoid = std::function<void(Nullable<WebviewError> error)>;

  // Represents a callback that is either called with |result| and null |error|
  // when the method execution completes, or with |error| when an error occurs.
  template <typename T>
  using DoneCB = std::function<void(Nullable<WebviewError> error, T result)>;

  // Initializes CEF and starts the CEF message loop on a dedicated thread.
  // This method must be called on the platform plugin thread.
  // |done_cb| is called back asynchronously on the CEF UI thread when the CEF
  // message loop has started. In case of failure to start CEF, this method
  // returns a |WebviewError| and |done_cb| is never called.
  static Nullable<WebviewError> StartCef(
      const std::vector<std::string>& command_line_args,
      const DoneCBVoid& done_cb);

  // Closes all browsers and shuts down CEF. This method blocks until CEF is
  // shut down. This method must be called on the platform plugin
  // thread, which is the same thread on which StartCef was called.
  //
  // Note: CEF cannnot be restarted once shutdown due to the limitation of CEF.
  static Nullable<WebviewError> ShutdownCef();

  // Create a new browser with the given |webview_id| and |params|. |done_cb| is
  // called back either when the browser is created and starts loading
  // |params.url|, or an error occurs. If |params.url| is empty the browser
  // loads "about:blank" instead.
  static void CreateBrowser(WebviewId webview_id,
                            const WebviewCreationParams& params,
                            const DoneCBVoid& done_cb);

  // Close the browser with |webview_id|. |done_cb| is called back just before
  // the browser closes or when an error occurs.
  static void CloseBrowser(WebviewId webview_id, const DoneCBVoid& done_cb);

  // Sends a mouse move event to the browser specified by |webview_id|.
  static void SendMouseMove(WebviewId webview_id,
                            int x,
                            int y,
                            uint32 modifiers,
                            bool mouseLeave,
                            const DoneCBVoid& done_cb);

  // Sends a mouse whell event to the the browser specified by |webview_id|.
  static void SendMouseWheel(WebviewId webview_id,
                             int x,
                             int y,
                             uint32 modifiers,
                             int deltaX,
                             int deltaY,
                             const DoneCBVoid& done_cb);

  // Sends a mouse click event to the browser specified by |webview_id|.
  static void SendMouseClick(WebviewId webview_id,
                             int x,
                             int y,
                             uint32 modifiers,
                             int mouseButtonType,
                             bool mouseUp,
                             int clickCount,
                             const DoneCBVoid& done_cb);

  // Sends a key event to the browser specified by |webview_id|.
  static void SendKey(WebviewId webview_id,
                      int keyEventType,
                      uint32 modifiers,
                      int windowsKeyCode,
                      int nativeKeyCode,
                      bool isSystemKey,
                      char16 character,
                      char16 unmodifiedCharacter,
                      const DoneCBVoid& done_cb);

  // Sets the rendering resolution of the browser with |webview_id| to |width|
  // and |height|. |width| and |height| must be greater than 0.
  static void Resize(WebviewId webview_id,
                     int width,
                     int height,
                     const DoneCBVoid& done_cb);

  // Loads the specified url on the main frame of the browser with |webview_id|.
  static void LoadUrl(WebviewId webview_id,
                      const std::string& url,
                      const DoneCBVoid& done_cb);

  // Loads the specified request.
  //
  // WARNING: The underlying method CefFrame::LoadRequest will fail with "bad
  // IPC message" reason INVALID_INITIATOR_ORIGIN (213) unless you first
  // navigate to the request origin using some other mechanism (LoadURL, link
  // click, etc).
  static void LoadRequest(
      WebviewId webview_id,
      const std::string& uri,
      const std::string& method,
      const std::multimap<std::string, std::string>& headers,
      const std::vector<uint8_t>& body,
      const DoneCBVoid& done_cb);

  // Get the URL currently loaded in the main frame of the browser specified by
  // |webview_id|. The URL is given as |result| in the callback
  // |current_url_cb|.
  static void CurrentUrl(WebviewId webview_id,
                         const DoneCB<const std::string&>& current_url_cb);

  // Get if the brwoser specified by |webview_id| can navigate backwards. The
  // result is given as |result| in the callback |can_go_back_cb|.
  static void CanGoBack(WebviewId webview_id,
                        const DoneCB<bool>& can_go_back_cb);

  // Get if the browser specified by |webview_id| can navigate forwards. The
  // result is given as |result| in the callback |can_go_forward_cb|.
  static void CanGoForward(WebviewId webview_id,
                           const DoneCB<bool>& can_go_forward_cb);

  // Navigates the browser specified by |webview_id| backwards.
  static void GoBack(WebviewId webview_id, const DoneCBVoid& done_cb);

  // Navigates the browser specified by |webview_id| forwards.
  static void GoForward(WebviewId webview_id, const DoneCBVoid& done_cb);

  // Reloads the current page of the browser specified by |webview_id|.
  static void Reload(WebviewId webview_id, const DoneCBVoid& done_cb);

  // Get the current title set by the page of the browser specified by
  // |webview_id|. The title is given as |result| in the callback
  // |get_title_cb|.
  static void GetTitle(WebviewId webview_id,
                       const DoneCB<const std::string&>& get_title_cb);

  // Sends a 'FrameMsg_RequestRunJavascript' message containing the JS to be
  // executed to the renderer process. The JS is executed on the renderer
  // process, and the evaluation result is sent back to the browser process
  // handler as a 'FrameHostMsg_RunJavascriptResponse' message. That handler
  // calls the JavascriptResultCallback given in WebviewCreationParams.
  static void RequestRunJavascript(WebviewId webview_id,
                                   int js_run_id,
                                   const std::string& javascript,
                                   const DoneCBVoid& done_cb);

  // Set a cookie for all WebView instances.
  static void SetCookie(const std::string& domain,
                        const std::string& path,
                        const std::string& name,
                        const std::string& value,
                        const DoneCBVoid& done_cb);

  // Clear all cookies for all WebView instances. Returns in the |result| of the
  // callback |clear_cookies_cb| whether cookies were present before cleaning.
  static void ClearCookies(const DoneCB<bool>& clear_cookies_cb);

 private:
  enum class CefState {
    // The initial state
    kUninitialized = 0,
    // Waiting for the CEF main thread to start and
    // CefBrowserProcessHandler::OnContextInitialized to be called.
    kInitializing,
    // The state in which CefBrowserProcessHandler::OnContextInitialized is
    // called.
    kInitialized,
    // The state in which ShutdownCef() is called and waiting for all browsers
    // to close.
    kShuttingDown,
    // The state in which CefShutdown() has completed.
    kShutdown
  };

  // Map of existing browser windows. Only accessed on the CEF UI thread.
  using BrowserMap = std::unordered_map<WebviewId, CefRefPtr<CefBrowser>>;

  // The callback called when CefBrowserProcessHandler::OnContextInitialized is
  // called. It means the completion of CEF initialization.
  static void OnContextInitialized();

  // Entry function of the CEF message loop thread, started by StartCef.
  // |command_line_args| are passed to CefInitialize.
  static void CefThreadMain(std::vector<std::string> command_line_args);

  // The callback called when CefLifespanHandler::OnAfterCreated is called.
  // Holds the reference to the |browser| in the browser list.
  static void OnAfterCreated(WebviewId webview_id,
                             CefRefPtr<CefBrowser> browser);

  // Performs shutdown sequence; called on the CEF UI thread.
  static void CloseAllBrowsersAndQuitMessageLoop();

  // The callback called when CefLifespanHandler::OnBeforeClose is called.
  // Removes the reference to the |browser| from the browser list. If all
  // browsers are closed when in ShuttingDown state, terminates CEF.
  static void OnBeforeClose(WebviewId webview_id,
                            CefRefPtr<CefBrowser> browser);

  static CefRefPtr<CefBrowser> GetBrowserByWebviewId(WebviewId webview_id);
  static std::string GetCefStateName(CefState state);

  // Members to be accessed only on the platform plugin thread
  static bool is_start_cef_done_;
  static bool is_shutdown_cef_done_;
  static std::thread cef_thread_;

  // Members to be accessed only on the CEF UI thread
  static CefState cef_state_;
  static DoneCBVoid start_cef_cb_;
  static BrowserMap browser_map_;
};

#endif  // LINUX_FLUTTER_WEBVIEW_CONTROLLER_H_
