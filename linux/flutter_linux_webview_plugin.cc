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

#include "flutter_linux_webview/flutter_linux_webview_plugin.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>
#include <sys/utsname.h>

#include <algorithm>
#include <iostream>

#include "flutter_linux_webview/flutter_webview_types.h"
#include "flutter_webview_controller.h"
#include "flutter_webview_texture_manager.h"
#include "include/base/cef_callback.h"
#include "include/wrapper/cef_closure_task.h"

#define FLUTTER_LINUX_WEBVIEW_PLUGIN(obj)                                     \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), flutter_linux_webview_plugin_get_type(), \
                              FlutterLinuxWebviewPlugin))

struct _FlutterLinuxWebviewPlugin {
  GObject parent_instance;

  FlMethodChannel* method_channel;
  GdkGLContext* gdk_gl_context;
  FlPluginRegistrar* plugin_registrar;
  std::unique_ptr<FlutterWebviewTextureManager> texture_manager;
};

G_DEFINE_TYPE(FlutterLinuxWebviewPlugin,
              flutter_linux_webview_plugin,
              g_object_get_type())

namespace {

static constexpr char kBadArgumentsError[] = "Bad Arguments";
static constexpr char kPluginError[] = "Plugin Error";

// Checks if the type of |map| is FL_VALUE_TYPE_MAP
bool check_args_is_map(FlValue* map, FlMethodResponse** out_error) {
  if (fl_value_get_type(map) != FL_VALUE_TYPE_MAP) {
    *out_error = FL_METHOD_RESPONSE(fl_method_error_response_new(
        kBadArgumentsError, "The arguments must be Map", nullptr));
    return false;
  }
  return true;
}

// Retrieves an FL_VALUE_TYPE_INT value from the |map| by the |key| and outputs
// it as an int64_t value. Returns false and ourputs |out_error| in case of
// error.
bool get_arg_int64(FlValue* map,
                   const char* key,
                   int64_t* out,
                   FlMethodResponse** out_error) {
  FlValue* arg = fl_value_lookup_string(map, key);
  if (fl_value_get_type(arg) != FL_VALUE_TYPE_INT) {
    *out_error = FL_METHOD_RESPONSE(fl_method_error_response_new(
        kBadArgumentsError, (std::string(key) + " must be int").c_str(),
        nullptr));
    return false;
  }
  // int64_t
  *out = fl_value_get_int(arg);
  return true;
}

// Retrieves an FL_VALUE_TYPE_INT value from the |map| by the |key| and outputs
// it as an int value. The FL_VALUE_TYPE_INT value must be within the int range.
// Returns false and outputs |out_error| in case of error.
bool get_arg_int64_to_int(FlValue* map,
                          const char* key,
                          int* out,
                          FlMethodResponse** out_error) {
  int64_t buf;
  if (!get_arg_int64(map, key, &buf, out_error)) {
    return false;
  }
  if (buf < INT_MIN || INT_MAX < buf) {
    *out_error = FL_METHOD_RESPONSE(fl_method_error_response_new(
        kBadArgumentsError,
        (std::string(key) + " must be in the range of " +
         std::to_string(INT_MIN) + " (INT_MIN) to " + std::to_string(INT_MAX) +
         " (INT_MAX)")
            .c_str(),
        nullptr));
    return false;
  }
  *out = static_cast<int>(buf);
  return true;
}

// Retrieves an FL_VALUE_TYPE_INT value from the |map| by the |key| and outputs
// it as a (CEF-defined) uint32 value. The FL_VALUE_TYPE_INT value must be
// within the uint32 range. Returns false and outputs |out_error| in case of
// error.
bool get_arg_int64_to_uint32(FlValue* map,
                             const char* key,
                             uint32* out,
                             FlMethodResponse** out_error) {
  int64_t buf;
  if (!get_arg_int64(map, key, &buf, out_error)) {
    return false;
  }
  if (buf < 0 || UINT32_MAX < buf) {
    *out_error = FL_METHOD_RESPONSE(fl_method_error_response_new(
        kBadArgumentsError,
        (std::string(key) + " must be in the range of 0 to " +
         std::to_string(UINT32_MAX) + " (UINT32_MAX)")
            .c_str(),
        nullptr));
    return false;
  }
  *out = static_cast<uint32>(buf);
  return true;
}

// Obtains an FL_VALUE_TYPE_INT value from the |map| by the |key| and outputs it
// as a (CEF-defined) char16 value. The FL_VALUE_TYPE_INT value must be within
// the char16 range. Returns false and outputs |out_error| in case of error.
bool get_arg_int64_to_char16(FlValue* map,
                             const char* key,
                             char16* out,
                             FlMethodResponse** out_error) {
  int64_t buf;
  if (!get_arg_int64(map, key, &buf, out_error)) {
    return false;
  }
  if (buf < 0 || USHRT_MAX < buf) {
    *out_error = FL_METHOD_RESPONSE(fl_method_error_response_new(
        kBadArgumentsError,
        (std::string(key) + " must be in the range of 0 to " +
         std::to_string(USHRT_MAX) + " (USHRT_MAX)")
            .c_str(),
        nullptr));
    return false;
  }
  *out = static_cast<char16>(buf);
  return true;
}

// Retrieves an FL_VALUE_TYPE_STRING value from the |map| by the |key| and
// outputs it as an std::string value. Returns false and outputs |out_error| in
// case of error.
bool get_arg_string(FlValue* map,
                    const char* key,
                    std::string* out_string,
                    FlMethodResponse** out_error) {
  FlValue* arg = fl_value_lookup_string(map, key);
  if (fl_value_get_type(arg) != FL_VALUE_TYPE_STRING) {
    *out_error = FL_METHOD_RESPONSE(fl_method_error_response_new(
        kBadArgumentsError, (std::string(key) + " must be string").c_str(),
        nullptr));
    return false;
  }
  *out_string = fl_value_get_string(arg);
  return true;
}

// Retrieves an FL_VALUE_TYPE_BOOL value from the |map| by the |key| and outputs
// it as a bool value. Returns false and outputs |out_error| in case of error.
bool get_arg_bool(FlValue* map,
                  const char* key,
                  bool* out_bool,
                  FlMethodResponse** out_error) {
  FlValue* arg = fl_value_lookup_string(map, key);
  if (fl_value_get_type(arg) != FL_VALUE_TYPE_BOOL) {
    *out_error = FL_METHOD_RESPONSE(fl_method_error_response_new(
        kBadArgumentsError, (std::string(key) + " must be bool").c_str(),
        nullptr));
    return false;
  }
  *out_bool = fl_value_get_bool(arg);
  return true;
}

// Retrieves an FL_VALUE_TYPE_UINT8_LIST value from the |map| by the |key| and
// outputs it as an std::vector<uint8_t> value. Returns false and outputs
// |out_error| in case of error.
bool get_arg_uint8_list(FlValue* map,
                        const char* key,
                        std::vector<uint8_t>* out_uint8,
                        FlMethodResponse** out_error) {
  FlValue* arg = fl_value_lookup_string(map, key);
  if (fl_value_get_type(arg) != FL_VALUE_TYPE_UINT8_LIST) {
    *out_error = FL_METHOD_RESPONSE(fl_method_error_response_new(
        kBadArgumentsError, (std::string(key) + " must be Uint8List").c_str(),
        nullptr));
    return false;
  }
  const uint8_t* arr = fl_value_get_uint8_list(arg);
  size_t len = fl_value_get_length(arg);
  *out_uint8 = std::vector<uint8_t>{&arr[0], &arr[len]};
  return true;
}

// Retrieves an FL_VALUE_TYPE_LIST value which consisting of
// FL_VALUE_TYPE_STRING value from the |map| by the |key| and outputs it as an
// std::vector<std::string> value. Returns false and outputs |out_error| in case
// of error.
bool get_arg_string_list(FlValue* map,
                         const char* key,
                         std::vector<std::string>* out_str_vec,
                         FlMethodResponse** out_error) {
  FlValue* arg = fl_value_lookup_string(map, key);
  if (fl_value_get_type(arg) != FL_VALUE_TYPE_LIST) {
    *out_error = FL_METHOD_RESPONSE(fl_method_error_response_new(
        kBadArgumentsError,
        (std::string(key) + " must be List<String>").c_str(), nullptr));
    return false;
  }
  size_t len = fl_value_get_length(arg);
  std::vector<std::string> buf;
  for (int i = 0; i < len; ++i) {
    FlValue* str_value = (fl_value_get_list_value(arg, i));
    if (fl_value_get_type(str_value) != FL_VALUE_TYPE_STRING) {
      *out_error = FL_METHOD_RESPONSE(fl_method_error_response_new(
          kBadArgumentsError,
          (std::string(key) + " must be List<String>").c_str(), nullptr));
      return false;
    }
    buf.push_back(fl_value_get_string(str_value));
  }
  *out_str_vec = std::move(buf);
  return true;
}

// Retrieves an FL_VALUE_TYPE_MAP value which maps FL_VALUE_TYPE_STRING to
// FL_VALUE_TYPE_STRING from the |map| by the |key| and outputs it as T type
// (map family type is expected). Returns false and outputs |out_error| in case
// of error.
template <typename T>
bool get_arg_string_string_map(FlValue* map,
                               const char* key,
                               T* out,
                               FlMethodResponse** out_error) {
  FlValue* arg = fl_value_lookup_string(map, key);
  if (fl_value_get_type(arg) != FL_VALUE_TYPE_MAP) {
    *out_error = FL_METHOD_RESPONSE(fl_method_error_response_new(
        kBadArgumentsError,
        (std::string(key) + " must be Map<String,String>").c_str(), nullptr));
    return false;
  }

  size_t len = fl_value_get_length(arg);
  T buf;
  for (int i = 0; i < len; i++) {
    FlValue* mapkey = fl_value_get_map_key(arg, i);
    FlValue* mapvalue = fl_value_get_map_value(arg, i);
    if (fl_value_get_type(mapkey) != FL_VALUE_TYPE_STRING ||
        fl_value_get_type(mapvalue) != FL_VALUE_TYPE_STRING) {
      *out_error = FL_METHOD_RESPONSE(fl_method_error_response_new(
          kBadArgumentsError,
          (std::string(key) + " must be Map<String,String>").c_str(), nullptr));
      return false;
    }
    buf.insert(std::make_pair(fl_value_get_string(mapkey),
                              fl_value_get_string(mapvalue)));
  }
  *out = std::move(buf);
  return true;
}

bool is_plugin_alive(FlutterLinuxWebviewPlugin* plugin) {
  bool alive = g_type_check_instance_is_a(
      (GTypeInstance*)plugin, flutter_linux_webview_plugin_get_type());
  if (!alive) {
    std::cerr << "plugin is not alive." << std::endl;
  }
  return alive;
}

gboolean method_call_respond(FlMethodCall* method_call,
                             FlMethodResponse* response) {
  g_autoptr(GError) gerror = NULL;
  gboolean respond_result =
      fl_method_call_respond(method_call, response, &gerror);
  if (gerror != NULL) {
    std::cerr << "fl_method_call_respond() failed: " << gerror->message
              << std::endl;
  }
  return respond_result;
}

gboolean respond_with_webview_error(FlMethodCall* method_call,
                                    const WebviewError& error) {
  g_autoptr(FlMethodResponse) response =
      FL_METHOD_RESPONSE(fl_method_error_response_new(
          error.code.c_str(), error.message.c_str(), nullptr));
  return method_call_respond(method_call, response);
}

gboolean respond_with_value(FlMethodCall* method_call, FlValue* result) {
  g_autoptr(FlMethodResponse) response =
      FL_METHOD_RESPONSE(fl_method_success_response_new(result));
  return method_call_respond(method_call, response);
}

class ReplyFuncAccessor {
 private:
  static FlValue* convert_to_fl_value(const std::string& value) {
#if defined(FLUTTER_WEBVIEW_DEBUG)
    std::cerr << __func__ << "(const std::string&)" << std::endl;
#endif
    return fl_value_new_string_sized(value.c_str(), value.size());
  }

