# flutter_linux_webview Validation Report

* flutter_linux_webview version: 0.1.0
* Validated on 2023-12-13

## Validation Apps

- example/lib/multiple_webviews_example.dart 
    - A simple demo app that allows us to "Add a new WebView" or "Remove a WebView" to/from the ListView by pressing the buttons.
    - The validation procedure:
        - Repeat the process of "Adding about 10 times then Removing about 10 times" about 10 times.
- example/integration_test/flutter_linux_webview_test.dart 
    - The integration test brought from <a href="https://github.com/flutter/plugins/blob/webview_flutter-v3.0.4/packages/webview_flutter/webview_flutter/example/integration_test/webview_flutter_test.dart">webview_flutter v3.0.4</a>.
    - During the test, more than 10 WebViews repeatedly start and disappear in a few seconds.

## Flutter versions used for the validation

Using Flutter Linux stable channel.

- Flutter 3.10.0 [release date: 2023-05-11]
- Flutter 3.13.9 [release date: 2023-10-26]
- Flutter 3.16.3 [release date: 2023-12-07] (stable as of 2023-12-13)

## Target Platforms

- (1) [laptop] Macbook Air 2014 x86_64 / Ubuntu 22.04 Desktop
- (2) [laptop] NEC LE150/N (2013) x86_64 / Debian 11 (bullseye)
- (3) [Desktop PC] Ryzen9 5900X + GeForce GTX 1650 x86_64 / Ubuntu 18.04 Chrome Remote Desktop
- (4) [Desktop PC] Ryzen9 5900X + Radeon RX 7800 XT x86_64 / Ubuntu 22.04 Desktop
- (5) Raspberry Pi 4 Model B arm64 / Raspberry Pi OS bullseye 64bit
- (6) Raspberry Pi 4 Model B arm64 / Ubuntu 22.04 Desktop

## Results

### (1) [laptop] Macbook Air 2014 x86_64 / Ubuntu 22.04 Desktop

<table>
  <tr>
   <td> </td>
   <td> </td>
   <td>Flutter 3.10.0 [2023-05-11] </td>
   <td>Flutter 3.13.9 [2023-10-26] </td>
   <td>Flutter 3.16.3 [2023-12-07] </td>
  </tr>
  <tr>
   <td> X11 </td>
   <td>lib/multiple_webviews_example.dart </td>
   <td> <span style="color:green">It works. No errors.</span> </td>
   <td> <span style="color:green">It works. No errors.</span> </td>
   <td> <span style="color:green">It works. No errors.</span> </td>
  </tr>
  <tr>
   <td> X11 </td>
   <td>integration_test/flutter_linux_webview_test.dart </td>
   <td> <span style="color:green">Test completes to the end.</span> </td>
   <td> <span style="color:green">Test completes to the end.</span> </td>
   <td> <span style="color:red">Test hangs on the first item.</span> </td>
  </tr>
  <tr>
   <td> Wayland </td>
   <td>lib/multiple_webviews_example.dart </td>
   <td> <span style="color:green">It works. No errors.</span> </td>
   <td> <span style="color:green">It works. No errors.</span> </td>
   <td> <span style="color:green">It works. No errors.</span> </td>
  </tr>
  <tr>
   <td> Wayland </td>
   <td>integration_test/flutter_linux_webview_test.dart </td>
   <td> <span style="color:green">Test completes to the end.</span> </td>
   <td> <span style="color:orange">Test sometimes crashes in the middle?</span> </td>
   <td> <span style="color:red">Test hangs on the first item.</span> </td>
  </tr>
</table>



### (2) [laptop] NEC LE150/N (2013) x86_64 / Debian 11 (bullseye)

Almost same results as above (1) Macbook Air Ubuntu 22.04 Desktop.


### (3) [Desktop PC] Ryzen9 5900X + GeForce GTX 1650 x86_64 / Ubuntu 18.04 Chrome Remote Desktop

Same results for X11 as above (1) Macbook Air Ubuntu 22.04 Desktop. (Not trying with Wayland.)

### (4) [Desktop PC] Ryzen9 5900X + Radeon RX 7800 XT x86_64 / Ubuntu 22.04 Desktop


<table>
  <tr>
   <td> </td>
   <td> </td>
   <td>Flutter 3.10.0 </td>
   <td>Flutter 3.13.9 </td>
   <td>Flutter 3.16.3 </td>
  </tr>
  <tr>
   <td> X11 </td>
   <td>lib/multiple_webviews_example.dart </td>
   <td> <span style="color:red">It works, but crashes about once every few dozen times when "Add a new webview" is pressed. (the crash message below)</span> </td>
   <td> <span style="color:red">It works, but crashes about once every few dozen times when "Add a new webview" is pressed. (the crash message below)</span> </td>
   <td> <span style="color:red">It works, but crashes about once every few dozen times when "Add a new webview" is pressed. (the crash message below)</span> </td>
  </tr>
  <tr>
   <td> X11 </td>
   <td>integration_test/flutter_linux_webview_test.dart </td>
   <td> <span style="color:red">Test crashes in the middle.</span> </td>
   <td> <span style="color:red">Test crashes in the middle.</span> </td>
   <td> <span style="color:red">Test hangs on the first item.</span> </td>
  </tr>
  <tr>
   <td> Wayland </td>
   <td>lib/multiple_webviews_example.dart </td>
   <td> <span style="color:green">It works. No errors.</span> </td>
   <td> <span style="color:green">It works. No errors.</span> </td>
   <td> <span style="color:green">It works. No errors.</span> </td>
  </tr>
  <tr>
   <td> Wayland </td>
   <td>integration_test/flutter_linux_webview_test.dart </td>
   <td> <span style="color:red">Test crashes in the middle.</span> </td>
   <td> <span style="color:red">Test crashes in the middle.</span> </td>
   <td> <span style="color:red">Test hangs on the first item.</span> </td>
  </tr>
