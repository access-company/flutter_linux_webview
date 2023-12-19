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
import 'dart:convert';
import 'dart:io';
// Do not remove, required to use Uint8List prior to Flutter 3.10
import 'dart:typed_data';

import 'package:path/path.dart' as p;
import 'package:flutter/widgets.dart';
import 'package:flutter/services.dart';
import 'package:flutter/gestures.dart';
import 'package:webview_flutter_platform_interface/webview_flutter_platform_interface.dart';

import 'instance_manager.dart';
import 'linux_webview_plugin.dart';
import 'logging.dart';
import 'webview_linux_cookie_manager.dart';
import 'cef_types.dart';
import 'native_key_code.dart';
import 'windows_key_code.dart';

class WebViewLinuxWidget extends StatefulWidget {
  const WebViewLinuxWidget({
    Key? key,
    this.initialWidth = 640,
    this.initialHeight = 480,
    required this.creationParams,
    required this.callbacksHandler,
    required this.javascriptChannelRegistry,
    this.onWebViewPlatformCreated,
  }) : super(key: key);

  final int initialWidth;
  final int initialHeight;

  /// Initial parameters used to setup the WebView.
  ///
  /// Most of the [WebView](https://pub.dev/documentation/webview_flutter/3.0.4/webview_flutter/WebView-class.html)'s
  /// properties (e.g. [WebView.initialUrl](https://pub.dev/documentation/webview_flutter/3.0.4/webview_flutter/WebView/initialUrl.html),
  /// [WebView.userAgent](https://pub.dev/documentation/webview_flutter/3.0.4/webview_flutter/WebView/userAgent.html))
  /// are passed through to this backing property [WebViewLinuxWidget.creationParams].
  ///
  /// On Linux, the behavior of some properties of [WebView](https://pub.dev/documentation/webview_flutter/3.0.4/webview_flutter/WebView-class.html)
  /// differ from that on Android/iOS due to limitations of the underlying
  /// browser or we have not yet implemented some features. The support status
  /// of each property is managed in README.md.
  ///
  /// As you can see from the description of [CreationParams], the properties
  /// directly under [CreationParams] are only effective when the
  /// [WebView] is first created, and the properties under
  /// [CreationParams.webSettings] are values that can be dynamically updated by
  /// [WebViewPlatformController.updateSettings]. Note that `userAgent` is
  /// present in both of them.
  final CreationParams creationParams;

  final WebViewPlatformCallbacksHandler callbacksHandler;
  final JavascriptChannelRegistry javascriptChannelRegistry;
  final WebViewPlatformCreatedCallback? onWebViewPlatformCreated;

  @override
  State<WebViewLinuxWidget> createState() => _WebViewLinuxWidgetState();
}

class _WebViewLinuxWidgetState extends State<WebViewLinuxWidget> {
  static const kTextureUninitialized = 0;

  late final WebViewLinuxPlatformController _controller;
  int _textureId = kTextureUninitialized;

  /// The [PointerEvent.buttons] saved for future comparison of differences.
  int _prevButtons = 0;

  final FocusNode _focusNode = FocusNode();

  @override
  void initState() {
    super.initState();
    _controller = WebViewLinuxPlatformController(
      creationParams: widget.creationParams,
      callbacksHandler: widget.callbacksHandler,
      javascriptChannelRegistry: widget.javascriptChannelRegistry,
    );

    WebViewCookieManagerPlatform.instance ??= WebViewLinuxCookieManager();
    _initController();
  }

  void _initController() async {
    // set all the given cookies.
    await Future.forEach(widget.creationParams.cookies,
        WebViewCookieManagerPlatform.instance!.setCookie);

    if (!mounted) {
      // this widget is disposed
      return;
    }

    int? textureId = await _controller._create(
        widget.creationParams.initialUrl,
        widget.creationParams.backgroundColor,
        widget.initialWidth,
        widget.initialHeight);

    if (!mounted) {
      // this widget was disposed during WebView creation
      _controller._dispose();
      return;
    }

    setState(() {
      _textureId = textureId ?? kTextureUninitialized;
    });

    if (widget.onWebViewPlatformCreated != null) {
      widget.onWebViewPlatformCreated!(_controller);
    }

    // resize the browser when the widget is rendered
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!mounted) return;
      if (context.size == null) {
        log.info('context.size is null');
        return;
      }

