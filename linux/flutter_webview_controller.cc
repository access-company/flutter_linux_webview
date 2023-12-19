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

#include "flutter_webview_controller.h"

#include <libgen.h>        // dirname
#include <linux/limits.h>  // PATH_MAX
#include <unistd.h>        // readlink

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "flutter_linux_webview/flutter_webview_types.h"
#include "flutter_webview_app.h"
#include "flutter_webview_handler.h"
#include "include/base/cef_callback.h"
#include "include/base/cef_logging.h"
#include "include/cef_app.h"
#include "include/cef_cookie.h"
#include "include/cef_parser.h"
#include "include/internal/cef_types.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"
#include "subprocess/src/flutter_webview_process_messages.h"

constexpr char WebviewError::kInvalidWebviewId[];
constexpr char WebviewError::kInvalidWebviewIdErrorMessage[];
constexpr char WebviewError::kRuntimeError[];
constexpr char WebviewError::kBadArgumentsError[];

// static private members accessed on the platform plugin thread
bool FlutterWebviewController::is_start_cef_done_ = false;
bool FlutterWebviewController::is_shutdown_cef_done_ = false;
std::thread FlutterWebviewController::cef_thread_;

// static private members accessed on the CEF UI thread
FlutterWebviewController::CefState FlutterWebviewController::cef_state_ =
    CefState::kUninitialized;
FlutterWebviewController::DoneCBVoid FlutterWebviewController::start_cef_cb_;
FlutterWebviewController::BrowserMap FlutterWebviewController::browser_map_;


// static
Nullable<WebviewError> FlutterWebviewController::StartCef(
    const std::vector<std::string>& command_line_args,
    const DoneCBVoid& done_cb) {
  // Called on the platform plugin thread

  if (is_start_cef_done_) {
    std::string error_message{
        "FlutterWebviewController::StartCef: Warning: Should not be called "
        "more than once."};
    std::cerr << error_message << std::endl;
    return Nullable<WebviewError>(
        WebviewError{WebviewError::kRuntimeError, std::move(error_message)});
  }

  // At this point, cef_thread_ has not created yet, so cef_state_ can be
  // accessed.
  assert(cef_state_ == CefState::kUninitialized);

  is_start_cef_done_ = true;
  start_cef_cb_ = done_cb;
  cef_state_ = CefState::kInitializing;
#if FLUTTER_WEBVIEW_DEBUG
  std::cerr << __func__ << ": cef_state_ has changed to "
            << GetCefStateName(cef_state_) << std::endl;
#endif  // FLUTTER_WEBVIEW_DEBUG
  // start CEF thread
  cef_thread_ = std::thread(&CefThreadMain, command_line_args);
  return Nullable<WebviewError>();
}

