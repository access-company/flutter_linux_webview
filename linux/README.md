# flutter_linux_webview v0.1.0 plugin Overview

This package provides the Linux Desktop implementation of `webview_flutter` (v3.0.4).

[webview_flutter v3.0.4](https://pub.dev/packages/webview_flutter/versions/3.0.4) is a [federated package](https://docs.flutter.dev/packages-and-plugins/developing-packages#federated-plugins), consisting of an app-facing package, platform interface package, and platform packages.

This plugin package provides the platform part of it for Linux.

| Package Type               | Package Name                                                                                                     | Components (not exhaustive)                                             | 
| -------------------------- | ---------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------- | 
| app-facing package         | [webview_flutter](https://pub.dev/packages/webview_flutter/versions/3.0.4)                                       | WebView, WebViewController                                                    | 
| (↓ depends on)            |                                                                                                                  | (↓ uses)                                                                     | 
| platform interface package | [webview_flutter_platform_interface](https://pub.dev/packages/webview_flutter_platform_interface/versions/1.8.1) | WebViewPlatform, WebViewPlatformController, WebViewCookieManagerPlatform, ... | 
| (↑ depends on)            |                                                                                                                  | (↑ implements, extends, uses)                                                | 
| platform package           | [flutter_linux_android](https://pub.dev/packages/webview_flutter_android/versions/2.8.8)                         | AndroidWebView, WebViewAndroidPlatformController, WebViewAndroidCookieManager, native implementation | 
| platform package           | **flutter_linux_webview (this package)**                                                                         | **LinuxWebView, WebViewLinuxPlatformController, WebViewLinuxCookieManager, native implementation (C++/CEF)** | 

## How a WebView user uses the platform implementation

When users create a [WebView](https://pub.dev/documentation/webview_flutter/3.0.4/webview_flutter/WebView-class.html) widget in their app, they can obtain a [WebViewController](https://pub.dev/documentation/webview_flutter/3.0.4/webview_flutter/WebViewController-class.html) through the `WebView.onWebViewCreated` callback, which they can use to manipulate the WebView.

The `WebViewController` delegates platform-specific operations to [its private member](https://github.com/flutter/plugins/blob/webview_flutter-v3.0.4/packages/webview_flutter/webview_flutter/lib/src/webview.dart#L505) of the [WebViewPlatformController](https://github.com/flutter/plugins/blob/webview_flutter-v3.0.4/packages/webview_flutter/webview_flutter_platform_interface/lib/src/platform_interface/webview_platform_controller.dart#L18) type.

On Linux, this member is an instance of `WebViewLinuxPlatformController`, which extends `WebViewPlatformController`.

Therefore, the `WebViewController` delegates operations to the `WebViewLinuxPlatformController`, which is provided in this package. `WebViewLinuxPlatformController` also invokes the native implementation.

It can be represented using the following diagram:

```
A WebView User ---uses---> WebViewController ---delegates---> WebViewLinuxPlatformController ---invokes---> native implementation (C++/CEF)
```

See `/example` or `../README.md` for examples of using a WebView widget and WebViewController.

### FYI: How the platform implementation is set in WebViewController

1. The user sets `LinuxWebView()` to the `WebView.platform` property.
    * where `LinuxWebView` implements `WebViewPlatform`.
1. The user creates a `WebView` widget in the app.
    * (Before this point, the user must call `LinuxWebViewPlugin.initialize()` (located in `lib/src/linux_webview_plugin.dart`) in advance to start CEF.)
1. In `_WebViewState.build()`, `WebView.platform.build()`, that is, `LinuxWebView.build()` is called.
    * Here [_WebViewState._onWebViewPlatformCreated()](https://github.com/flutter/plugins/blob/webview_flutter-v3.0.4/packages/webview_flutter/webview_flutter/lib/src/webview.dart#L342) is passed to `LinuxWebView.build()` as a callback.
1. `LinuxWebView.build()` returns `WebViewLinuxWidget`.
1. `WebViewLinuxWidget.initState()` is called by the Flutter Framework.
1. In `WebViewLinuxWidget.initState()`, `WebViewLinuxPlatformController` (extends `WebViewPlatformController`) is asynchronously initialized.
1. After initializing `WebViewLinuxPlatformController`, The `WebViewLinuxWidget` returns it to the given `onWebViewPlatformCreated` callback, that is, `_WebViewState._onWebViewPlatformCreated()`.
1. `_WebViewState._onWebViewPlatformCreated()` takes the `WebViewLinuxPlatformController`, creates a `WebViewController` with it, and passes the `WebViewController` to the `WebView.onWebViewCreated` callback.
1. The user obtains the `WebViewController` through the `WebView.onWebViewCreated` callback.
    * This `WebViewController` delegates operations to `WebViewLinuxPlatformController`.

## The Native Plugin Implementation (C++/CEF)

The native implementation of the plugin uses CEF (Chromium Embedded Framework) as the underlying browser (see `../README.md` for the version of CEF) and consists of three main parts:

* `FlutterLinuxWebviewPlugin`
  * The base part of the plugin. It handles the initialization and termination of the plugin, and is the connection point between the Dart-side method calls and `FlutterWebviewController`.
* `FlutterWebviewController`
  * It provides the API to control a WebView, wrapping the CEF API.
* `FlutterWebviewHandler`
  * It defines the behavior of a WebView.

### Initialization, method calls, termination and the threads

* The native plugin runs on the platform thread, which is the same thread on which the Flutter Engine runs.
* When the Flutter application starts, `flutter_linux_webview_plugin_register_with_registrar()` located in `flutter_linux_webview_plugin.cc` is called to initialize this plugin.
* Once the plugin is initialized, method calls from the Dart side come to `method_call_cb()` and are handled by `flutter_linux_webview_plugin_handle_method_call()`.
* An user explicitly calls `LinuxWebViewPlugin.initialize()` (located in `lib/src/linux_webview_plugin.dart`) to start CEF before creating the first WebView.
    * `LinuxWebViewPlugin.initialize()` invokes `FlutterWebviewController::StartCef()` on the native side.
    * `FlutterWebviewController::StartCef()` creates and starts a new thread, the CEF UI thread, whose entry point is `FlutterWebviewController::CefMainThread()`.
    * `FlutterWebviewController::CefMainThread()` calls `CefInitialize()` and `CefRunMessageLoop()`, which starts CEF. (The thread is blocked by the message loop.)
* `FlutterWebviewController` methods except `StartCef()` and `ShutdownCef()` must be executed in the CEF UI thread.
* (**Prior to Flutter 3.10**) When the plugin object is destroyed, that is, when the application exits, `flutter_linux_webview_plugin_class_finalize()` is called, which calls `FlutterWebviewController::ShutdownCef()`.
* (**Flutter 3.10 or later (as of 3.13)**) The user explicitly calls `LinuxWebViewPlugin.terminate()` when the application exits. It invokes `FlutterWebviewController::ShutdownCef()` on the native side.
    * In Flutter 3.10, [WidgetsBindingObserver.didRequestAppExit](https://api.flutter.dev/flutter/widgets/WidgetsBindingObserver/didRequestAppExit.html) API has been added. However, as a side effect of that change, it no longer calls the `flutter_linux_webview_plugin_class_finalize()/_dispose()`. So we need to have users explicitly call `LinuxWebViewPlugin.terminate()`.
        * ref. https://github.com/flutter/flutter/pull/121378 + https://github.com/flutter/engine/pull/40033#discussion_r1200216166
* `FlutterWebviewController::ShutdownCef()` requests all running browsers to exit and waits for all browsers and the CEF UI thread to exit.

### Separate executables layout

CEF runs using a browser process and sub-processes. This plugin executes the browser using the separate sub-process executable layout (ref. https://bitbucket.org/chromiumembedded/cef/wiki/GeneralUsage#markdown-header-separate-sub-process-executable).

The sub-process executable is built with the name `flutter_webview_subprocess`, bundled with the Flutter app, and launched as needed.

The source files for the sub-process executable are located in `linux/subprocess/` as shown below.

```
+-- linux/
    +-- CMakeLists.txt
    +-- <plugin source files>
    +-- subprocess/
    |   +-- src/
    |   |   +-- <CEF sub-process source files>
    |   |   +-- CMakeLists_subprocess_project.txt
    |   +-- cef_distrib_patch/
    |       +-- build_flutter_webview_subprocess.patch
    +-- <CEF binary distribution directory>/    # to be downloaded
        +-- CMakeLists.txt                        # to be modified with `build_flutter_webview_subprocess.patch`
        |-- tests/                                # to be created
        |   +-- flutter_webview_subprocess/       # to be created
        |       +-- CMakeLists.txt                # a symbolic link to `CMakeLists_subprocess_project.txt` to be created
        |       +-- <symbolic links to the source code files for the sub-process executable>    # to be created
        +-- ... # CEF headers, binaries, resources...
```

During the plugin building process, the CEF binary distribution is downloaded and extracted, and the sub-process source files are symlinked in `linux/<CEF binary distribution directory>/tests/flutter_webview_subprocess/`.

## The build process for the plugin

The build process is described in `linux/CMakeLists.txt` and is outlined below.

First of all, this directory (`linux/`) is added using `add_subdirectory()` during the build configuration of the Flutter Linux app project.

1. Verifies that the string `include(flutter/ephemeral/.plugin_symlinks/flutter_linux_webview/linux/cmake/link_to_cef_library.cmake)` is included in the `CMakeLists.txt` of the app project. If it is not included, adds the string to the end of that file, and shows a message to prompt the user to rebuild the app.
    - `linux/cmake/link_to_cef_library.cmake` contains a `target_link_libraries()` command for linking to `libcef.so`.
    - This setting is necessary because the plugin will hang in the `CefInitialize()` function if the Flutter app executable is not linked to `libcef.so`.
2. Downloads and extracts the CEF binary distribution (cef_binary_96.0.18+gfe551e4+chromium-96.0.4664.110_linux64_minimal) in the `linux/` directory.
3. Modifies the CEF binary distribution to prepare for building the sub-process executable.
    1. Creates a directory `linux/<CEF binary distrib dir>/tests/flutter_webview_subprocess` and creates symbolic links to the sub-process source code files in it:
        - `linux/<CEF binary distrib dir>/tests/flutter_webview_subprocess/*.{cc,h}` -> `linux/subprocess/src/*.{cc,h}`
        - `linux/<CEF binary distirb dir>/tests/flutter_webview_subprocess/CMakeLists.txt` -> `linux/subprocess/src/CMakeLists_subprocess_project.txt`
    2. Applies `linux/subprocess/cef_distrib_patch/build_flutter_webview_subprocess.patch` to `linux/<CEF binary distrib dir>/CMakeLists.txt` to enable building for the `tests/flutter_webview_subprocess`.
4. CEF binary files, CEF resource files, and the sub-process executable are bundled in `<Flutter app build dir>/bundle/lib`.