</table>

#### Crash message: `X Window System BadAccess (attempt to access private resource denied)`

```
(flutter_linux_webview_example:7404): Gdk-ERROR **: 19:05:33.186: The program 'flutter_linux_webview_example' received an X Window System error.
This probably reflects a bug in the program.
The error was 'BadAccess (attempt to access private resource denied)'.
  (Details: serial 16332 error_code 10 request_code 152 (GLX) minor_code 26)
  (Note to programmers: normally, X errors are reported asynchronously;
   that is, you will receive the error a while after causing it.
   To debug your program, run it with the GDK_SYNCHRONIZE environment
   variable to change this behavior. You can then get a meaningful
   backtrace from your debugger if you break on the gdk_x_error() function.)
```

### (5) Raspberry Pi 4 Model B arm64 / Raspberry Pi OS bullseye 64bit


<table>
  <tr>
   <td> </td>
   <td> </td>
   <td>Flutter 3.10.0 </td>
   <td>Flutter 3.13.9 </td>
   <td>Flutter 3.16.3 </td>
  </tr>
  <tr>
   <td> X11 </td>
   <td>lib/multiple_webviews_example.dart </td>
   <td> <span style="color:green">It works. No errors.</span> </td>
   <td> <span style="color:red">Pressing "Add a new WebView" causes the app to hang (force termination required). But sometimes it does not hang at all. (*)</span> </td>
   <td> <span style="color:red">Pressing "Add a new WebView" causes the app to hang (force termination required). But sometimes it does not hang at all.</span> </td>
  </tr>
  <tr>
   <td> X11 </td>
   <td>integration_test/flutter_linux_webview_test.dart </td>
   <td> <span style="color:green">Test completes to the end.</span> </td>
   <td> <span style="color:green">Test completes to the end.</span> </td>
   <td> <span style="color:red">Test hangs on the first item.</span> </td>
  </tr>
</table>


(*) In Flutter 3.13.9, on rare occasions, I have seen crashes with the following errors instead of hangs.

```
[xcb] Unknown sequence number while processing queue
[xcb] Most likely this is a multi-threaded client and XInitThreads has not been called
[xcb] Aborting, sorry about that.
flutter_linux_webview_example: ../../src/xcb_io.c:269: poll_for_event: Assertion `!xcb_xlib_threads_sequence_lost' failed.
[0100/000000.502783:ERROR:zygote_linux.cc(607)] Zygote could not fork: process_type renderer numfds 4 child_pid -1
[0100/000000.503420:ERROR:zygote_linux.cc(639)] write: Broken pipe (32)
```

### (6) Raspberry Pi 4 Model B arm64 / Ubuntu 22.04 Desktop


<table>
  <tr>
   <td> </td>
   <td> </td>
   <td>Flutter 3.10.0 </td>
   <td>Flutter 3.13.9 </td>
   <td>Flutter 3.16.3 </td>
  </tr>
  <tr>
   <td> X11 </td>
   <td>lib/multiple_webviews_example.dart </td>
   <td> <span style="color:green">It works. No errors.</span> </td>
   <td> <span style="color:green">It works. No errors.</span> </td>
   <td> <span style="color:green">It works. No errors.</span> </td>
  </tr>
  <tr>
   <td> X11 </td>
   <td>integration_test/flutter_linux_webview_test.dart </td>
   <td> <span style="color:green">Test completes to the end.</span> </td>
   <td> <span style="color:green">Test completes to the end.</span> </td>
   <td> <span style="color:red">Test hangs on the first item.</span> </td>
  </tr>
  <tr>
   <td> Wayland </td>
   <td>lib/multiple_webviews_example.dart </td>
   <td> No hang, but only black texture is displayed. (*) </td>
   <td> No hang, but only black texture is displayed. (*) </td>
   <td> No hang, but only black texture is displayed. (*) </td>
  </tr>
  <tr>
   <td> Wayland </td>
   <td>integration_test/flutter_linux_webview_test.dart </td>
   <td> <span style="color:red">Test crashes in the middle.</span> </td>
   <td> <span style="color:red">Test crashes in the middle.</span> </td>
   <td> <span style="color:red">Test hangs on the first item.</span> </td>
  </tr>
</table>

(*) On RPi4 Ubuntu 22.04 Wayland, any Flutter app will not work without `GDK_GL=gles`.  
(ref. [Flutter app can't create a GL context on RPi 4/Ubuntu Core 20 - Issue #49 - MirServer/ubuntu-frame - GitHub](https://github.com/MirServer/ubuntu-frame/issues/49))  
However, with `GDK_GL=gles`, off-screen rendering does not work. The texture is not rendered and we get a black WebView.  
`$ GDK_GL=gles cefclient --off-screen-rendering-enabled` also results in a white screen.