// static
void FlutterWebviewController::CefThreadMain(
    std::vector<std::string> command_line_args) {
  // Get the executable name
  std::string exe_name;
  {
    char* path = (char*)std::malloc(PATH_MAX + 1);
    if (path == NULL) {
      std::cerr << "Could not allocate memory" << std::endl;
      return;
    }
    ssize_t count = ::readlink("/proc/self/exe", path, PATH_MAX + 1);
    if (count == -1) {
      std::cerr << "Error: Failed to readlink /proc/self/exe" << std::endl;
      std::free(path);
      return;
    }
    exe_name = std::string(basename(path));
    std::free(path);
  }

  // Emulate argv and provide them as command-line arguments to CEF.
  std::vector<char*> argv;
  std::string argv0 = std::string("./" + exe_name);
  argv.push_back(const_cast<char*>(argv0.c_str()));
  for (const auto& arg : command_line_args) {
    argv.push_back(const_cast<char*>(arg.data()));
  }
  argv.push_back(nullptr);

#if FLUTTER_WEBVIEW_DEBUG
  std::cerr << "CEF command-line arguments:" << std::endl;
  for (const std::string& arg : command_line_args) {
    std::cerr << arg << std::endl;
  }
  std::cerr << std::endl;
#endif  // FLUTTER_WEBVIEW_DEBUG

  CefMainArgs main_args(argv.size() - 1, argv.data());

  // Specify CEF global settings here.
  CefSettings settings;
  settings.windowless_rendering_enabled = true;

  CefRefPtr<FlutterWebviewApp> app(
      new FlutterWebviewApp(&FlutterWebviewController::OnContextInitialized));

#if FLUTTER_WEBVIEW_DEBUG
  std::cerr << __func__ << ": Entering CefInitialize()..." << std::endl;
#endif  // FLUTTER_WEBVIEW_DEBUG

  // Initialize CEF for the browser process.
  CefInitialize(main_args, settings, app.get(), nullptr);

#if FLUTTER_WEBVIEW_DEBUG
  std::cerr << __func__
            << ": Exited CefInitialize(), and CefRunMessageLoop() starts."
            << std::endl;
#endif  // FLUTTER_WEBVIEW_DEBUG

  // Run the CEF message loop. This will block until CefQuitMessageLoop() is
  // called.
  CefRunMessageLoop();

  // Shut down CEF.
  CefShutdown();

  cef_state_ = CefState::kShutdown;
#if FLUTTER_WEBVIEW_DEBUG
  std::cerr << __func__ << ": cef_state_ has changed to "
            << GetCefStateName(cef_state_) << std::endl;
#endif  // FLUTTER_WEBVIEW_DEBUG
}

// static
void FlutterWebviewController::CloseAllBrowsersAndQuitMessageLoop() {
  if (cef_state_ != CefState::kInitialized) {
    LOG(WARNING) << "ShutdownCef() must be called in the Initialized state "
                    "but called in "
                 << GetCefStateName(cef_state_);
    return;
  }

  cef_state_ = CefState::kShuttingDown;
  VLOG(1) << __func__ << ": cef_state_ has changed to "
          << GetCefStateName(cef_state_);

  if (browser_map_.empty()) {
    CefQuitMessageLoop();
    return;
  }

  // Get the pointers to the handlers to call handler->CloseBrowser(),
  // since browser_map_ changes during the CloseBrowser iteration.
  std::vector<FlutterWebviewHandler*> handlers(browser_map_.size());
  std::transform(browser_map_.begin(), browser_map_.end(), handlers.begin(),
                 [](std::pair<const WebviewId, CefRefPtr<CefBrowser>>& p) {
                   return static_cast<FlutterWebviewHandler*>(
                       p.second->GetHost()->GetClient().get());
                 });

  for (auto h_it = handlers.begin(); h_it != handlers.end(); ++h_it) {
    (*h_it)->CloseBrowser(/* close_browser_cb= */ nullptr);
  }
  // FlutterWebviewHandler::OnBeforeClose (overrides
  // CefLifeSpanHandler::OnBeforeClose) calls OnBeforeClose() of this class.
  // OnBeforeClose() then calls CefQuitMessageLoop() for the last browser.
}

// static
Nullable<WebviewError> FlutterWebviewController::ShutdownCef() {
  // Called on the platform plugin thread

  if (!is_start_cef_done_) {
    std::string error_message{
        "FlutterWebviewController::ShutdownCef: Error: Must be called after "
        "StartCef() is called."};
    std::cerr << error_message << std::endl;
    return Nullable<WebviewError>(
        WebviewError{WebviewError::kRuntimeError, error_message});
  }

  if (is_shutdown_cef_done_) {
    std::cerr << "FlutterWebviewController::ShutdownCef: Warning: CEF has "
                 "already been shut down."
              << std::endl;
    // No need to make a Dart exception for multiple calls.
    return Nullable<WebviewError>();
  }

  if (!CefPostTask(TID_UI,
                   base::BindOnce(CloseAllBrowsersAndQuitMessageLoop))) {
    // CefPostTask may fail if CefInitialize() has not yet been called.
    std::string error_message{
        "FlutterWebviewController::ShutdownCef: Warning: CefPostTask() "
        "failed."};
    std::cerr << error_message << std::endl;
    return Nullable<WebviewError>(
        WebviewError{WebviewError::kRuntimeError, error_message});
  }

  // The logging functions in cef_logging.h should not be used during CEF
  // shutdown.
  std::cerr << "ShutdownCef: Waiting for CEF shutdown..." << std::endl;
  cef_thread_.join();
  is_shutdown_cef_done_ = true;
  std::cerr << "ShutdownCef: CEF shutdown." << std::endl;
  return Nullable<WebviewError>();
}