  static FlValue* convert_to_fl_value(bool value) {
#if defined(FLUTTER_WEBVIEW_DEBUG)
    std::cerr << __func__ << "(bool)" << std::endl;
#endif
    return fl_value_new_bool(value);
  }

  static FlValue* convert_to_fl_value(int64_t value) {
#if defined(FLUTTER_WEBVIEW_DEBUG)
    std::cerr << __func__ << "(int64_t)" << std::endl;
#endif
    return fl_value_new_int(value);
  }

  class ReplyFuncVoid {
   public:
    explicit ReplyFuncVoid(FlMethodCall* method_call)
        : method_call_(method_call) {}

    void operator()(Nullable<WebviewError> error) {
      // On the CEF UI thread
      struct Data {
        FlMethodCall* method_call;
        Nullable<WebviewError> error;
      };

      GSourceFunc do_reply = [](gpointer user_data) -> gboolean {
        // On the plugin main thread
        std::unique_ptr<Data> data(static_cast<Data*>(user_data));
        g_autoptr(FlMethodCall) method_call = data->method_call;

        if (!data->error.is_null()) {
          respond_with_webview_error(method_call, data->error.value());
          return FALSE;
        }

        respond_with_value(method_call, nullptr);
        return FALSE;
      };

      // Run the reply function on the platform main thread with the passed data
      std::unique_ptr<Data> data(new Data{method_call_, std::move(error)});
      g_idle_add(do_reply, data.release());
    }