      log.fine('resize: context.size: ${context.size}');
      _controller._resize(
          context.size!.width.toInt(), context.size!.height.toInt());
    });
  }

  @override
  void dispose() {
    _controller._dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    if (_textureId == kTextureUninitialized) {
      return const SizedBox.expand();
    }

    final Widget texture = Texture(textureId: _textureId);

    Widget webviewInputHandler(Widget screen) {
      return KeyboardListener(
        focusNode: _focusNode,
        autofocus: true, // necessary
        onKeyEvent: _onKeyEvent,
        child: MouseRegion(
          onExit: _onExit,
          child: _SerialTapGestureDetector(
            onSerialTapDown: _onSerialTapDown,
            child: Listener(
              onPointerDown: (PointerDownEvent event) {
                log.fine(
                    'onPointerDown: ${event.toString()}, buttons=${event.buttons}');
              },
              onPointerUp: _onPointerUp,
              onPointerSignal: _onPointerSignal,
              onPointerMove: _onPointerMove,
              onPointerHover: _onPointerHover,
              child: screen,
            ),
          ),
        ),
      );
    }

    bool onSizeChangedLayoutNotification(
        SizeChangedLayoutNotification notification) {
      if (context.size == null) {
        log.fine('context.size is null');
        return true;
      }
      log.fine('resize: ${context.size}');
      _controller._resize(
          context.size!.width.toInt(), context.size!.height.toInt());
      return true;
    }

    Widget resizeNotifier = NotificationListener<SizeChangedLayoutNotification>(
      onNotification: onSizeChangedLayoutNotification,
      child: SizeChangedLayoutNotifier(child: webviewInputHandler(texture)),
    );

    return resizeNotifier;
  }

  void _onKeyEvent(KeyEvent event) {
    log.fine(
        '${event is KeyUpEvent ? '⬆' : event is KeyDownEvent ? '⬇' : 'REPEAT'} KeyEvent: ${event.runtimeType}\n'
        '  LogicalKey: ${event.logicalKey}\n'
        '  PhysicalKey: ${event.physicalKey}');

    // final logicalKeysPressed = HardwareKeyboard.instance.logicalKeysPressed;
    // final phisicalKeysPressed = HardwareKeyboard.instance.physicalKeysPressed;
    // final lockModesEnabled = HardwareKeyboard.instance.lockModesEnabled;
    // final rawKeysPressed = RawKeyboard.instance.keysPressed;
    // final rawPhysicalKeysPressed = RawKeyboard.instance.physicalKeysPressed;

    int getCharCode(String? character) {
      if (character == null || character.isEmpty) return 0;
      if (character.length == 1) {
        return character[0].codeUnits[0];
      }
      return 0;
    }

    final KeyboardCode windowsKeyCode =
        getWindowsKeyCodeWithoutLocation(getWindowsKeyCode(event.logicalKey));
    final int nativeKeyCode = getNativeKeyCode(event.physicalKey.usbHidUsage);
    final int modifiers = _getModifiers(_prevButtons) |
        (_isNumpadKey(event.logicalKey)
            ? CefEventFlags.EVENTFLAG_IS_KEY_PAD.value
            : 0);
    final int unmodifiedCharacter = (windowsKeyCode == KeyboardCode.VKEY_RETURN
        ? 0x0d /* '\r' */
        : getCharCode(event.character));

    final bool isSystemKey =
        modifiers & CefEventFlags.EVENTFLAG_ALT_DOWN.value != 0;
    final bool isShiftDown =
        modifiers & CefEventFlags.EVENTFLAG_SHIFT_DOWN.value != 0;
    final bool isControlDown =
        modifiers & CefEventFlags.EVENTFLAG_CONTROL_DOWN.value != 0;
    // If ctrl key is pressed down, then control character shall be input.
    final int character = isControlDown
        ? getControlCharacter(windowsKeyCode, isShiftDown)
        : unmodifiedCharacter;

    if (event is KeyDownEvent || event is KeyRepeatEvent) {
      log.fine('KeyDownEvent: $event');

      final int keyDownType = event is KeyDownEvent
          ? CefKeyEventType.KEYEVENT_RAWKEYDOWN.value
          : CefKeyEventType.KEYEVENT_KEYDOWN.value;

      _controller._sendKey(keyDownType, modifiers, windowsKeyCode.code,
          nativeKeyCode, isSystemKey, character, unmodifiedCharacter);

      if (!isControlDown && character != 0) {
        _controller._sendKey(
            CefKeyEventType.KEYEVENT_CHAR.value,
            modifiers,
            windowsKeyCode.code,
            nativeKeyCode,
            isSystemKey,
            character,
            unmodifiedCharacter);
      }
    } else if (event is KeyUpEvent) {
      log.fine('KeyUpEvent: $event');
      _controller._sendKey(
          CefKeyEventType.KEYEVENT_KEYUP.value,
          modifiers,
          windowsKeyCode.code,
          nativeKeyCode,
          isSystemKey,
          character,
          unmodifiedCharacter);
    }
  }

  void _onExit(PointerExitEvent event) {
    log.finer('onExit: ${event.toString()}, buttons=${event.buttons}');
    int modifiers = _getModifiers(event.buttons);
    _controller._sendMouseMove(event.localPosition.dx.toInt(),
        event.localPosition.dy.toInt(), modifiers, true);
  }

  void _onSerialTapDown(SerialTapDownDetails details) {
    log.fine('onSerialTapDown:\n'
        '  details: ${details.toString()}\n'
        '  kind: ${details.kind}\n'
        '  buttons: ${details.buttons}\n'
        '  count: ${details.count}');

    if (details.kind != PointerDeviceKind.mouse) {
      return;
    }

    _focusNode.requestFocus();

    // NOTE: click_count is not allowed to be greater than 3
    int count = details.count > 3 ? 3 : details.count;
    int modifiers = _getModifiers(details.buttons);
    List<CefMouseButtonType> buttons =
        _getButtonsStateChangedToDown(details.buttons, _prevButtons);

    for (CefMouseButtonType button in buttons) {
      _controller._sendMouseClick(
          details.localPosition.dx.toInt(),
          details.localPosition.dy.toInt(),
          modifiers,
          button.value,
          false,
          count);
    }

    _prevButtons = details.buttons; // save it for mouseUp
  }

  void _onPointerMove(PointerMoveEvent event) {
    log.fine('onPointerMove:\n'
        '  details: ${event.toString()}\n'
        '  kind: ${event.kind}\n'
        '  buttons: ${event.buttons}');

    int modifiers = _getModifiers(event.buttons);

    // If other mouse buttons are already pressed, a mouse up/down event is
    // detected by Listener.onPointerMove, not by Listener.onPointerUp/Down
    if (event.buttons != _prevButtons) {
      List<CefMouseButtonType> buttonsStateChangedToDown =
          _getButtonsStateChangedToDown(event.buttons, _prevButtons);
      List<CefMouseButtonType> buttonsStateChangedToUp =
          _getButtonsStateChangedToUp(event.buttons, _prevButtons);

      for (CefMouseButtonType button in buttonsStateChangedToDown) {
        _controller._sendMouseClick(
            event.localPosition.dx.toInt(),
            event.localPosition.dy.toInt(),
            modifiers,
            button.value,
            false,
            // TODO(Ino): support for dbclick when other mouse buttons are
            // already down
            1);
      }

      for (CefMouseButtonType button in buttonsStateChangedToUp) {
        _controller._sendMouseClick(event.localPosition.dx.toInt(),
            event.localPosition.dy.toInt(), modifiers, button.value, true, 1);
      }
    }

    _controller._sendMouseMove(event.localPosition.dx.toInt(),
        event.localPosition.dy.toInt(), modifiers, false);

    _prevButtons = event.buttons; // save it for mouseUp
  }

  void _onPointerUp(PointerUpEvent event) {
    log.fine('onPointerUp:\n'
        '  details: ${event.toString()}\n'
        '  kind: ${event.kind}\n'
        '  buttons: ${event.buttons}');

    List<CefMouseButtonType> buttons =
        _getButtonsStateChangedToUp(event.buttons, _prevButtons);
    int modifiers = _getModifiers(event.buttons);

    for (CefMouseButtonType button in buttons) {
      _controller._sendMouseClick(event.localPosition.dx.toInt(),
          event.localPosition.dy.toInt(), modifiers, button.value, true, 1);
    }

    _prevButtons = event.buttons;
  }

  void _onPointerHover(PointerHoverEvent event) {
    log.finer('onPointerHover:\n'
        '  details: ${event.toString()}\n'
        '  kind: ${event.kind}\n'
        '  buttons: ${event.buttons}');
    int modifiers = _getModifiers(event.buttons);
    _controller._sendMouseMove(event.localPosition.dx.toInt(),
        event.localPosition.dy.toInt(), modifiers, false);

    _prevButtons = event.buttons; // save it for mouseUp
  }

  void _onPointerSignal(PointerSignalEvent event) {
    log.finer(
        'onPointerSignal: delta.dx=${event.delta.dx}, delta.dy=${event.delta.dy}');
    if (event is PointerScrollEvent) {
      // consume a scroll event on the webview
      GestureBinding.instance.pointerSignalResolver
          .register(event, (PointerSignalEvent event) {});

      int modifiers = _getModifiers(event.buttons);

      _controller._sendMouseWheel(
          event.localPosition.dx.toInt(),
          event.localPosition.dy.toInt(),
          modifiers,
          -event.scrollDelta.dx.toInt(),
          -event.scrollDelta.dy.toInt());

      _prevButtons = event.buttons; // save it for mouseUp
    }
  }
}