// static
CefRefPtr<CefBrowser> FlutterWebviewController::GetBrowserByWebviewId(
    WebviewId webview_id) {
  CEF_REQUIRE_UI_THREAD();

  BrowserMap::iterator it = browser_map_.find(webview_id);
  if (it == browser_map_.end()) {
    LOG(WARNING) << __func__ << ": The browser for webview_id=" << webview_id
                 << " is not found!";
    return nullptr;
  }

  return it->second;
}

// static
void FlutterWebviewController::OnContextInitialized() {
  CEF_REQUIRE_UI_THREAD();

  cef_state_ = CefState::kInitialized;
  VLOG(1) << __func__ << ": cef_state_ has changed to "
          << GetCefStateName(cef_state_);

  if (start_cef_cb_) {
    start_cef_cb_(Nullable<WebviewError>());
  } else {
    LOG(WARNING) << __func__ << ": called without calling StartCef().";
  }
}

// static
void FlutterWebviewController::OnAfterCreated(WebviewId webview_id,
                                              CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  VLOG(1) << __func__ << ": webview_id=" << webview_id
          << ", browser=" << browser;

  auto it_inserted = browser_map_.emplace(webview_id, browser);
  if (!it_inserted.second) {
    LOG(ERROR) << __func__ << ": webview_id=" << webview_id
               << " is already exists.";
  }
}

// static
void FlutterWebviewController::OnBeforeClose(WebviewId webview_id,
                                             CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  VLOG(1) << __func__ << ": webview_id=" << webview_id
          << ", browser=" << browser;

  size_t num_erased = browser_map_.erase(webview_id);
  if (num_erased == 0) {
    LOG(ERROR) << __func__ << ": webview_id=" << webview_id << " is not found.";
    return;
  }

  if (cef_state_ == CefState::kShuttingDown && browser_map_.empty()) {
    VLOG(1) << __func__ << ": all browsers closed, calls CefQuitMessageLoop().";
    // All browser windows have closed. Quit the application message loop.
    CefQuitMessageLoop();
  }
}

// static
void FlutterWebviewController::CreateBrowser(
    WebviewId webview_id,
    const WebviewCreationParams& params,
    const DoneCBVoid& done_cb) {
  CEF_REQUIRE_UI_THREAD();

  // Specify CEF browser settings here.
  CefBrowserSettings browser_settings;
  browser_settings.windowless_frame_rate = 60;

  // Specify browser_settings.background_color, if any
  if (params.background_color.size() != 0) {
    if (params.background_color.size() != 4) {
      constexpr char kErrorMessage[] =
          "FlutterWebviewController::CreateBrowser: params.background_color is "
          "specified but does not have exactly 4 values.";
      LOG(ERROR) << kErrorMessage;
      done_cb(Nullable<WebviewError>(
          WebviewError{WebviewError::kBadArgumentsError, kErrorMessage}));
      return;
    } else {
      uint8_t a = params.background_color.at(0);
      uint8_t r = params.background_color.at(1);
      uint8_t g = params.background_color.at(2);
      uint8_t b = params.background_color.at(3);
      browser_settings.background_color = CefColorSetARGB(a, r, g, b);
      std::stringstream color_hex;
      color_hex << std::hex << browser_settings.background_color;
      VLOG(1) << __func__ << ": browser_settings.background_color is set to 0x"
              << color_hex.str();
    }
  }

  constexpr char kDefaultUrl[] = "about:blank";
  const std::string initial_url =
      !params.url.empty() ? params.url : kDefaultUrl;

  // Information used when creating the native window.
  CefWindowInfo window_info;
  window_info.windowless_rendering_enabled = true;

  CefRefPtr<FlutterWebviewHandler> handler(new FlutterWebviewHandler(
      webview_id, params, &OnAfterCreated,
      [create_browser_cb = done_cb] {
        create_browser_cb(Nullable<WebviewError>());
      },
      &OnBeforeClose));

  // Create the browser window.
  CefBrowserHost::CreateBrowser(window_info, handler, initial_url,
                                browser_settings, nullptr, nullptr);
}

