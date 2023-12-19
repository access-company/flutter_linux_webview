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

import 'dart:async';
import 'dart:io';
import 'logging.dart';
import 'package:flutter/services.dart';

import 'package:path/path.dart' as path;

import 'webview_linux_widget.dart';

enum _PluginState {
  uninitialized,
  initializing,
  initialized,
}

class LinuxWebViewPlugin {
  /// The private MethodChannel. To be exported with [channel].
  /// Users should not create and use their own
  /// MethodChannel('flutter_linux_webview') object.
  static const MethodChannel _channel = MethodChannel('flutter_linux_webview');

  /// The plugin state used for exporting the MethodChannel.
  static _PluginState _pluginState = _PluginState.uninitialized;

  /// The completer for the exported method channels that wait for [initialize].
  static final Completer<void> _pluginInitDone = Completer<void>();

  /// An internal property that users should not use. The exported MethodChannel
  /// that is resolved when the plugin is initialized.
  static Future<MethodChannel> get channel async {
    switch (_pluginState) {
      case _PluginState.uninitialized:
        throw StateError('LinuxWebViewPlugin has not been initialized.'
            ' Call LinuxWebViewPlugin.initialize() first to'
            ' initialize the plugin if you have not already done so.');
      case _PluginState.initializing:
        log.info(
            'LinuxWebViewPlugin.channel is pending completion of plugin initialization...');
        await _pluginInitDone.future;
        return _channel;
      case _PluginState.initialized:
        return _channel;
    }
  }

  /// Initializes the plugin. This method must be called before the first
  /// WebView creation.
  ///
  /// [options] are additional options used to initialize the underlying browser
  /// (CEF; Chromium Embedded Framework). The CEF command line arguments can be
  /// specified. Specify arguments in the following format:
  ///
  /// ```dart
  /// <String, String?>{
  ///   'user-agent': 'UA String',
  ///   'remote-debugging-port': '8888',
  ///   'autoplay-policy': 'no-user-gesture-required',
  /// }
  /// ```
  ///
  /// Note that `--` and `=` are not needed, although the usual CEF command line
  /// arguments are of the form `--switch=value`.
  ///
  /// The following fundamental options are specified by default:
  /// - `browser-subprocess-path`: `<browser subprocess path (determined automatically)>`
  /// - `disable-javascript-close-windows`
  /// - `ozone-platform`: `wayland` (if XDG_SESSION_TYPE is wayland)
  ///
  /// If [options] overlap with the default options, the specified [options]
  /// override the default options.
  ///
  /// You do not necessarily need to wait for this method to complete. [channel]
  /// is resolved when this initialization is completed.
  static Future<void> initialize({Map<String, String?>? options}) async {
    _channel.setMethodCallHandler(WebViewLinuxPlatformController.onMethodCall);
    setupLogger();
    _pluginState = _PluginState.initializing;

    final Map<String, String?> defaultSwitches = {
      if (Platform.environment['XDG_SESSION_TYPE'] == 'wayland')
        'ozone-platform': 'wayland'
    };

    final Map<String, String?> essentialSwitches = {
      'browser-subprocess-path':
          '${path.dirname(Platform.resolvedExecutable)}/lib/flutter_webview_subprocess',
      // Necessary to prevent browsers from closing with window.close():
      'disable-javascript-close-windows': null,
    };

    final Map<String, String?> switches = {}
      ..addAll(defaultSwitches)
      ..addAll(essentialSwitches)
      ..addAll(options ?? {});

    final List<String> commandLineArgs = [
      // The Map in Dart iterates in key insertion order.
      for (MapEntry<String, String?> entry in switches.entries)
        entry.value != null ? '--${entry.key}=${entry.value}' : '--${entry.key}'
    ];

    log.fine('Initialize the plugin with args: $commandLineArgs');

    await _channel.invokeMethod('startCef', <String, dynamic>{
      'commandLineArgs': commandLineArgs,
    });
    _pluginState = _PluginState.initialized;
    _pluginInitDone.complete();
    log.fine('LinuxWebViewPlugin initialization done.');
  }

  /// Terminates the plugin. **In Flutter 3.10 or later, this method must be
  /// called before the application exits. Prior to Flutter 3.10, this method
  /// does not need to be called.** because the plugin automatically exits.
  ///
  /// Since [WidgetsBindingObserver.didRequestAppExit] was added in Flutter
  /// 3.10, that could be used as a timing for plugin termination.
  static Future<void> terminate() async {
    await _channel.invokeMethod('shutdownCef');
    _pluginState = _PluginState.uninitialized;
    log.fine('LinuxWebviewPlugin has been terminated.');
  }
}
