# flutter_linux_webview_example

## About the example apps

There are several example apps as shown below.

Note: If you are using Flutter prior to version 3.10, the example apps will fail to compile and require some editing. See each example app.

- lib/simple_example.dart
  - The simplest example app that displays a single WebView. It is the same as the example in the [README.md of the plugin](../README.md).
- lib/main.dart
  - This came from webview_flutter v3.0.4 example.
- lib/multiple_webviews_example.dart
  - The simple demo of dynamically adding and removing webviews. This app is not well maintained.

---

- integration_test/flutter_linux_webview_test.dart
  - This came from webview_flutter v3.0.4 example/integration_test.

## How to run

### Build and run

```shell
# Debug build and run
$ flutter run -t lib/simple_example.dart
# or
# Release build and run
$ flutter run --release -t lib/simple_example.dart
```

You can add `-v` option for verbose outputs.

### Only Building

For debug build:

```shell
# Debug build
$ flutter build linux --debug -t lib/simple_example.dart
# To run:
$ build/linux/x64/debug/bundle/flutter_linux_webview_example
```

For release build:

```shell
# Release build
$ flutter build linux -t lib/webview_sample.dart
# To run:
$ build/linux/x64/release/bundle/flutter_linux_webview_example
```

## Clean build artifacts and Flutter cache

```shell
$ flutter clean
```

## Run the test

```shell
$ flutter test integration_test/flutter_linux_webview_test.dart
```