List<CefMouseButtonType> _getButtonsStateChangedToDown(
        int currentButtons, int prevButtons) =>
    _getButtonsStateChanged(true, currentButtons, prevButtons);

List<CefMouseButtonType> _getButtonsStateChangedToUp(
        int currentButtons, int prevButtons) =>
    _getButtonsStateChanged(false, currentButtons, prevButtons);

List<CefMouseButtonType> _getButtonsStateChanged(
    bool down, int currentButtons, int prevButtons) {
  bool isLeftButtonPressed = currentButtons & kPrimaryMouseButton != 0;
  bool isMiddleButtonPressed = currentButtons & kMiddleMouseButton != 0;
  bool isSecondaryButtonPressed = currentButtons & kSecondaryMouseButton != 0;

  bool isLeftButtonPrevPressed = prevButtons & kPrimaryMouseButton != 0;
  bool isMiddleButtonPrevPressed = prevButtons & kMiddleMouseButton != 0;
  bool isSecondaryButtonPrevPressed = prevButtons & kSecondaryMouseButton != 0;

  List<CefMouseButtonType> buttons = [];
  if (down) {
    if (!isLeftButtonPrevPressed && isLeftButtonPressed) {
      buttons.add(CefMouseButtonType.MBT_LEFT);
    }
    if (!isMiddleButtonPrevPressed && isMiddleButtonPressed) {
      buttons.add(CefMouseButtonType.MBT_MIDDLE);
    }
    if (!isSecondaryButtonPrevPressed && isSecondaryButtonPressed) {
      buttons.add(CefMouseButtonType.MBT_RIGHT);
    }
    log.finer("buttondown buttons: $buttons");
  } else {
    if (isLeftButtonPrevPressed && !isLeftButtonPressed) {
      buttons.add(CefMouseButtonType.MBT_LEFT);
    }
    if (isMiddleButtonPrevPressed && !isMiddleButtonPressed) {
      buttons.add(CefMouseButtonType.MBT_MIDDLE);
    }
    if (isSecondaryButtonPrevPressed && !isSecondaryButtonPressed) {
      buttons.add(CefMouseButtonType.MBT_RIGHT);
    }
    log.finer("buttonup buttons: $buttons");
  }
  return buttons;
}

/// Helper function to get CefEventFlags bits from [PointerEvent.buttons]
int _getModifiers(int mouseButtons) {
  int modifiers = 0;
  final isPrimaryButtonPressed = mouseButtons & kPrimaryButton != 0;
  final isSecondaryButtonPressed = mouseButtons & kSecondaryButton != 0;
  final isMiddleButtonPressed = mouseButtons & kMiddleMouseButton != 0;
  final bool isShiftPressed = HardwareKeyboard.instance.logicalKeysPressed.any(
      (logicalKey) =>
          logicalKey == LogicalKeyboardKey.shift ||
          logicalKey == LogicalKeyboardKey.shiftLeft ||
          logicalKey == LogicalKeyboardKey.shiftRight);
  final bool isCapsLockOn = HardwareKeyboard.instance.lockModesEnabled
      .any((lockKey) => lockKey.logicalKey == LogicalKeyboardKey.capsLock);
  final bool isNumLockOn = HardwareKeyboard.instance.lockModesEnabled
      .any((lockKey) => lockKey.logicalKey == LogicalKeyboardKey.numLock);
  final bool isControlPressed = HardwareKeyboard.instance.logicalKeysPressed
      .any((logicalKey) =>
          logicalKey == LogicalKeyboardKey.control ||
          logicalKey == LogicalKeyboardKey.controlLeft ||
          logicalKey == LogicalKeyboardKey.controlRight);
  final bool isAltPressed = HardwareKeyboard.instance.logicalKeysPressed.any(
      (logicalKey) =>
          logicalKey == LogicalKeyboardKey.alt ||
          logicalKey == LogicalKeyboardKey.altLeft ||
          logicalKey == LogicalKeyboardKey.altRight);
  final bool isMetaPressed = HardwareKeyboard.instance.logicalKeysPressed.any(
      (logicalKey) =>
          logicalKey == LogicalKeyboardKey.superKey ||
          logicalKey == LogicalKeyboardKey.meta ||
          logicalKey == LogicalKeyboardKey.metaLeft ||
          logicalKey == LogicalKeyboardKey.metaRight);

  if (isPrimaryButtonPressed) {
    modifiers |= CefEventFlags.EVENTFLAG_LEFT_MOUSE_BUTTON.value;
  }
  if (isSecondaryButtonPressed) {
    modifiers |= CefEventFlags.EVENTFLAG_RIGHT_MOUSE_BUTTON.value;
  }
  if (isMiddleButtonPressed) {
    modifiers |= CefEventFlags.EVENTFLAG_MIDDLE_MOUSE_BUTTON.value;
  }
  if (isShiftPressed) {
    modifiers |= CefEventFlags.EVENTFLAG_SHIFT_DOWN.value;
  }
  if (isCapsLockOn) {
    modifiers |= CefEventFlags.EVENTFLAG_CAPS_LOCK_ON.value;
  }
  if (isNumLockOn) {
    modifiers |= CefEventFlags.EVENTFLAG_NUM_LOCK_ON.value;
  }
  if (isControlPressed) {
    modifiers |= CefEventFlags.EVENTFLAG_CONTROL_DOWN.value;
  }
  if (isAltPressed) {
    modifiers |= CefEventFlags.EVENTFLAG_ALT_DOWN.value;
  }
  if (isMetaPressed) {
    modifiers |= CefEventFlags.EVENTFLAG_COMMAND_DOWN.value;
  }
  return modifiers;
}

