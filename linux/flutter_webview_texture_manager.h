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

#ifndef LINUX_FLUTTER_WEBVIEW_TEXTURE_MANAGER_H_
#define LINUX_FLUTTER_WEBVIEW_TEXTURE_MANAGER_H_

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>

#include <memory>
#include <unordered_map>

#include "flutter_linux_webview/fl_custom_texture_gl.h"
#include "flutter_linux_webview/flutter_webview_types.h"

// A utility class regarding fl_texture_gl. Accessed only on the platform plugin
// thread.
class FlutterWebviewTextureManager {
 public:
  FlutterWebviewTextureManager();

  ///
  /// For a given |webview_id|, creates a native texture, creates a
  /// FlCustomTextureGL from it, registers it with the engine, and stores it.
  ///
  /// @return (transfer none): Returns the newly created FlCustomTextureGL* on
  /// success, nullptr otherwise.
  ///
  FlCustomTextureGL* CreateAndRegisterTexture(
      WebviewId webview_id,
      GdkGLContext* context,
      FlTextureRegistrar* texture_registrar,
      int width,
      int height);

  ///
  /// Get a stored texture for a given |webview_id|
  ///
  /// @return (transfer none): Returns a FlCustomTextureGL*. Returns nullptr if
  /// a texture is not found.
  ///
  FlCustomTextureGL* GetTexture(WebviewId webview_id);

  ///
  /// Returns the ID of the given texture. This is the ID to pass to the Dart
  /// side.
  ///
  /// @return Returns the ID of |fl_texture|.
  ///
  int64_t GetTextureId(FlTexture* fl_texture);

  ///
  /// Unregister and delete a texture for a given |webview_id|.
  ///
  /// @return Returns if the native texture was successfully deleted.
  ///
  bool UnregisterAndDestroyTexture(WebviewId webview_id,
                                   FlTextureRegistrar* texture_registrar);

  /// Unregisters and deletes all registered textures.
  /// If |skip_unregister_texture| is true, it skips calling
  /// fl_texture_unregister_texture().
  void UnregisterAndDestroyAllTextures(FlTextureRegistrar* texture_registrar,
                                       bool skip_unregister_texture);

 private:
  bool UnregisterAndDestroyTextureInternal(
      WebviewId webview_id,
      FlTextureRegistrar* texture_registrar,
      bool skip_unregister_texture);

  std::unordered_map<WebviewId, FlCustomTextureGL*> texture_store_;
};

#endif  // LINUX_FLUTTER_WEBVIEW_TEXTURE_MANAGER_H_
