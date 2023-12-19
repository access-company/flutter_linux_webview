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
// Required to use AppExitResponse for Fluter 3.10 or later
import 'dart:ui';
import 'package:flutter/material.dart';
import 'package:webview_flutter/webview_flutter.dart';
import 'package:flutter_linux_webview/flutter_linux_webview.dart';

void main() {
  // ensureInitialized() is required if the plugin is initialized before runApp()
  WidgetsFlutterBinding.ensureInitialized();

  // Run `LinuxWebViewPlugin.initialize()` first before creating a WebView.
  LinuxWebViewPlugin.initialize(options: <String, String?>{
    'user-agent': 'UA String',
    'remote-debugging-port': '8888',
    'autoplay-policy': 'no-user-gesture-required',
  });

  // Configure [WebView] to use the [LinuxWebView].
  WebView.platform = LinuxWebView();

  runApp(const MaterialApp(home: _WebViewExample()));
}

class _WebViewExample extends StatefulWidget {
  const _WebViewExample({Key? key}) : super(key: key);

  @override
  _WebViewExampleState createState() => _WebViewExampleState();
}

class _WebViewExampleState extends State<_WebViewExample>
    with WidgetsBindingObserver {
  final Completer<WebViewController> _controller =
      Completer<WebViewController>();

  /// Prior to Flutter 3.10, comment out the following code since
  /// [WidgetsBindingObserver.didRequestAppExit] does not exist.
  // ===== begin: For Flutter 3.10 or later =====
  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
  }

  @override
  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
    super.dispose();
  }

  @override
  Future<AppExitResponse> didRequestAppExit() async {
    await LinuxWebViewPlugin.terminate();
    return AppExitResponse.exit;
  }
  // ===== end: For Flutter 3.10 or later =====

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('flutter_linux_webview example'),
      ),
      body: WebView(
        initialUrl: 'https://flutter.dev',
        initialCookies: const [
          WebViewCookie(name: 'mycookie', value: 'foo', domain: 'flutter.dev')
        ],
        onWebViewCreated: (WebViewController webViewController) {
          _controller.complete(webViewController);
        },
        javascriptMode: JavascriptMode.unrestricted,
      ),
      floatingActionButton: favoriteButton(),
    );
  }

  Widget favoriteButton() {
    return FutureBuilder<WebViewController>(
        future: _controller.future,
        builder: (BuildContext context,
            AsyncSnapshot<WebViewController> controller) {
          if (controller.hasData) {
            return FloatingActionButton(
              onPressed: () async {
                final String useragent = (await controller.data!
                    .runJavascriptReturningResult('navigator.userAgent'));
                final String title = (await controller.data!.getTitle())!;
                final String url = (await controller.data!.currentUrl())!;
                final String cookies = await (controller.data!
                    .runJavascriptReturningResult('document.cookie'));
                ScaffoldMessenger.of(context).showSnackBar(
                  SnackBar(
                    content: Text(
                        'userAgent: $useragent, title: $title, url: $url, cookie: $cookies'),
                  ),
                );
              },
              child: const Icon(Icons.favorite),
            );
          }
          return Container();
        });
  }
}