// static
void FlutterWebviewController::CloseBrowser(WebviewId webview_id,
                                            const DoneCBVoid& done_cb) {
  CEF_REQUIRE_UI_THREAD();

  CefRefPtr<CefBrowser> browser = GetBrowserByWebviewId(webview_id);
  if (!browser) {
    done_cb(Nullable<WebviewError>(
        WebviewError{WebviewError::kInvalidWebviewId,
                     WebviewError::kInvalidWebviewIdErrorMessage}));
    return;
  }

  FlutterWebviewHandler* handler = static_cast<FlutterWebviewHandler*>(
      browser->GetHost()->GetClient().get());
  handler->CloseBrowser([close_browser_cb = done_cb]() {
    close_browser_cb(Nullable<WebviewError>());
  });
}

// static
void FlutterWebviewController::SendMouseMove(WebviewId webview_id,
                                             int x,
                                             int y,
                                             uint32 modifiers,
                                             bool mouseLeave,
                                             const DoneCBVoid& done_cb) {
  CEF_REQUIRE_UI_THREAD();

  CefMouseEvent mouse_event;
  mouse_event.x = x;
  mouse_event.y = y;
  mouse_event.modifiers = modifiers;

  CefRefPtr<CefBrowser> browser = GetBrowserByWebviewId(webview_id);
  if (!browser) {
    done_cb(Nullable<WebviewError>(
        WebviewError{WebviewError::kInvalidWebviewId,
                     WebviewError::kInvalidWebviewIdErrorMessage}));
    return;
  }

  CefRefPtr<CefBrowserHost> host = browser->GetHost();
  host->SendMouseMoveEvent(mouse_event, mouseLeave);
  done_cb(Nullable<WebviewError>());
}

// static
void FlutterWebviewController::SendMouseWheel(WebviewId webview_id,
                                              int x,
                                              int y,
                                              uint32 modifiers,
                                              int deltaX,
                                              int deltaY,
                                              const DoneCBVoid& done_cb) {
  CEF_REQUIRE_UI_THREAD();

  CefMouseEvent mouse_event;
  mouse_event.x = x;
  mouse_event.y = y;
  mouse_event.modifiers = modifiers;

  CefRefPtr<CefBrowser> browser = GetBrowserByWebviewId(webview_id);
  if (!browser) {
    done_cb(Nullable<WebviewError>(
        WebviewError{WebviewError::kInvalidWebviewId,
                     WebviewError::kInvalidWebviewIdErrorMessage}));
    return;
  }

  CefRefPtr<CefBrowserHost> host = browser->GetHost();
  host->SendMouseWheelEvent(mouse_event, deltaX, deltaY);
  done_cb(Nullable<WebviewError>());
}

