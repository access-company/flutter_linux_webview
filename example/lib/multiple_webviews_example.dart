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

import 'package:flutter/material.dart';
import 'dart:async';
import 'dart:io' show Platform;
// Required to use AppExitResponse for Fluter 3.10 or later
import 'dart:ui';

import 'package:flutter_linux_webview/flutter_linux_webview.dart';
import 'package:webview_flutter/webview_flutter.dart';

Future<void> main() async {
  WebView.platform = LinuxWebView();
  WidgetsFlutterBinding.ensureInitialized();
  await LinuxWebViewPlugin.initialize();
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({Key? key}) : super(key: key);

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> with WidgetsBindingObserver {
  int _webviewWidth = 700;
  int _webviewHeight = 300;

  final webviewsModel = <WebView>[];
  final savedControllers = <WebViewController>[];

  @override
  void initState() {
    super.initState();
    Map<String, String> envVars = Platform.environment;
    List<String>? size = envVars['WEBVIEW_SIZE']?.split("x");
    if (size != null && size.length == 2) {
      _webviewWidth = int.parse(size[0]);
      _webviewHeight = int.parse(size[1]);
    }
    WidgetsBinding.instance.addObserver(this);
    initPlatformState();
  }

  /// Prior to Flutter 3.10, comment it out since
  /// [WidgetsBindingObserver.didRequestAppExit] does not exist.
  @override
  Future<AppExitResponse> didRequestAppExit() async {
    await LinuxWebViewPlugin.terminate();
    return AppExitResponse.exit;
  }

  // Platform messages are asynchronous, so we initialize in an async method.
  Future<void> initPlatformState() async {
    // If the widget was removed from the tree while the asynchronous platform
    // message was in flight, we want to discard the reply rather than calling
    // setState to update our non-existent appearance.
    if (!mounted) return;

    setState(() {});
  }

  @override
  Widget build(BuildContext context) {
    Widget wrapWebviewWithCard(WebView webview) {
      return Card(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: <Widget>[
            ListTile(
              title: Text('WEBVIEW_SIZE: ${_webviewWidth}x${_webviewHeight}'),
            ),
            SizedBox(
                width: _webviewWidth.toDouble(),
                height: _webviewHeight.toDouble(),
                child: webview)
          ],
        ),
      );
    }

    return MaterialApp(
      home: Scaffold(
          backgroundColor: Color.fromARGB(255, 220, 220, 220),
          appBar: AppBar(
            title: const Text('Plugin example app'),
          ),
          body: Center(
            child: Container(
              width: _webviewWidth + 200,
              child: ListView(
                children: [
                  ButtonBar(children: [
                    TextButton(
                      child: const Text('Add a new WebView'),
                      onPressed: () {
                        setState(() {
                          webviewsModel.add(WebView(
                              initialUrl: 'https://www.google.com',
                              onWebViewCreated: (WebViewController controller) {
                                savedControllers.add(controller);
                              }));
                        });
                      },
                    ),
                    TextButton(
                      child: const Text('Remove a WebView'),
                      onPressed: () {
                        setState(() {
                          webviewsModel.removeAt(0);
                        });
                      },
                    ),
                  ]),
                  for (int i = 0; i < webviewsModel.length; i++)
                    wrapWebviewWithCard(webviewsModel[i])
                ],
              ),
            ),
          )),
    );
  }
}