/// Helper function corresponding to the following logic in
/// BrowserWindowOsrGtk::KeyEvent in cefclient's browser_window_osr_gtk.cc:
/// ```
/// (event->keyval >= GDK_KP_Space && event->keyval <= GDK_KP_9)
/// ```
bool _isNumpadKey(LogicalKeyboardKey key) {
  // TODO(Ino): PhysicalKeyboardKey is better?

  return (
      // GDK_KP_Space 0xff80
      // GDK_KP_Tab 0xff89

      // GDK_KP_Enter 0xff8d
      key == LogicalKeyboardKey.numpadEnter ||

          // GDK_KP_F1 0xff91
          // GDK_KP_F2 0xff92
          // GDK_KP_F3 0xff93
          // GDK_KP_F4 0xff94
          // GDK_KP_Home 0xff95
          // GDK_KP_Left 0xff96
          // GDK_KP_Up 0xff97
          // GDK_KP_Right 0xff98
          // GDK_KP_Down 0xff99
          // GDK_KP_Prior 0xff9a
          // GDK_KP_Page_Up 0xff9a
          // GDK_KP_Next 0xff9b
          // GDK_KP_Page_Down 0xff9b
          // GDK_KP_End 0xff9c
          // GDK_KP_Begin 0xff9d
          // GDK_KP_Insert 0xff9e
          // GDK_KP_Delete 0xff9f

          // GDK_KP_Equal 0xffbd
          key == LogicalKeyboardKey.numpadEqual ||
          // GDK_KP_Multiply 0xffaa
          key == LogicalKeyboardKey.numpadMultiply ||
          // GDK_KP_Add 0xffab
          key == LogicalKeyboardKey.numpadAdd ||
          // GDK_KP_Separator 0xffac
          key == LogicalKeyboardKey.numpadEnter ||
          // GDK_KP_Subtract 0xffad
          key == LogicalKeyboardKey.numpadSubtract ||
          // GDK_KP_Decimal 0xffae
          key == LogicalKeyboardKey.numpadDecimal ||
          // GDK_KP_Divide 0xffaf
          key == LogicalKeyboardKey.numpadDivide ||
          // GDK_KP_0 0xffb0
          key == LogicalKeyboardKey.numpad0 ||
          // GDK_KP_1 0xffb1
          key == LogicalKeyboardKey.numpad1 ||
          // GDK_KP_2 0xffb2
          key == LogicalKeyboardKey.numpad2 ||
          // GDK_KP_3 0xffb3
          key == LogicalKeyboardKey.numpad3 ||
          // GDK_KP_4 0xffb4
          key == LogicalKeyboardKey.numpad4 ||
          // GDK_KP_5 0xffb5
          key == LogicalKeyboardKey.numpad5 ||
          // GDK_KP_6 0xffb6
          key == LogicalKeyboardKey.numpad6 ||
          // GDK_KP_7 0xffb7
          key == LogicalKeyboardKey.numpad7 ||
          // GDK_KP_8 0xffb8
          key == LogicalKeyboardKey.numpad8 ||
          // GDK_KP_9 0xffb9
          key == LogicalKeyboardKey.numpad9);
}

class _JavascriptResult {
  final bool wasExecuted;
  final bool isException;
  final String result;
  final bool isUndefined;

  _JavascriptResult({
    required this.wasExecuted,
    required this.isException,
    required this.result,
    required this.isUndefined,
  });
}

/// To operate in the int32 range for CefProcessMessage to carry int on the C++ side.
typedef _JsRunId = int;

class WebViewLinuxPlatformController extends WebViewPlatformController {
  static final InstanceManager instanceManager = InstanceManager.instance;

  /// Completers to wait for javascript execution to complete.
  static final _jsCompleters = <_JsRunId, Completer<_JavascriptResult>>{};

  /// JS execution ID, rotates from 0 to int32 max
  static _JsRunId _nextJsRunId = 0;
  final int _kInt32Max = (1 << 31) - 1; // 2147483647