// static
void FlutterWebviewController::SendMouseClick(WebviewId webview_id,
                                              int x,
                                              int y,
                                              uint32 modifiers,
                                              int mouseButtonType,
                                              bool mouseUp,
                                              int clickCount,
                                              const DoneCBVoid& done_cb) {
  CEF_REQUIRE_UI_THREAD();

  CefBrowserHost::MouseButtonType button_type;
  if (mouseButtonType == MBT_LEFT) {
    button_type = MBT_LEFT;
  } else if (mouseButtonType == MBT_RIGHT) {
    button_type = MBT_RIGHT;
  } else if (mouseButtonType == MBT_MIDDLE) {
    button_type = MBT_MIDDLE;
  } else {
    return;
  }

  CefMouseEvent mouse_event;
  mouse_event.x = x;
  mouse_event.y = y;
  mouse_event.modifiers = modifiers;

  // NOTE: A click_count greater than 3 causes a crash.
  // [0825/194207.709969:FATAL:event.cc(601)] Check failed: 3 >= click_count (3
  // vs. 4)
  const int click_count =
      (0 < clickCount) ? (clickCount <= 3 ? clickCount : 3) : 1;

  CefRefPtr<CefBrowser> browser = GetBrowserByWebviewId(webview_id);
  if (!browser) {
    done_cb(Nullable<WebviewError>(
        WebviewError{WebviewError::kInvalidWebviewId,
                     WebviewError::kInvalidWebviewIdErrorMessage}));
    return;
  }

  CefRefPtr<CefBrowserHost> host = browser->GetHost();
  host->SendMouseClickEvent(mouse_event, button_type, mouseUp, click_count);
  done_cb(Nullable<WebviewError>());
}

// static
void FlutterWebviewController::SendKey(WebviewId webview_id,
                                       int keyEventType,
                                       uint32 modifiers,
                                       int windowsKeyCode,
                                       int nativeKeyCode,
                                       bool isSystemKey,
                                       char16 character,
                                       char16 unmodifiedCharacter,
                                       const DoneCBVoid& done_cb) {
  CEF_REQUIRE_UI_THREAD();

  CefKeyEvent key_event;
  switch (keyEventType) {
    case KEYEVENT_RAWKEYDOWN:
      key_event.type = KEYEVENT_RAWKEYDOWN;
      break;
    case KEYEVENT_KEYDOWN:
      key_event.type = KEYEVENT_KEYDOWN;
      break;
    case KEYEVENT_CHAR:
      key_event.type = KEYEVENT_CHAR;
      break;
    case KEYEVENT_KEYUP:
      key_event.type = KEYEVENT_KEYUP;
      break;
    default:
      LOG(ERROR) << __func__ << ": type must be cef_key_event_type_t";
      return;
  }
  key_event.modifiers = modifiers;
  key_event.windows_key_code = windowsKeyCode;
  key_event.native_key_code = nativeKeyCode;
  key_event.is_system_key = static_cast<int>(isSystemKey);
  key_event.character = character;
  key_event.unmodified_character = unmodifiedCharacter;

  VLOG(1) << __func__ << std::endl
          << "key_event: modifiers=" << key_event.modifiers << ", "
          << "type=" << key_event.type << ", "
          << "windows_key_code=" << key_event.windows_key_code << ", "
          << "native_key_code=" << key_event.native_key_code << ", "
          << "is_system_key=" << key_event.is_system_key << ", "
          << "character=" << key_event.character << ", "
          << "unmodified_character=" << key_event.unmodified_character;

  CefRefPtr<CefBrowser> browser = GetBrowserByWebviewId(webview_id);
  if (!browser) {
    done_cb(Nullable<WebviewError>(
        WebviewError{WebviewError::kInvalidWebviewId,
                     WebviewError::kInvalidWebviewIdErrorMessage}));
    return;
  }

  CefRefPtr<CefBrowserHost> host = browser->GetHost();
  host->SendKeyEvent(key_event);
  done_cb(Nullable<WebviewError>());
}

// static
void FlutterWebviewController::Resize(WebviewId webview_id,
                                      int width,
                                      int height,
                                      const DoneCBVoid& done_cb) {
  CEF_REQUIRE_UI_THREAD();

  if (width <= 0 || height <= 0) {
    done_cb(Nullable<WebviewError>(
        WebviewError{WebviewError::kBadArgumentsError,
                     "width and height must be greater than 0."}));
    return;
  }

  CefRefPtr<CefBrowser> browser = GetBrowserByWebviewId(webview_id);
  if (!browser) {
    done_cb(Nullable<WebviewError>{
        WebviewError{WebviewError::kInvalidWebviewId,
                     WebviewError::kInvalidWebviewIdErrorMessage}});
    return;
  }

  CefRefPtr<CefBrowserHost> host = browser->GetHost();
  FlutterWebviewHandler* handler =
      static_cast<FlutterWebviewHandler*>(host->GetClient().get());
  handler->SetViewRect(width, height);
  host->WasResized();
  done_cb(Nullable<WebviewError>());
}