   private:
    FlMethodCall* method_call_;
  };

  template <typename T>
  class ReplyFunc {
   public:
    explicit ReplyFunc(FlMethodCall* method_call) : method_call_(method_call) {}

    void operator()(Nullable<WebviewError> error, T result) {
      // On the CEF UI thread
      struct Data {
        FlMethodCall* method_call;
        Nullable<WebviewError> error;
        T result;
      };

      GSourceFunc do_reply = [](gpointer user_data) -> gboolean {
        // On the plugin main thread
        std::unique_ptr<Data> data(static_cast<Data*>(user_data));
        g_autoptr(FlMethodCall) method_call = data->method_call;

        if (!data->error.is_null()) {
          respond_with_webview_error(method_call, data->error.value());
          return FALSE;
        }

        // NOTE: Make sure that the expected overload is called.
        g_autoptr(FlValue) result = convert_to_fl_value(data->result);
        respond_with_value(method_call, result);
        return FALSE;
      };

      // Run the reply function on the platform main thread with the passed data
      std::unique_ptr<Data> data(
          new Data{method_call_, std::move(error), std::move(result)});
      g_idle_add(do_reply, data.release());
    }

   private:
    FlMethodCall* method_call_;
  };

 public:
  using ReplyCallbackVoid = ReplyFuncVoid;
  using ReplyCallbackString = ReplyFunc<std::string>;
  using ReplyCallbackBool = ReplyFunc<bool>;
};

using ReplyCallbackVoid = ReplyFuncAccessor::ReplyCallbackVoid;
using ReplyCallbackString = ReplyFuncAccessor::ReplyCallbackString;
using ReplyCallbackBool = ReplyFuncAccessor::ReplyCallbackBool;

}  // namespace