  /// An internal property that users should not use. A handler that handles
  /// method calls coming from the native side.
  static Future<bool?> onMethodCall(MethodCall call) async {
    /// Get the webview instance corresponding to webviewId
    WebViewLinuxPlatformController _getControllerByWebviewId(int webviewId) {
      final controller = instanceManager.getInstance(webviewId)
          as WebViewLinuxPlatformController?;
      if (controller == null) {
        throw 'WebView with webviewId=$webviewId is not found';
      }
      return controller;
    }

    switch (call.method) {
      case 'javascriptChannelMessage':
        // javascriptChannels have not yet been implemented.
        // TODO(Ino): implement javascriptChannels
        throw UnimplementedError('javascriptChannelMessage is not implemented');
      case 'navigationRequest':
        // TODO(Ino): implement navigationDelegate
        // WebViewLinuxPlatformController controller = _getController(call);
        // return await controller.callbacksHandler.onNavigationRequest(
        //   url: call.arguments['url'] as String,
        //   isForMainFrame: call.arguments['isForMainFrame'] as bool,
        // );
        throw UnimplementedError('navigationRequest is not implemented');
      case 'javascriptResult':
        // The result of _runJavascriptInternal comes here.
        final int jsRunId = call.arguments['jsRunId'] as int;
        final bool wasExecuted = call.arguments['wasExecuted'] as bool;
        final bool isException = call.arguments['isException'] as bool;
        final String result = call.arguments['result'] as String;
        final bool isUndefined = call.arguments['isUndefined'] as bool;

        log.fine('javascriptResult received!:\n'
            '  jsRunId: $jsRunId\n'
            '  wasExecuted: $wasExecuted\n'
            '  isException: $isException\n'
            '  result: $result\n'
            '  isUndefined: $isUndefined');

        // complete the waiting completer
        if (_jsCompleters[jsRunId] != null) {
          _jsCompleters[jsRunId]!.complete(_JavascriptResult(
              wasExecuted: wasExecuted,
              isException: isException,
              result: result,
              isUndefined: isUndefined));
          _jsCompleters.remove(jsRunId);
        } else {
          log.warning('The js completer for jsRunId=$jsRunId does not exist');
        }
        return true;
      case 'onPageFinished':
        WebViewLinuxPlatformController controller =
            _getControllerByWebviewId(call.arguments['webviewId'] as int);
        controller.callbacksHandler
            .onPageFinished(call.arguments['url'] as String);
        return null;
      case 'onProgress':
        WebViewLinuxPlatformController controller =
            _getControllerByWebviewId(call.arguments['webviewId'] as int);
        controller.callbacksHandler
            .onProgress(call.arguments['progress'] as int);
        return null;
      case 'onPageStarted':
        WebViewLinuxPlatformController controller =
            _getControllerByWebviewId(call.arguments['webviewId'] as int);
        controller.callbacksHandler
            .onPageStarted(call.arguments['url'] as String);
        return null;
      case 'onWebResourceError':
        WebViewLinuxPlatformController controller =
            _getControllerByWebviewId(call.arguments['webviewId']);
        controller.callbacksHandler.onWebResourceError(
          WebResourceError(
            errorCode: call.arguments['errorCode'] as int,
            description: call.arguments['description'] as String,
            failingUrl: call.arguments['failingUrl'] as String?,
            // TODO(Ino): support domain
            domain: call.arguments['domain'] as String?,
            // TODO(Ino): support errorType
            errorType: call.arguments['errorType'] == null
                ? null
                : WebResourceErrorType.values.firstWhere(
                    (WebResourceErrorType type) {
                      return type.toString() ==
                          '$WebResourceErrorType.${call.arguments['errorType']}';
                    },
                  ),
          ),
        );
        return null;
    }

    throw MissingPluginException(
        '${call.method} was invoked but has no handler');
  }

  WebViewLinuxPlatformController(
      {required this.callbacksHandler,
      required this.javascriptChannelRegistry,
      required CreationParams creationParams})
      // NOTE: This super constructor will be removed in the future. See
      // the comment in the WebViewPlatformController class.
      : super(callbacksHandler);

  final WebViewPlatformCallbacksHandler callbacksHandler;
  final JavascriptChannelRegistry javascriptChannelRegistry;
  int? _webviewId;

  /// create a browser.
  Future<int?> _create(String? initialUrl, Color? backgroundColor,
      int initialWidth, int initialHeight) async {
    final int? webviewId = instanceManager.tryAddInstance(this);
    if (webviewId != null) {
      _webviewId = webviewId;
      log.fine('createBrowser called. webviewId: $webviewId');
      final int? textureId = await (await LinuxWebViewPlugin.channel)
          .invokeMethod('createBrowser', <String, dynamic>{
        'webviewId': webviewId,
        'initialUrl': initialUrl ?? '',
        'backgroundColor': (backgroundColor != null)
            ? Uint8List.fromList([
                backgroundColor.alpha,
                backgroundColor.red,
                backgroundColor.green,
                backgroundColor.blue
              ])
            : Uint8List.fromList([]),
        'initialWidth': initialWidth,
        'initialHeight': initialHeight,
      });
      log.fine('return from createBrowser: textureId=$textureId');

      return textureId;
    }
    return null;
  }

  /// close the browser
  Future<void> _dispose() async {
    final int? webviewId = instanceManager.getInstanceId(this);
    log.fine('disposeBrowser called. webviewId: $webviewId');
    if (webviewId != null) {
      await (await LinuxWebViewPlugin.channel)
          .invokeMethod('disposeBrowser', <String, dynamic>{
        'webviewId': webviewId,
      });
      instanceManager.removeInstance(this);
    }
  }

  /// Send a MouseMove event to the browser.
  Future<void> _sendMouseMove(
      int x, int y, int modifiers, bool mouseLeave) async {
    final int? webviewId = instanceManager.getInstanceId(this)!;
    if (webviewId == null) {
      throw 'Failed to get the webview instance';
    }
    await (await LinuxWebViewPlugin.channel)
        .invokeMethod('sendMouseMove', <String, dynamic>{
      'webviewId': webviewId,
      'x': x,
      'y': y,
      'modifiers': modifiers,
      'mouseLeave': mouseLeave,
    });
  }

  /// Send a MouseWheel event to the browser.
  Future<void> _sendMouseWheel(
      int x, int y, int modifiers, int deltaX, int deltaY) async {
    final int? webviewId = instanceManager.getInstanceId(this);
    if (webviewId == null) {
      throw 'Failed to get the webview instance';
    }
    await (await LinuxWebViewPlugin.channel)
        .invokeMethod('sendMouseWheel', <String, dynamic>{
      'webviewId': webviewId,
      'x': x,
      'y': y,
      'modifiers': modifiers,
      'deltaX': deltaX,
      'deltaY': deltaY,
    });
  }

  /// Send a MouseClick event to the browser.
  Future<void> _sendMouseClick(int x, int y, int modifiers, int mouseButtonType,
      bool mouseUp, int clickCount) async {
    final int? webviewId = instanceManager.getInstanceId(this);
    if (webviewId == null) {
      throw 'Failed to get the webview instance';
    }
    await (await LinuxWebViewPlugin.channel)
        .invokeMethod('sendMouseClick', <String, dynamic>{
      'webviewId': webviewId,
      'x': x,
      'y': y,
      'modifiers': modifiers,
      'mouseButtonType': mouseButtonType,
      'mouseUp': mouseUp,
      'clickCount': clickCount,
    });
  }