// static
void FlutterWebviewController::LoadUrl(WebviewId webview_id,
                                       const std::string& url,
                                       const DoneCBVoid& done_cb) {
  CEF_REQUIRE_UI_THREAD();

  CefRefPtr<CefBrowser> browser = GetBrowserByWebviewId(webview_id);
  if (!browser) {
    done_cb(Nullable<WebviewError>(
        WebviewError{WebviewError::kInvalidWebviewId,
                     WebviewError::kInvalidWebviewIdErrorMessage}));
    return;
  }

  browser->GetMainFrame()->LoadURL(url);
  done_cb(Nullable<WebviewError>());
}

// static
void FlutterWebviewController::LoadRequest(
    WebviewId webview_id,
    const std::string& uri,
    const std::string& method,
    const std::multimap<std::string, std::string>& headers,
    const std::vector<uint8_t>& body,
    const DoneCBVoid& load_request_cb) {
  CEF_REQUIRE_UI_THREAD();

  // NOTE:
  // The LoadRequest method will fail with "bad IPC message" reason
  // INVALID_INITIATOR_ORIGIN (213) unless you first navigate to the
  // request origin using some other mechanism (LoadURL, link click, etc).

  CefRefPtr<CefRequest> request(CefRequest::Create());

  // copy headers
  CefRequest::HeaderMap headerMap;
  for (auto it = headers.begin(); it != headers.end(); it++) {
    headerMap.insert(std::make_pair(it->first, it->second));
  }

  request->SetURL(uri);
  request->SetMethod(method);
  request->SetHeaderMap(headerMap);

  if (method == "POST") {
    // Add post data to the request.  The correct method and content-
    // type headers will be set by CEF.
    CefRefPtr<CefPostDataElement> postDataElement(CefPostDataElement::Create());
    postDataElement->SetToBytes(body.size(), body.data());
    CefRefPtr<CefPostData> postData(CefPostData::Create());
    postData->AddElement(postDataElement);
    request->SetPostData(postData);
  }

  CefRefPtr<CefBrowser> browser = GetBrowserByWebviewId(webview_id);
  if (!browser) {
    load_request_cb(Nullable<WebviewError>(
        WebviewError{WebviewError::kInvalidWebviewId,
                     WebviewError::kInvalidWebviewIdErrorMessage}));
    return;
  }

  browser->GetMainFrame()->LoadRequest(request);
  load_request_cb(Nullable<WebviewError>());
}

// static
void FlutterWebviewController::CurrentUrl(
    WebviewId webview_id,
    const DoneCB<const std::string&>& current_url_cb) {
  CEF_REQUIRE_UI_THREAD();

  CefRefPtr<CefBrowser> browser = GetBrowserByWebviewId(webview_id);
  if (!browser) {
    current_url_cb(Nullable<WebviewError>(WebviewError{
                       WebviewError::kInvalidWebviewId,
                       WebviewError::kInvalidWebviewIdErrorMessage}),
                   "" /* don't care */);
    return;
  }

  current_url_cb(Nullable<WebviewError>(),
                 browser->GetMainFrame()->GetURL().ToString());
}

// static
void FlutterWebviewController::CanGoBack(WebviewId webview_id,
                                         const DoneCB<bool>& can_go_back_cb) {
  CEF_REQUIRE_UI_THREAD();

  CefRefPtr<CefBrowser> browser = GetBrowserByWebviewId(webview_id);
  if (!browser) {
    can_go_back_cb(Nullable<WebviewError>(WebviewError{
                       WebviewError::kInvalidWebviewId,
                       WebviewError::kInvalidWebviewIdErrorMessage}),
                   false /* don't care */);
    return;
  }

  can_go_back_cb(Nullable<WebviewError>(), browser->CanGoBack());
}