// sendMouseMove
static FlMethodResponse* plugin_on_send_mouse_move_async(
    FlutterLinuxWebviewPlugin* plugin,
    FlMethodCall* method_call,
    FlValue* args) {
  FlMethodResponse* error_response;

  if (!check_args_is_map(args, &error_response)) {
    return error_response;
  }

  int64_t webviewId;
  int x;
  int y;
  uint32 modifiers;
  bool mouseLeave;

  if (!get_arg_int64(args, "webviewId", &webviewId, &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_int(args, "x", &x, &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_int(args, "y", &y, &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_uint32(args, "modifiers", &modifiers,
                               &error_response)) {
    return error_response;
  }
  if (!get_arg_bool(args, "mouseLeave", &mouseLeave, &error_response)) {
    return error_response;
  }

  // prevent release
  g_object_ref(method_call);
  ReplyCallbackVoid reply_cb{method_call};
  CefPostTask(TID_UI,
              base::BindOnce(&FlutterWebviewController::SendMouseMove,
                             webviewId, x, y, modifiers, mouseLeave, reply_cb));
  // Will respond later.
  return nullptr;
}

// sendMouseWheel
static FlMethodResponse* plugin_on_send_mouse_wheel_async(
    FlutterLinuxWebviewPlugin* plugin,
    FlMethodCall* method_call,
    FlValue* args) {
  FlMethodResponse* error_response;

  if (!check_args_is_map(args, &error_response)) {
    return error_response;
  }

  int64_t webviewId;
  int x;
  int y;
  int deltaX;
  int deltaY;
  uint32 modifiers;

  if (!get_arg_int64(args, "webviewId", &webviewId, &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_int(args, "x", &x, &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_int(args, "y", &y, &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_int(args, "deltaX", &deltaX, &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_int(args, "deltaY", &deltaY, &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_uint32(args, "modifiers", &modifiers,
                               &error_response)) {
    return error_response;
  }

  // prevent release
  g_object_ref(method_call);
  ReplyCallbackVoid reply_cb{method_call};
  CefPostTask(TID_UI, base::BindOnce(&FlutterWebviewController::SendMouseWheel,
                                     webviewId, x, y, modifiers, deltaX, deltaY,
                                     reply_cb));
  // Will respond later.
  return nullptr;
}

// sendMouseClick
static FlMethodResponse* plugin_on_send_mouse_click_async(
    FlutterLinuxWebviewPlugin* plugin,
    FlMethodCall* method_call,
    FlValue* args) {
  FlMethodResponse* error_response;

  if (!check_args_is_map(args, &error_response)) {
    return error_response;
  }

  int64_t webviewId;
  int x;
  int y;
  uint32 modifiers;
  int mouseButtonType;
  bool mouseUp;
  int clickCount;

  if (!get_arg_int64(args, "webviewId", &webviewId, &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_int(args, "x", &x, &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_int(args, "y", &y, &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_uint32(args, "modifiers", &modifiers,
                               &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_int(args, "mouseButtonType", &mouseButtonType,
                            &error_response)) {
    return error_response;
  }
  if (!get_arg_bool(args, "mouseUp", &mouseUp, &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_int(args, "clickCount", &clickCount, &error_response)) {
    return error_response;
  }

  // prevent release
  g_object_ref(method_call);
  ReplyCallbackVoid reply_cb{method_call};
  CefPostTask(TID_UI,
              base::BindOnce(&FlutterWebviewController::SendMouseClick,
                             webviewId, x, y, modifiers, mouseButtonType,
                             mouseUp, clickCount, reply_cb));
  // Will respond later.
  return nullptr;
}

// sendKey
static FlMethodResponse* plugin_on_send_key_async(
    FlutterLinuxWebviewPlugin* plugin,
    FlMethodCall* method_call,
    FlValue* args) {
  FlMethodResponse* error_response;

  if (!check_args_is_map(args, &error_response)) {
    return error_response;
  }

  int64_t webviewId;
  int keyEventType;
  uint32 modifiers;
  int windowsKeyCode;
  int nativeKeyCode;
  bool isSystemKey;
  char16 character;
  char16 unmodifiedCharacter;

  if (!get_arg_int64(args, "webviewId", &webviewId, &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_int(args, "keyEventType", &keyEventType,
                            &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_uint32(args, "modifiers", &modifiers,
                               &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_int(args, "windowsKeyCode", &windowsKeyCode,
                            &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_int(args, "nativeKeyCode", &nativeKeyCode,
                            &error_response)) {
    return error_response;
  }
  if (!get_arg_bool(args, "isSystemKey", &isSystemKey, &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_char16(args, "character", &character,
                               &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_char16(args, "unmodifiedCharacter",
                               &unmodifiedCharacter, &error_response)) {
    return error_response;
  }

  // prevent release
  g_object_ref(method_call);
  ReplyCallbackVoid reply_cb{method_call};
  CefPostTask(TID_UI, base::BindOnce(&FlutterWebviewController::SendKey,
                                     webviewId, keyEventType, modifiers,
                                     windowsKeyCode, nativeKeyCode, isSystemKey,
                                     character, unmodifiedCharacter, reply_cb));
  // Will respond later.
  return nullptr;
}

// resize
static FlMethodResponse* plugin_on_resize_async(
    FlutterLinuxWebviewPlugin* plugin,
    FlMethodCall* method_call,
    FlValue* args) {
  FlMethodResponse* error_response;

  if (!check_args_is_map(args, &error_response)) {
    return error_response;
  }

  int64_t webviewId;
  int width;
  int height;

  if (!get_arg_int64(args, "webviewId", &webviewId, &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_int(args, "width", &width, &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_int(args, "height", &height, &error_response)) {
    return error_response;
  }

  // prevent release
  g_object_ref(method_call);
  ReplyCallbackVoid reply_cb{method_call};
  CefPostTask(TID_UI, base::BindOnce(&FlutterWebviewController::Resize,
                                     webviewId, width, height, reply_cb));
  // Will respond later.
  return nullptr;
}

// loadUrl
static FlMethodResponse* plugin_on_load_url_async(
    FlutterLinuxWebviewPlugin* plugin,
    FlMethodCall* method_call,
    FlValue* args) {
  FlMethodResponse* error_response;

  if (!check_args_is_map(args, &error_response)) {
    return error_response;
  }

  int64_t webviewId;
  std::string url;

  if (!get_arg_int64(args, "webviewId", &webviewId, &error_response)) {
    return error_response;
  }
  if (!get_arg_string(args, "url", &url, &error_response)) {
    return error_response;
  }

  // prevent release
  g_object_ref(method_call);
  ReplyCallbackVoid reply_cb{method_call};
  CefPostTask(TID_UI, base::BindOnce(&FlutterWebviewController::LoadUrl,
                                     webviewId, url, reply_cb));
  // Will respond later.
  return nullptr;
}

// loadRequest
static FlMethodResponse* plugin_on_load_request_async(
    FlutterLinuxWebviewPlugin* plugin,
    FlMethodCall* method_call,
    FlValue* args) {
  FlMethodResponse* error_response;

  if (!check_args_is_map(args, &error_response)) {
    return error_response;
  }

  int64_t webviewId;
  std::string uri;
  std::string method;
  std::multimap<std::string, std::string> headers;
  std::vector<uint8_t> body;

  if (!get_arg_int64(args, "webviewId", &webviewId, &error_response)) {
    return error_response;
  }
  if (!get_arg_string(args, "uri", &uri, &error_response)) {
    return error_response;
  }
  if (!get_arg_string(args, "method", &method, &error_response)) {
    return error_response;
  }
  if (!get_arg_string_string_map<std::multimap<std::string, std::string>>(
          args, "headers", &headers, &error_response)) {
    return error_response;
  }
  if (!get_arg_uint8_list(args, "body", &body, &error_response)) {
    return error_response;
  }

  // prevent release
  g_object_ref(method_call);
  ReplyCallbackVoid reply_cb{method_call};
  CefPostTask(TID_UI,
              base::BindOnce(&FlutterWebviewController::LoadRequest, webviewId,
                             uri, method, headers, body, reply_cb));
  // Will respond later.
  return nullptr;
}

// currentUrl
static FlMethodResponse* plugin_on_current_url_async(
    FlutterLinuxWebviewPlugin* plugin,
    FlMethodCall* method_call,
    FlValue* args) {
  FlMethodResponse* error_response;

  if (!check_args_is_map(args, &error_response)) {
    return error_response;
  }

  int64_t webviewId;

  if (!get_arg_int64(args, "webviewId", &webviewId, &error_response)) {
    return error_response;
  }

  // prevent release
  g_object_ref(method_call);
  ReplyCallbackString reply_cb{method_call};
  CefPostTask(TID_UI, base::BindOnce(&FlutterWebviewController::CurrentUrl,
                                     webviewId, reply_cb));
  // Will responed later.
  return nullptr;
}

// canGoBack
static FlMethodResponse* plugin_on_can_go_back_async(
    FlutterLinuxWebviewPlugin* plugin,
    FlMethodCall* method_call,
    FlValue* args) {
  FlMethodResponse* error_response;

  if (!check_args_is_map(args, &error_response)) {
    return error_response;
  }

  int64_t webviewId;

  if (!get_arg_int64(args, "webviewId", &webviewId, &error_response)) {
    return error_response;
  }
  // prevent release
  g_object_ref(method_call);
  ReplyCallbackBool reply_cb{method_call};
  CefPostTask(TID_UI, base::BindOnce(&FlutterWebviewController::CanGoBack,
                                     webviewId, reply_cb));
  // Will respond later.
  return nullptr;
}

// canGoForward
static FlMethodResponse* plugin_on_can_go_forward_async(
    FlutterLinuxWebviewPlugin* plugin,
    FlMethodCall* method_call,
    FlValue* args) {
  FlMethodResponse* error_response;

  if (!check_args_is_map(args, &error_response)) {
    return error_response;
  }

  int64_t webviewId;

  if (!get_arg_int64(args, "webviewId", &webviewId, &error_response)) {
    return error_response;
  }

  // prevent release
  g_object_ref(method_call);
  ReplyCallbackBool reply_cb{method_call};
  CefPostTask(TID_UI, base::BindOnce(&FlutterWebviewController::CanGoForward,
                                     webviewId, reply_cb));
  // Will respond later.
  return nullptr;
}

// goBack
static FlMethodResponse* plugin_on_go_back_async(
    FlutterLinuxWebviewPlugin* plugin,
    FlMethodCall* method_call,
    FlValue* args) {
  FlMethodResponse* error_response;

  if (!check_args_is_map(args, &error_response)) {
    return error_response;
  }

  int64_t webviewId;

  if (!get_arg_int64(args, "webviewId", &webviewId, &error_response)) {
    return error_response;
  }

  // prevent release
  g_object_ref(method_call);
  ReplyCallbackVoid reply_cb{method_call};
  CefPostTask(TID_UI, base::BindOnce(&FlutterWebviewController::GoBack,
                                     webviewId, reply_cb));
  // Will respond later.
  return nullptr;
}

// goForward
static FlMethodResponse* plugin_on_go_forward_async(
    FlutterLinuxWebviewPlugin* plugin,
    FlMethodCall* method_call,
    FlValue* args) {
  FlMethodResponse* error_response;

  if (!check_args_is_map(args, &error_response)) {
    return error_response;
  }

  int64_t webviewId;

  if (!get_arg_int64(args, "webviewId", &webviewId, &error_response)) {
    return error_response;
  }

  // prevent release
  g_object_ref(method_call);
  ReplyCallbackVoid reply_cb{method_call};
  CefPostTask(TID_UI, base::BindOnce(&FlutterWebviewController::GoForward,
                                     webviewId, reply_cb));
  // Will respond later.
  return nullptr;
}

// reload
static FlMethodResponse* plugin_on_reload_async(
    FlutterLinuxWebviewPlugin* plugin,
    FlMethodCall* method_call,
    FlValue* args) {
  FlMethodResponse* error_response;

  if (!check_args_is_map(args, &error_response)) {
    return error_response;
  }

  int64_t webviewId;

  if (!get_arg_int64(args, "webviewId", &webviewId, &error_response)) {
    return error_response;
  }

  // prevent release
  g_object_ref(method_call);
  ReplyCallbackVoid reply_cb{method_call};
  CefPostTask(TID_UI, base::BindOnce(&FlutterWebviewController::Reload,
                                     webviewId, reply_cb));
  // Will respond later.
  return nullptr;
}

// getTitle
static FlMethodResponse* plugin_on_get_title_async(
    FlutterLinuxWebviewPlugin* plugin,
    FlMethodCall* method_call,
    FlValue* args) {
  FlMethodResponse* error_response;

  if (!check_args_is_map(args, &error_response)) {
    return error_response;
  }

  int64_t webviewId;

  if (!get_arg_int64(args, "webviewId", &webviewId, &error_response)) {
    return error_response;
  }

  // prevent release
  g_object_ref(method_call);
  ReplyCallbackString reply_cb{method_call};
  CefPostTask(TID_UI, base::BindOnce(&FlutterWebviewController::GetTitle,
                                     webviewId, reply_cb));
  // Will respond later.
  return nullptr;
}

// implementation of runJavascript, runJavascriptReturningResult and
// evaluateJavascript
static FlMethodResponse* plugin_on_request_run_javascript_async(
    FlutterLinuxWebviewPlugin* plugin,
    FlMethodCall* method_call,
    FlValue* args) {
  FlMethodResponse* error_response;

  if (!check_args_is_map(args, &error_response)) {
    return error_response;
  }

  int64_t webviewId;
  int jsRunId;
  std::string javascript;

  if (!get_arg_int64(args, "webviewId", &webviewId, &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_int(args, "jsRunId", &jsRunId, &error_response)) {
    return error_response;
  }
  if (!get_arg_string(args, "javascript", &javascript, &error_response)) {
    return error_response;
  }

  // prevent release
  g_object_ref(method_call);
  ReplyCallbackVoid reply_cb{method_call};
  CefPostTask(TID_UI,
              base::BindOnce(&FlutterWebviewController::RequestRunJavascript,
                             webviewId, jsRunId, javascript, reply_cb));
  // Will respond later.
  return nullptr;
}

// setCookie
static FlMethodResponse* plugin_on_set_cookie_async(
    FlutterLinuxWebviewPlugin* plugin,
    FlMethodCall* method_call,
    FlValue* args) {
  FlMethodResponse* error_response;

  if (!check_args_is_map(args, &error_response)) {
    return error_response;
  }

  std::string domain;
  std::string path;
  std::string name;
  std::string value;

  if (!get_arg_string(args, "domain", &domain, &error_response)) {
    return error_response;
  }
  if (!get_arg_string(args, "path", &path, &error_response)) {
    return error_response;
  }
  if (!get_arg_string(args, "name", &name, &error_response)) {
    return error_response;
  }
  if (!get_arg_string(args, "value", &value, &error_response)) {
    return error_response;
  }

  // prevent release
  g_object_ref(method_call);
  ReplyCallbackVoid reply_cb{method_call};
  CefPostTask(TID_UI, base::BindOnce(&FlutterWebviewController::SetCookie,
                                     domain, path, name, value, reply_cb));
  // Will respond later.
  return nullptr;
}

// clearCookies
static FlMethodResponse* plugin_on_clear_cookies_async(
    FlutterLinuxWebviewPlugin* plugin,
    FlMethodCall* method_call,
    FlValue* args) {
  // prevent release
  g_object_ref(method_call);
  ReplyCallbackBool reply_cb{method_call};
  CefPostTask(TID_UI, base::BindOnce(&FlutterWebviewController::ClearCookies,
                                     reply_cb));
  // Will respond later.
  return nullptr;
}

// createBrowser
static FlMethodResponse* plugin_on_create_browser_async(
    FlutterLinuxWebviewPlugin* plugin,
    FlMethodCall* method_call,
    FlValue* args) {
  FlMethodResponse* error_response;

  if (!check_args_is_map(args, &error_response)) {
    return error_response;
  }

  int64_t webviewId;
  std::string initialUrl;
  std::vector<uint8_t> backgroundColor;
  int initialWidth;
  int initialHeight;

  if (!get_arg_int64(args, "webviewId", &webviewId, &error_response)) {
    return error_response;
  }
  if (!get_arg_string(args, "initialUrl", &initialUrl, &error_response)) {
    return error_response;
  }
  if (!get_arg_uint8_list(args, "backgroundColor", &backgroundColor,
                          &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_int(args, "initialWidth", &initialWidth,
                            &error_response)) {
    return error_response;
  }
  if (!get_arg_int64_to_int(args, "initialHeight", &initialHeight,
                            &error_response)) {
    return error_response;
  }

  FlCustomTextureGL* texture =
      plugin->texture_manager->CreateAndRegisterTexture(
          webviewId, plugin->gdk_gl_context,
          fl_plugin_registrar_get_texture_registrar(plugin->plugin_registrar),
          initialWidth, initialHeight);
  if (texture == nullptr) {
    return FL_METHOD_RESPONSE(fl_method_error_response_new(
        kPluginError, "TextureManager::CreateAndRegisterTexture() failed.",
        nullptr));
  }

  auto on_paint_begin = [plugin](WebviewId webview_id) {
    // On the CEF UI thread
    if (!is_plugin_alive(plugin)) {
      return;
    }
    gdk_gl_context_make_current(plugin->gdk_gl_context);
  };

  auto on_paint_end = [plugin](WebviewId webview_id) {
    // On the CEF UI thread
    if (!is_plugin_alive(plugin)) {
      return;
    }
    gdk_gl_context_clear_current();

    struct Data {
      FlutterLinuxWebviewPlugin* plugin;
      WebviewId webview_id;
    };

    GSourceFunc func = [](gpointer user_data) -> gboolean {
      // On the plugin main thread
      std::unique_ptr<Data> data(static_cast<Data*>(user_data));
      if (!is_plugin_alive(data->plugin)) {
        return FALSE;
      }
      FlCustomTextureGL* texture =
          data->plugin->texture_manager->GetTexture(data->webview_id);
      if (texture == nullptr) {
        std::cerr << "Warning: Could not get the texture for webview_id="
                  << data->webview_id << std::endl;
        return FALSE;
      }
      if (!fl_texture_registrar_mark_texture_frame_available(
              fl_plugin_registrar_get_texture_registrar(
                  data->plugin->plugin_registrar),
              FL_TEXTURE(texture))) {
        std::cerr
            << "Error: fl_texture_registrar_mark_texture_frame_available() "
               "failed."
            << std::endl;
        return FALSE;
      }
      return FALSE;
    };

    std::unique_ptr<Data> data(new Data{plugin, webview_id});
    g_idle_add(func, data.release());
  };

  WebviewCreationParams::PageStartedCallback on_page_started =
      [plugin](WebviewId webview_id, const std::string& url) {
        // On the CEF UI thread
        struct Data {
          FlutterLinuxWebviewPlugin* plugin;
          WebviewId webview_id;
          std::string url;
        };

        GSourceFunc func = [](gpointer user_data) -> gboolean {
          // On the plugin main thread
          std::unique_ptr<Data> data(static_cast<Data*>(user_data));
          if (!is_plugin_alive(data->plugin)) {
            return FALSE;
          }
          g_autoptr(FlValue) args = fl_value_new_map();
          fl_value_set_string_take(args, "webviewId",
                                   fl_value_new_int(data->webview_id));
          fl_value_set_string_take(
              args, "url",
              fl_value_new_string_sized(data->url.c_str(), data->url.size()));
          fl_method_channel_invoke_method(data->plugin->method_channel,
                                          "onPageStarted", args, NULL, NULL,
                                          NULL);
          return FALSE;
        };

        std::unique_ptr<Data> data(new Data{plugin, webview_id, url});
        g_idle_add(func, data.release());
      };

  WebviewCreationParams::PageFinishedCallback on_page_finished =
      [plugin](WebviewId webview_id, const std::string& url) {
        // On the CEF UI thread
        struct Data {
          FlutterLinuxWebviewPlugin* plugin;
          WebviewId webview_id;
          std::string url;
        };

        GSourceFunc func = [](gpointer user_data) -> gboolean {
          // On the plugin main thread
          std::unique_ptr<Data> data(static_cast<Data*>(user_data));
          if (!is_plugin_alive(data->plugin)) {
            return FALSE;
          }
          g_autoptr(FlValue) args = fl_value_new_map();
          fl_value_set_string_take(args, "webviewId",
                                   fl_value_new_int(data->webview_id));
          fl_value_set_string_take(
              args, "url",
              fl_value_new_string_sized(data->url.c_str(), data->url.size()));
          fl_method_channel_invoke_method(data->plugin->method_channel,
                                          "onPageFinished", args, NULL, NULL,
                                          NULL);
          return FALSE;
        };

        std::unique_ptr<Data> data(new Data{plugin, webview_id, url});
        g_idle_add(func, data.release());
      };

  WebviewCreationParams::PageLoadingCallback on_progress =
      [plugin](WebviewId webview_id, int progress) {
        // On the CEF UI thread
        struct Data {
          FlutterLinuxWebviewPlugin* plugin;
          WebviewId webview_id;
          int progress;
        };

        GSourceFunc func = [](gpointer user_data) -> gboolean {
          // On the plugin main thread
          std::unique_ptr<Data> data(static_cast<Data*>(user_data));
          if (!is_plugin_alive(data->plugin)) {
            return FALSE;
          }
          g_autoptr(FlValue) args = fl_value_new_map();
          fl_value_set_string_take(args, "webviewId",
                                   fl_value_new_int(data->webview_id));
          fl_value_set_string_take(args, "progress",
                                   fl_value_new_int(data->progress));
          fl_method_channel_invoke_method(data->plugin->method_channel,
                                          "onProgress", args, NULL, NULL, NULL);
          return FALSE;
        };

        std::unique_ptr<Data> data(new Data{plugin, webview_id, progress});
        g_idle_add(func, data.release());
      };

  WebviewCreationParams::WebResourceErrorCallback on_web_resource_error =
      [plugin](WebviewId webview_id, int errorCode,
               const std::string& description, const std::string& failingUrl) {
        // On the CEF UI thread
        struct Data {
          FlutterLinuxWebviewPlugin* plugin;
          WebviewId webview_id;
          int errorCode;
          std::string description;
          std::string failingUrl;
        };

        GSourceFunc func = [](gpointer user_data) -> gboolean {
          // On the plugin main thread
          std::unique_ptr<Data> data(static_cast<Data*>(user_data));
          if (!is_plugin_alive(data->plugin)) {
            return FALSE;
          }
          g_autoptr(FlValue) args = fl_value_new_map();
          fl_value_set_string_take(args, "webviewId",
                                   fl_value_new_int(data->webview_id));
          fl_value_set_string_take(args, "errorCode",
                                   fl_value_new_int(data->errorCode));
          fl_value_set_string_take(
              args, "description",
              fl_value_new_string_sized(data->description.c_str(),
                                        data->description.size()));
          fl_value_set_string_take(
              args, "failingUrl",
              fl_value_new_string_sized(data->failingUrl.c_str(),
                                        data->failingUrl.size()));
          fl_method_channel_invoke_method(data->plugin->method_channel,
                                          "onWebResourceError", args, NULL,
                                          NULL, NULL);
          return FALSE;
        };

        std::unique_ptr<Data> data(
            new Data{plugin, webview_id, errorCode, description, failingUrl});
        g_idle_add(func, data.release());
      };

  WebviewCreationParams::JavascriptResultCallback on_javascript_result =
      [plugin](WebviewId webview_id, int js_run_id, bool was_executed,
               bool is_exception, const std::string& result,
               bool is_undefined) {
        // On the CEF UI thread
        struct Data {
          FlutterLinuxWebviewPlugin* plugin;
          WebviewId webview_id;
          int js_run_id;
          bool was_executed;
          bool is_exception;
          std::string result;
          bool is_undefined;
        };

        GSourceFunc func = [](gpointer user_data) -> gboolean {
          // On the plugin main thread
          std::unique_ptr<Data> data(static_cast<Data*>(user_data));
          if (!is_plugin_alive(data->plugin)) {
            return FALSE;
          }
          g_autoptr(FlValue) args = fl_value_new_map();
          fl_value_set_string_take(args, "webviewId",
                                   fl_value_new_int(data->webview_id));
          fl_value_set_string_take(args, "jsRunId",
                                   fl_value_new_int(data->js_run_id));
          fl_value_set_string_take(args, "wasExecuted",
                                   fl_value_new_bool(data->was_executed));
          fl_value_set_string_take(args, "isException",
                                   fl_value_new_bool(data->is_exception));
          fl_value_set_string_take(
              args, "result",
              fl_value_new_string_sized(data->result.c_str(),
                                        data->result.size()));
          fl_value_set_string_take(args, "isUndefined",
                                   fl_value_new_bool(data->is_undefined));
          fl_method_channel_invoke_method(data->plugin->method_channel,
                                          "javascriptResult", args, NULL, NULL,
                                          NULL);
          return FALSE;
        };

        std::unique_ptr<Data> data(new Data{plugin, webview_id, js_run_id,
                                            was_executed, is_exception, result,
                                            is_undefined});
        g_idle_add(func, data.release());
      };

  const WebviewCreationParams params{
      texture->native_texture_id,        // native_texture_id
      initialWidth,                      // width
      initialHeight,                     // height
      std::move(on_paint_begin),         // on_paint_begin
      std::move(on_paint_end),           // on_paint_end
      std::move(initialUrl),             // url
      std::move(backgroundColor),        // background_color
      std::move(on_page_started),        // on_page_started
      std::move(on_page_finished),       // on_page_finished
      std::move(on_progress),            // on_progress
      std::move(on_web_resource_error),  // on_web_resource_error
      std::move(on_javascript_result),   // on_javascript_result
  };

  int64_t fl_texture_id =
      plugin->texture_manager->GetTextureId(FL_TEXTURE(texture));

  // prevent release
  g_object_ref(method_call);
  using DoneCBVoid = FlutterWebviewController::DoneCBVoid;
  DoneCBVoid callback = [method_call,
                         fl_texture_id](Nullable<WebviewError> error) {
    // On the CEF UI thread
    struct Data {
      FlMethodCall* method_call;
      int64_t fl_texture_id;
      Nullable<WebviewError> error;
    };

    GSourceFunc func = [](gpointer user_data) -> gboolean {
      // On the plugin main thread
      std::unique_ptr<Data> data(static_cast<Data*>(user_data));
      g_autoptr(FlMethodCall) method_call = data->method_call;
      if (!data->error.is_null()) {
        respond_with_webview_error(method_call, data->error.value());
        return FALSE;
      }
      // respond fl_texture_id to the Dart side
      g_autoptr(FlValue) result = fl_value_new_int(data->fl_texture_id);
      respond_with_value(method_call, result);
      return FALSE;
    };

    std::unique_ptr<Data> data(
        new Data{method_call, fl_texture_id, std::move(error)});
    g_idle_add(func, data.release());
  };
  CefPostTask(TID_UI, base::BindOnce(&FlutterWebviewController::CreateBrowser,
                                     webviewId, params, callback));
  // Will respond later.
  return nullptr;
}

// disposeBrowser
static FlMethodResponse* plugin_on_dispose_browser_async(
    FlutterLinuxWebviewPlugin* plugin,
    FlMethodCall* method_call,
    FlValue* args) {
  FlMethodResponse* error_response;

  if (!check_args_is_map(args, &error_response)) {
    return error_response;
  }

  int64_t webviewId;

  if (!get_arg_int64(args, "webviewId", &webviewId, &error_response)) {
    return error_response;
  }

  // prevent release
  g_object_ref(method_call);

  using DoneCBVoid = FlutterWebviewController::DoneCBVoid;
  DoneCBVoid callback = [method_call, plugin,
                         webviewId](Nullable<WebviewError> error) {
    // On the CEF UI thread
    struct Data {
      FlMethodCall* method_call;
      FlutterLinuxWebviewPlugin* plugin;
      WebviewId webviewId;
      Nullable<WebviewError> error;
    };

    GSourceFunc func = [](gpointer user_data) -> gboolean {
      // On the plugin main thread
      std::unique_ptr<Data> data(static_cast<Data*>(user_data));
      g_autoptr(FlMethodCall) method_call = data->method_call;
      if (!is_plugin_alive(data->plugin)) {
        return FALSE;
      }
      if (!data->error.is_null()) {
        respond_with_webview_error(method_call, data->error.value());
        return FALSE;
      }
      if (!data->plugin->texture_manager->UnregisterAndDestroyTexture(
              data->webviewId, fl_plugin_registrar_get_texture_registrar(
                                   data->plugin->plugin_registrar))) {
        std::cerr << "Error: TextureManager::UnregisterAndDestroyTexture() "
                     "failed."
                  << std::endl;
        g_autoptr(FlMethodResponse) response =
            FL_METHOD_RESPONSE(fl_method_error_response_new(
                kPluginError,
                "TextureManager::UnregisterAndDestroyTexture() failed.",
                nullptr));
        method_call_respond(method_call, response);
        return FALSE;
      }
      respond_with_value(method_call, nullptr);
      return FALSE;
    };
    std::unique_ptr<Data> data(
        new Data{method_call, plugin, webviewId, std::move(error)});
    g_idle_add(func, data.release());
  };
  CefPostTask(TID_UI, base::BindOnce(&FlutterWebviewController::CloseBrowser,
                                     webviewId, callback));
  // Will respond later.
  return nullptr;
}

// startCef
static FlMethodResponse* plugin_on_start_cef_async(
    FlutterLinuxWebviewPlugin* plugin,
    FlMethodCall* method_call,
    FlValue* args) {
  FlMethodResponse* error_response;

  std::vector<std::string> commandLineArgs;

  if (!get_arg_string_list(args, "commandLineArgs", &commandLineArgs,
                           &error_response)) {
    return error_response;
  }

  // prevent release
  g_object_ref(method_call);
  ReplyCallbackVoid reply_cb{method_call};
  Nullable<WebviewError> maybe_error =
      FlutterWebviewController::StartCef(commandLineArgs, reply_cb);
  if (!maybe_error.is_null()) {
    // Respond immediately.
    g_object_unref(method_call);
    WebviewError error = maybe_error.value();
    return FL_METHOD_RESPONSE(fl_method_error_response_new(
        error.code.c_str(), error.message.c_str(), nullptr));
  }
  // Will respond later.
  return nullptr;
}

// shutdownCef
// There is no asynchronous part.
static FlMethodResponse* plugin_on_shutdown_cef(
    FlutterLinuxWebviewPlugin* plugin,
    FlMethodCall* method_call,
    FlValue* args) {
  Nullable<WebviewError> maybe_error = FlutterWebviewController::ShutdownCef();
  if (!maybe_error.is_null()) {
    WebviewError error = maybe_error.value();
    return FL_METHOD_RESPONSE(fl_method_error_response_new(
        error.code.c_str(), error.message.c_str(), nullptr));
  }

  plugin->texture_manager->UnregisterAndDestroyAllTextures(
      fl_plugin_registrar_get_texture_registrar(plugin->plugin_registrar),
      /* skip_unregister_texture= */ false);

  return FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
}

// Called when a method call is received from Flutter.
static void flutter_linux_webview_plugin_handle_method_call(
    FlutterLinuxWebviewPlugin* self,
    FlMethodCall* method_call) {
  const gchar* method = fl_method_call_get_name(method_call);
  FlValue* args = fl_method_call_get_args(method_call);
  g_autoptr(FlMethodResponse) response = nullptr;

  if (0 == strcmp(method, "sendMouseMove")) {
    response = plugin_on_send_mouse_move_async(self, method_call, args);
  } else if (0 == strcmp(method, "sendMouseWheel")) {
    response = plugin_on_send_mouse_wheel_async(self, method_call, args);
  } else if (0 == strcmp(method, "sendMouseClick")) {
    response = plugin_on_send_mouse_click_async(self, method_call, args);
  } else if (0 == strcmp(method, "sendKey")) {
    response = plugin_on_send_key_async(self, method_call, args);
  } else if (0 == strcmp(method, "resize")) {
    response = plugin_on_resize_async(self, method_call, args);
  } else if (0 == strcmp(method, "loadUrl")) {
    response = plugin_on_load_url_async(self, method_call, args);
  } else if (0 == strcmp(method, "loadRequest")) {
    response = plugin_on_load_request_async(self, method_call, args);
  } else if (0 == strcmp(method, "currentUrl")) {
    response = plugin_on_current_url_async(self, method_call, args);
  } else if (0 == strcmp(method, "canGoBack")) {
    response = plugin_on_can_go_back_async(self, method_call, args);
  } else if (0 == strcmp(method, "canGoForward")) {
    response = plugin_on_can_go_forward_async(self, method_call, args);
  } else if (0 == strcmp(method, "goBack")) {
    response = plugin_on_go_back_async(self, method_call, args);
  } else if (0 == strcmp(method, "goForward")) {
    response = plugin_on_go_forward_async(self, method_call, args);
  } else if (0 == strcmp(method, "reload")) {
    response = plugin_on_reload_async(self, method_call, args);
  } else if (0 == strcmp(method, "getTitle")) {
    response = plugin_on_get_title_async(self, method_call, args);
  } else if (0 == strcmp(method, "requestRunJavascript")) {
    response = plugin_on_request_run_javascript_async(self, method_call, args);
  } else if (0 == strcmp(method, "setCookie")) {
    response = plugin_on_set_cookie_async(self, method_call, args);
  } else if (0 == strcmp(method, "clearCookies")) {
    response = plugin_on_clear_cookies_async(self, method_call, args);
  } else if (0 == strcmp(method, "createBrowser")) {
    response = plugin_on_create_browser_async(self, method_call, args);
  } else if (0 == strcmp(method, "disposeBrowser")) {
    response = plugin_on_dispose_browser_async(self, method_call, args);
  } else if (0 == strcmp(method, "startCef")) {
    response = plugin_on_start_cef_async(self, method_call, args);
  } else if (0 == strcmp(method, "shutdownCef")) {
    response = plugin_on_shutdown_cef(self, method_call, args);
  } else {
    response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
  }

  if (response != nullptr) {
    method_call_respond(method_call, response);
  }
}


static void flutter_linux_webview_plugin_dispose(GObject* object) {
  // Since Flutter 3.10 (as of Flutter 3.13), this "dispose" function is no
  // longer called as a new `ServicesBinding.handleRequestAppExit()` has been
  // added to the Framework (https://github.com/flutter/flutter/pull/121378) and
  // implemented for Linux (https://github.com/flutter/engine/pull/40033).
  // TODO(Ino):  Even in Flutter 3.10 and later, automatic cleanup should be
  // performed in the destructor of the plugin, rather than by the user calling
  // LinuxWebViewPlugin.terminate().
#if FLUTTER_WEBVIEW_DEBUG
  std::cerr << __func__ << ": called!" << std::endl;
#endif  // FLUTTER_WEBVIEW_DEBUG

  FlutterLinuxWebviewPlugin* self = FLUTTER_LINUX_WEBVIEW_PLUGIN(object);

  FlutterWebviewController::ShutdownCef();
  // In this "dispose" function, which is only called prior to Flutter 3.10,
  // fl_texture_registrar_unregister_texture() fails with "Unregistering a
  // non-existent texture", so it seems that the `fl_texture` s are already
  // automatically unregistered from the engine at this point.
  // Therefore we skip fl_texture_registrar_unregister_texture() here.
  self->texture_manager->UnregisterAndDestroyAllTextures(
      fl_plugin_registrar_get_texture_registrar(self->plugin_registrar),
      /* skip_unregister_textures= */ true);
  self->texture_manager.reset();
  g_clear_object(&self->method_channel);
  g_clear_object(&self->gdk_gl_context);
  g_clear_object(&self->plugin_registrar);

  G_OBJECT_CLASS(flutter_linux_webview_plugin_parent_class)->dispose(object);
}

static void flutter_linux_webview_plugin_class_init(
    FlutterLinuxWebviewPluginClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = flutter_linux_webview_plugin_dispose;
}

static void flutter_linux_webview_plugin_init(FlutterLinuxWebviewPlugin* self) {
}

static void method_call_cb(FlMethodChannel* channel,
                           FlMethodCall* method_call,
                           gpointer user_data) {
  FlutterLinuxWebviewPlugin* plugin = FLUTTER_LINUX_WEBVIEW_PLUGIN(user_data);
  flutter_linux_webview_plugin_handle_method_call(plugin, method_call);
}

// Entry point of the Flutter Linux platform plugin
void flutter_linux_webview_plugin_register_with_registrar(
    FlPluginRegistrar* registrar) {
  FlutterLinuxWebviewPlugin* plugin = FLUTTER_LINUX_WEBVIEW_PLUGIN(
      g_object_new(flutter_linux_webview_plugin_get_type(), nullptr));

  g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();
  g_autoptr(FlMethodChannel) channel =
      fl_method_channel_new(fl_plugin_registrar_get_messenger(registrar),
                            "flutter_linux_webview", FL_METHOD_CODEC(codec));
  fl_method_channel_set_method_call_handler(
      channel, method_call_cb, g_object_ref(plugin), g_object_unref);

  // Own the method channel
  plugin->method_channel = FL_METHOD_CHANNEL(g_object_ref(channel));

  FlView* fl_view = fl_plugin_registrar_get_view(registrar);
  GdkWindow* window = gtk_widget_get_parent_window(GTK_WIDGET(fl_view));
  g_autoptr(GError) gerror = NULL;
  g_autoptr(GdkGLContext) gl_context =
      gdk_window_create_gl_context(window, &gerror);
  if (gerror != NULL) {
    std::cerr << "Error: gdk_window_create_gl_context() failed:"
              << gerror->message << std::endl;
    plugin->gdk_gl_context = NULL;
  } else {
    // Own the gl context
    plugin->gdk_gl_context = GDK_GL_CONTEXT(g_object_ref(gl_context));
  }

  // Own the plugin registrar to get a FlTextureRegistrar from it later.
  plugin->plugin_registrar = FL_PLUGIN_REGISTRAR(g_object_ref(registrar));

  plugin->texture_manager = std::make_unique<FlutterWebviewTextureManager>();

  g_object_unref(plugin);
}