  /// Send a Key event to the browser.
  Future<void> _sendKey(
      int keyEventType,
      int modifiers,
      int windowsKeyCode,
      int nativeKeyCode,
      bool isSystemKey,
      int character,
      int unmodifiedCharacter) async {
    final int? webviewId = instanceManager.getInstanceId(this);
    if (webviewId == null) {
      throw 'Failed to get the webview instance';
    }
    await (await LinuxWebViewPlugin.channel)
        .invokeMethod('sendKey', <String, dynamic>{
      'webviewId': webviewId,
      'keyEventType': keyEventType,
      'modifiers': modifiers,
      'windowsKeyCode': windowsKeyCode,
      'nativeKeyCode': nativeKeyCode,
      'isSystemKey': isSystemKey,
      'character': character,
      'unmodifiedCharacter': unmodifiedCharacter,
    });
  }

  /// Request a browser resolution change.
  Future<void> _resize(int width, int height) async {
    final int? webviewId = instanceManager.getInstanceId(this);
    if (webviewId == null) {
      throw 'Failed to get the webview instance';
    }
    await (await LinuxWebViewPlugin.channel)
        .invokeMethod('resize', <String, dynamic>{
      'webviewId': webviewId,
      'width': width,
      'height': height,
    });
  }

  /// See [WebViewController.loadFile](https://pub.dev/documentation/webview_flutter/3.0.4/webview_flutter/WebViewController/loadFile.html)
  /// or [WebViewPlatformController.loadFile].
  ///
  /// ## Known bug (?)
  ///
  /// The timing of when Future is resolved is different from Android/iOS.
  /// Immediately after this method is resolved, the new URL cannot yet be
  /// obtained with [currentUrl].
  ///
  /// TODO(Ino): Immediately reflect the new URL in [currentUrl].
  @override
  Future<void> loadFile(
    String absoluteFilePath,
  ) async {
    final int? webviewId = instanceManager.getInstanceId(this);
    if (webviewId == null) {
      throw 'Failed to get the webview instance';
    }
    final String url = absoluteFilePath.startsWith('file://')
        ? absoluteFilePath
        : 'file://$absoluteFilePath';
    await (await LinuxWebViewPlugin.channel)
        .invokeMethod('loadUrl', <String, dynamic>{
      'webviewId': webviewId,
      'url': url,
    });
  }

  /// See [WebViewController.loadFlutterAsset](https://pub.dev/documentation/webview_flutter/3.0.4/webview_flutter/WebViewController/loadFlutterAsset.html)
  /// or [WebViewPlatformController.loadFlutterAsset].
  ///
  /// ## Known bug (?)
  ///
  /// The timing of when Future is resolved is different from Android/iOS.
  /// Immediately after this method is resolved, the new URL cannot yet be
  /// obtained with [currentUrl].
  ///
  /// TODO(Ino): Immediately reflect the new URL in [currentUrl].
  @override
  Future<void> loadFlutterAsset(
    String key,
  ) async {
    final int? webviewId = instanceManager.getInstanceId(this);
    if (webviewId == null) {
      throw 'Failed to get the webview instance';
    }

    final String assetUri = "file://" +
        p.dirname(Platform.resolvedExecutable) +
        "/data/flutter_assets/" +
        key;

    await (await LinuxWebViewPlugin.channel)
        .invokeMethod('loadUrl', <String, dynamic>{
      'webviewId': webviewId,
      'url': assetUri,
    });
  }

  /// See [WebViewController.loadHtmlString](https://pub.dev/documentation/webview_flutter/3.0.4/webview_flutter/WebViewController/loadHtmlString.html)
  /// or [WebViewPlatformController.loadHtmlString].
  ///
  /// ## Limitations on Linux
  ///
  /// [baseUrl] is not supported because the underlying browser does not support
  /// baseUrl.
  ///
  /// ## Known bug (?)
  ///
  /// The timing of when Future is resolved is different from Android/iOS.
  /// Immediately after this method is resolved, the new URL cannot yet be
  /// obtained with [currentUrl].
  ///
  /// TODO(Ino): Immediately reflect the new URL in [currentUrl].
  @override
  Future<void> loadHtmlString(
    String html, {
    String? baseUrl,
  }) async {
    final int? webviewId = instanceManager.getInstanceId(this);
    if (webviewId == null) {
      throw 'Failed to get the webview instance';
    }
    final data = base64Encode(utf8.encode(html));
    final dataUri = "data:text/html;base64,$data";

    await (await LinuxWebViewPlugin.channel)
        .invokeMethod('loadUrl', <String, dynamic>{
      'webviewId': webviewId,
      'url': dataUri,
    });
  }

  /// See [WebViewController.loadUrl](https://pub.dev/documentation/webview_flutter/3.0.4/webview_flutter/WebViewController/loadUrl.html)
  /// or [WebViewPlatformController.loadUrl].
  ///
  /// ## Limitations on Linux
  ///
  /// Requests with [headers] cannot be made for an origin different from the
  /// current page. You must first navigate to the request origin (scheme + domain)
  /// using some other mechanism ([loadUrl] without headers, link click, etc).
  ///
  /// This is due to CEF (the underlying browser) limitations.
  ///
  /// [loadUrl] calls CefFrame::LoadURL or CefFrame::LoadRequest on the native side.
  /// CefFrame::LoadURL is used when no header is specified, and CefFrame::LoadRequest
  /// is used when a header is specified. However, the LoadRequest method will fail
  /// the LoadRequest method will fail with a "bad IPC message" error unless you
  /// first navigate to the request origin.
  ///
  /// For more detail:
  /// https://bitbucket.org/chromiumembedded/cef/wiki/GeneralUsage#markdown-header-custom-requests
  ///
  /// ## Known bug
  ///
  /// The timing of when Future is resolved is different from that expected on
  /// Android/iOS (see the integration test).
  /// When [loadUrl] is resolved, the new URL is expected to be available in
  /// [currentUrl], but the current implementation does not do so.
  ///
  /// TODO(Ino): Fix [loadUrl] so that the expected URL is available in [currentUrl].
  @override
  Future<void> loadUrl(
    String url,
    Map<String, String>? headers,
  ) async {
    final int? webviewId = instanceManager.getInstanceId(this);
    if (webviewId == null) {
      throw 'Failed to get the webview instance';
    }

    if (headers == null || headers.isEmpty) {
      // no headers
      await (await LinuxWebViewPlugin.channel)
          .invokeMethod('loadUrl', <String, dynamic>{
        'webviewId': webviewId,
        'url': url,
      });
    } else {
      // with headers
      await loadRequest(WebViewRequest(
        uri: Uri.parse(url),
        method: WebViewRequestMethod.get,
        headers: headers,
      ));
    }
  }