// static
void FlutterWebviewController::CanGoForward(
    WebviewId webview_id,
    const DoneCB<bool>& can_go_forward_cb) {
  CEF_REQUIRE_UI_THREAD();

  CefRefPtr<CefBrowser> browser = GetBrowserByWebviewId(webview_id);
  if (!browser) {
    can_go_forward_cb(Nullable<WebviewError>(WebviewError{
                          WebviewError::kInvalidWebviewId,
                          WebviewError::kInvalidWebviewIdErrorMessage}),
                      false /* don't care */);
    return;
  }

  can_go_forward_cb(Nullable<WebviewError>(), browser->CanGoForward());
}

// static
void FlutterWebviewController::GoBack(WebviewId webview_id,
                                      const DoneCBVoid& done_cb) {
  CEF_REQUIRE_UI_THREAD();

  CefRefPtr<CefBrowser> browser = GetBrowserByWebviewId(webview_id);
  if (!browser) {
    done_cb(Nullable<WebviewError>(
        WebviewError{WebviewError::kInvalidWebviewId,
                     WebviewError::kInvalidWebviewIdErrorMessage}));
    return;
  }

  browser->GoBack();
  done_cb(Nullable<WebviewError>());
}

// static
void FlutterWebviewController::GoForward(WebviewId webview_id,
                                         const DoneCBVoid& done_cb) {
  CEF_REQUIRE_UI_THREAD();

  CefRefPtr<CefBrowser> browser = GetBrowserByWebviewId(webview_id);
  if (!browser) {
    done_cb(Nullable<WebviewError>(
        WebviewError{WebviewError::kInvalidWebviewId,
                     WebviewError::kInvalidWebviewIdErrorMessage}));
    return;
  }

  browser->GoForward();
  done_cb(Nullable<WebviewError>());
}

// static
void FlutterWebviewController::Reload(WebviewId webview_id,
                                      const DoneCBVoid& done_cb) {
  CEF_REQUIRE_UI_THREAD();

  CefRefPtr<CefBrowser> browser = GetBrowserByWebviewId(webview_id);
  if (!browser) {
    done_cb(Nullable<WebviewError>(
        WebviewError{WebviewError::kInvalidWebviewId,
                     WebviewError::kInvalidWebviewIdErrorMessage}));
    return;
  }

  browser->Reload();
  done_cb(Nullable<WebviewError>());
}

// static
void FlutterWebviewController::GetTitle(
    WebviewId webview_id,
    const DoneCB<const std::string&>& get_title_cb) {
  CEF_REQUIRE_UI_THREAD();

  CefRefPtr<CefBrowser> browser = GetBrowserByWebviewId(webview_id);
  if (!browser) {
    get_title_cb(Nullable<WebviewError>(
                     WebviewError{WebviewError::kInvalidWebviewId,
                                  WebviewError::kInvalidWebviewIdErrorMessage}),
                 "" /* don't care */);
    return;
  }

  CefRefPtr<CefNavigationEntry> navigation_entry =
      browser->GetHost()->GetVisibleNavigationEntry();
  if (!navigation_entry) {
    get_title_cb(Nullable<WebviewError>(WebviewError{
                     WebviewError::kRuntimeError,
                     "GetVisibleNavigationEntry() returns nullptr."}),
                 "" /* don't care */);
    return;
  }

  get_title_cb(Nullable<WebviewError>(),
               navigation_entry->GetTitle().ToString());
}

// static
void FlutterWebviewController::RequestRunJavascript(
    WebviewId webview_id,
    int js_run_id,
    const std::string& javascript,
    const DoneCBVoid& done_cb) {
  CEF_REQUIRE_UI_THREAD();

  CefRefPtr<CefBrowser> browser = GetBrowserByWebviewId(webview_id);
  if (!browser) {
    done_cb(Nullable<WebviewError>(
        WebviewError{WebviewError::kInvalidWebviewId,
                     WebviewError::kInvalidWebviewIdErrorMessage}));
    return;
  }

  CefRefPtr<CefFrame> frame = browser->GetMainFrame();
  CefRefPtr<CefProcessMessage> msg =
      flutter_webview_process_messages::Create_FrameMsg_RequestRunJavascript(
          js_run_id, javascript);
  frame->SendProcessMessage(PID_RENDERER, msg);
  done_cb(Nullable<WebviewError>());
}