  /// See [WebViewController.loadRequest](https://pub.dev/documentation/webview_flutter/3.0.4/webview_flutter/WebViewController/loadRequest.html)
  /// or [WebViewPlatformController.loadRequest].
  ///
  /// ## Limitations on Linux
  ///
  /// Requests cannot be made for an origin different from the current page.
  /// You must first navigate to the request origin (scheme + domain) using
  /// some other mechanism ([loadUrl] without headers, link click, etc).
  ///
  /// This is due to CEF (the underlying browser) limitations.
  ///
  /// [loadRequest] calls CefFrame::LoadRequest on the native side.
  /// the LoadRequest method will fail with a "bad IPC message" error unless you
  /// first navigate to the request origin.
  ///
  /// For more detail:
  /// https://bitbucket.org/chromiumembedded/cef/wiki/GeneralUsage#markdown-header-custom-requests
  ///
  /// ## Known bug (?)
  ///
  /// The timing of when Future is resolved is different from Android/iOS.
  /// Immediately after this method is resolved, the new URL cannot yet be
  /// obtained with [currentUrl].
  ///
  /// TODO(Ino): Immediately reflect the new URL in [currentUrl].
  @override
  Future<void> loadRequest(
    WebViewRequest request,
  ) async {
    final int? webviewId = instanceManager.getInstanceId(this);
    if (webviewId == null) {
      throw 'Failed to get the webview instance';
    }
    final String method = request.method.serialize().toUpperCase();
    await (await LinuxWebViewPlugin.channel)
        .invokeMethod('loadRequest', <String, dynamic>{
      'webviewId': webviewId,
      'uri': request.uri.toString(),
      'method': method,
      'headers': request.headers,
      'body': request.body ?? Uint8List(0),
    });
  }

  @override
  Future<void> updateSettings(WebSettings setting) async {
    await Future.wait(<Future<void>>[
      // TODO(Ino): implement [hasNavigationDelegate]
      // if (setting.hasNavigationDelegate != null)
      //   _setHasNavigationDelegate(setting.hasNavigationDelegate!),

      // TODO(Ino): implement [hasProgressTracking]
      // if (setting.hasProgressTracking != null)
      //   _setHasProgressTracking(setting.hasProgressTracking!),
    ]);
  }

  @override
  Future<String?> currentUrl() async {
    final int? webviewId = instanceManager.getInstanceId(this);
    if (webviewId == null) {
      throw 'Failed to get the webview instance';
    }
    String? result = await (await LinuxWebViewPlugin.channel)
        .invokeMethod('currentUrl', <String, dynamic>{
      'webviewId': webviewId,
    });
    return result;
  }

  @override
  Future<bool> canGoBack() async {
    final int? webviewId = instanceManager.getInstanceId(this);
    if (webviewId == null) {
      throw 'Failed to get the webview instance';
    }
    bool? result = await (await LinuxWebViewPlugin.channel)
        .invokeMethod('canGoBack', <String, dynamic>{
      'webviewId': webviewId,
    });
    return result!;
  }

  @override
  Future<bool> canGoForward() async {
    final int? webviewId = instanceManager.getInstanceId(this);
    if (webviewId == null) {
      throw 'Failed to get the webview instance';
    }
    bool? result = await (await LinuxWebViewPlugin.channel)
        .invokeMethod('canGoForward', <String, dynamic>{
      'webviewId': webviewId,
    });
    return result!;
  }

  @override
  Future<void> goBack() async {
    final int? webviewId = instanceManager.getInstanceId(this);
    if (webviewId == null) {
      throw 'Failed to get the webview instance';
    }
    await (await LinuxWebViewPlugin.channel)
        .invokeMethod('goBack', <String, dynamic>{
      'webviewId': webviewId,
    });
  }

  @override
  Future<void> goForward() async {
    final int? webviewId = instanceManager.getInstanceId(this);
    if (webviewId == null) {
      throw 'Failed to get the webview instance';
    }
    await (await LinuxWebViewPlugin.channel)
        .invokeMethod('goForward', <String, dynamic>{
      'webviewId': webviewId,
    });
  }

  @override
  Future<void> reload() async {
    final int? webviewId = instanceManager.getInstanceId(this);
    if (webviewId == null) {
      throw 'Failed to get the webview instance';
    }
    await (await LinuxWebViewPlugin.channel)
        .invokeMethod('reload', <String, dynamic>{
      'webviewId': webviewId,
    });
  }

  /// Not supported on Linux because the underlying browser does not provide
  /// such a feature.
  ///
  /// See [WebViewController.clearCache](https://pub.dev/documentation/webview_flutter/3.0.4/webview_flutter/WebViewController/clearCache.html)
  /// or [WebViewPlatformController.clearCache] for the original descriptions.
  @override
  Future<void> clearCache() {
    throw UnimplementedError(
        'WebView clearCache is not implemented on the current platform');
  }

  @override
  Future<String> evaluateJavascript(String javascript) {
    return runJavascriptReturningResult(javascript);
  }

  @override
  Future<void> runJavascript(String javascript) async {
    final int? webviewId = instanceManager.getInstanceId(this);
    if (webviewId == null) {
      throw 'Failed to get the webview instance';
    }

    await _runJavascriptInternal(webviewId, javascript);
  }

  /// See [WebViewController.runJavascriptReturningResult](https://pub.dev/documentation/webview_flutter/3.0.4/webview_flutter/WebViewController/runJavascriptReturningResult.html)
  /// or [WebViewPlatformController.runJavascriptReturningResult].
  ///
  /// On Linux, returns the evaluation result as a JSON formatted string.
  @override
  Future<String> runJavascriptReturningResult(String javascript) async {
    final int? webviewId = instanceManager.getInstanceId(this);
    if (webviewId == null) {
      throw 'Failed to get the webview instance';
    }

    _JavascriptResult jsResult =
        await _runJavascriptInternal(webviewId, javascript);
    if (!jsResult.wasExecuted) {
      throw "javascript was not executed for some reason";
    }
    if (jsResult.isException) {
      throw jsResult.result; // js error string
    }
    if (jsResult.isUndefined) {
      return "undefined";
    }
    return jsResult.result;
  }

  /// Calls the native runJavascript method and waits for the result. The result
  /// comes ascynhronously to [onMethodCall] with a `javascriptResult` method call
  /// from the native side on the method channel.
  Future<_JavascriptResult> _runJavascriptInternal(
      int webviewId, String javascript) async {
    final completer = Completer<_JavascriptResult>();

    // limit to the range 0 to int32 max
    _nextJsRunId = _nextJsRunId < _kInt32Max ? _nextJsRunId + 1 : 0;

    final _JsRunId jsRunId = _nextJsRunId;
    log.fine('_runJavascriptInternal: waiting for complete: jsRunId: $jsRunId');

    // Save the completer and wait until it is completed.
    //
    // The completer is completed when the `javascriptResult` method is called back
    // from the native side on the method channel.
    _jsCompleters[jsRunId] = completer;
    await (await LinuxWebViewPlugin.channel)
        .invokeMethod('requestRunJavascript', <String, dynamic>{
      'webviewId': webviewId,
      'jsRunId': jsRunId,
      'javascript': javascript,
    });
    _JavascriptResult jsResult = await completer.future;
    log.fine('_runJavascriptInternal: completed: jsRunId: $jsRunId');
    return jsResult;
  }

  /// Not implemented on Linux. Will be supported in the future.
  ///
  /// See [WebViewPlatformController.addJavascriptChannels] for the original
  /// description.
  ///
  /// TODO(Ino): implement [addJavascriptChannels]
  @override
  Future<void> addJavascriptChannels(Set<String> javascriptChannelNames) async {
    throw UnimplementedError(
        'WebView addJavascriptChannels is not implemented on the current platform');
  }

  /// Not implemented on Linux. Will be supported in the future.
  ///
  /// See [WebViewPlatformController.removeJavascriptChannels] for the original
  /// description.
  ///
  /// TODO(Ino): implement [removeJavascriptChannels]
  @override
  Future<void> removeJavascriptChannels(Set<String> javascriptChannelNames) {
    throw UnimplementedError(
        'WebView removeJavascriptChannels is not implemented on the current platform');
  }

  @override
  Future<String?> getTitle() async {
    final int? webviewId = instanceManager.getInstanceId(this);
    if (webviewId == null) {
      throw 'Failed to get the webview instance';
    }
    String? result = await (await LinuxWebViewPlugin.channel)
        .invokeMethod('getTitle', <String, dynamic>{
      'webviewId': webviewId,
    });
    return result;
  }

  /// Not implemented on Linux. Will be supported in the future.
  ///
  /// See [WebViewController.scrollTo](https://pub.dev/documentation/webview_flutter/3.0.4/webview_flutter/WebViewController/scrollTo.html)
  /// or [WebViewPlatformController.scrollTo] for the original descriptions.
  ///
  /// TODO(Ino): implement [scrollTo]
  @override
  Future<void> scrollTo(int x, int y) {
    throw UnimplementedError(
        'WebView scrollTo is not implemented on the current platform');
  }

  /// Not implemented on Linux. Will be supported in the future.
  ///
  /// See [WebViewController.scrollBy](https://pub.dev/documentation/webview_flutter/3.0.4/webview_flutter/WebViewController/scrollBy.html)
  /// or [WebViewPlatformController.scrollBy] for the original descriptions.
  ///
  /// TODO(Ino): implement [scrollBy]
  @override
  Future<void> scrollBy(int x, int y) {
    throw UnimplementedError(
        'WebView scrollBy is not implemented on the current platform');
  }

  /// Not implemented on Linux. Will be supported in the future.
  ///
  /// See [WebViewController.getScrollX](https://pub.dev/documentation/webview_flutter/3.0.4/webview_flutter/WebViewController/getScrollX.html)
  /// or [WebViewPlatformController.getScrollX] for the original descriptions.
  ///
  /// TODO(Ino): implement [getScrollX]
  @override
  Future<int> getScrollX() {
    throw UnimplementedError(
        'WebView getScrollX is not implemented on the current platform');
  }

  /// Not implemented on Linux. Will be supported in the future.
  ///
  /// See [WebViewController.getScrollY](https://pub.dev/documentation/webview_flutter/3.0.4/webview_flutter/WebViewController/getScrollY.html)
  /// or [WebViewPlatformController.getScrollY] for the original descriptions.
  ///
  /// TODO(Ino): implement [getScrollY]
  @override
  Future<int> getScrollY() {
    throw UnimplementedError(
        'WebView getScrollY is not implemented on the current platform');
  }
}

class _SerialTapGestureDetector extends StatelessWidget {
  const _SerialTapGestureDetector({
    Key? key,
    required this.child,
    this.onSerialTapDown,
  }) : super(key: key);

  final Widget child;

  final GestureSerialTapDownCallback? onSerialTapDown;
  // onSerialTapUp is not called once the drag begins, so we do not use this callback.
  // final GestureSerialTapUpCallback? onSerialTapUp;

  @override
  Widget build(BuildContext context) {
    return RawGestureDetector(
        child: child,
        gestures: <Type, GestureRecognizerFactory>{
          SerialTapGestureRecognizer:
              GestureRecognizerFactoryWithHandlers<SerialTapGestureRecognizer>(
            () =>
                SerialTapGestureRecognizer()..onSerialTapDown = onSerialTapDown,
            (instance) {},
          ),
        });
  }
}