// static
void FlutterWebviewController::SetCookie(const std::string& domain,
                                         const std::string& path,
                                         const std::string& name,
                                         const std::string& value,
                                         const DoneCBVoid& done_cb) {
  CefCookie cef_cookie;
  CefString(&cef_cookie.domain).FromString(domain);
  CefString(&cef_cookie.path).FromString(path);
  CefString(&cef_cookie.name).FromString(name);
  CefString(&cef_cookie.value).FromString(value);

  class SetCookieCallback : public CefSetCookieCallback {
   public:
    explicit SetCookieCallback(DoneCBVoid callback)
        : callback_(std::move(callback)) {}

    void OnComplete(bool success) override {
      if (!success) {
        LOG(WARNING) << "SetCookieCallback::OnComplete: The cookie was not set "
                        "successfully";
      }
      // Since the webview_flutter_android implementation does not care about
      // the result of CookieManager#setCookie, we return a null WebviewError
      // even if the cookie is not set.
      callback_(Nullable<WebviewError>());
    }

   private:
    DoneCBVoid callback_;

    IMPLEMENT_REFCOUNTING(SetCookieCallback);
  };

  std::string valid_url = "http://" + domain;
  bool set_cookie_return =
      CefCookieManager::GetGlobalManager(nullptr)->SetCookie(
          valid_url, cef_cookie, new SetCookieCallback(done_cb));

  if (!set_cookie_return) {
    constexpr char kErrorMessage[] = "CefCookieManager::SetCookie() failed.";
    LOG(WARNING) << kErrorMessage;
    done_cb(Nullable<WebviewError>(
        WebviewError{WebviewError::kRuntimeError, kErrorMessage}));
  }
}

// static
void FlutterWebviewController::ClearCookies(
    const DoneCB<bool>& clear_cookies_cb) {
  class DeleteCookiesCallback : public CefDeleteCookiesCallback {
   public:
    explicit DeleteCookiesCallback(DoneCB<bool> clear_cookies_cb)
        : clear_cookies_cb_(clear_cookies_cb) {}
    void OnComplete(int num_deleted) override {
      VLOG(1) << __func__
              << ": DeleteCookies completed. num_deleted: " << num_deleted;
      bool was_removed = num_deleted > 0;
      clear_cookies_cb_(Nullable<WebviewError>(), was_removed);
    }

   private:
    DoneCB<bool> clear_cookies_cb_;

    IMPLEMENT_REFCOUNTING(DeleteCookiesCallback);
  };

  bool delete_cookies_return =
      CefCookieManager::GetGlobalManager(nullptr)->DeleteCookies(
          CefString(), CefString(),
          new DeleteCookiesCallback(clear_cookies_cb));
  if (!delete_cookies_return) {
    constexpr char kErrorMessage[] = "CefCookieManager::DeleteCokies() failed.";
    LOG(WARNING) << kErrorMessage;
    clear_cookies_cb(Nullable<WebviewError>(WebviewError{
                         WebviewError::kRuntimeError, kErrorMessage}),
                     false /* don't care */);
  }
}

// static
std::string FlutterWebviewController::GetCefStateName(
    const FlutterWebviewController::CefState state) {
  switch (state) {
    case FlutterWebviewController::CefState::kUninitialized:
      return "Uninitialized";
    case FlutterWebviewController::CefState::kInitializing:
      return "Initializing";
    case FlutterWebviewController::CefState::kInitialized:
      return "Initialized";
    case FlutterWebviewController::CefState::kShuttingDown:
      return "ShuttingDown";
    case FlutterWebviewController::CefState::kShutdown:
      return "Shutdown";
    default:
      return "(Undefined state)";
  }
}
