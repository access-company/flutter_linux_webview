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

#include "flutter_webview_texture_manager.h"

#include <GL/gl.h>
#include <flutter_linux/flutter_linux.h>

#include <iostream>
#include <memory>
#include <unordered_map>

#include "flutter_linux_webview/fl_custom_texture_gl.h"
#include "flutter_linux_webview/flutter_linux_webview_plugin.h"

FlutterWebviewTextureManager::FlutterWebviewTextureManager() {}

FlCustomTextureGL* FlutterWebviewTextureManager::CreateAndRegisterTexture(
    WebviewId webview_id,
    GdkGLContext* context,
    FlTextureRegistrar* texture_registrar,
    int width,
    int height) {
  auto it_inserted = texture_store_.emplace(webview_id, nullptr);
  if (!it_inserted.second) {
    std::cerr << "Error: a texture for webview_id=" << webview_id
              << " is already stored." << std::endl;
    return nullptr;
  }

  gdk_gl_context_make_current(context);

  // Create a native texture
  GLuint native_texture_id;
  glGenTextures(1, &native_texture_id);

  // Create a custom fl texture
  g_autoptr(FlCustomTextureGL) flCustomTextureGL =
      fl_custom_texture_gl_new(GL_TEXTURE_2D, native_texture_id, width, height);

  // Store the created texture
  it_inserted.first->second =
      FL_CUSTOM_TEXTURE_GL(g_object_ref(flCustomTextureGL));

  if (!fl_texture_registrar_register_texture(texture_registrar,
                                             FL_TEXTURE(flCustomTextureGL))) {
    std::cerr << "Error: fl_texture_registrar_register_texture() failed."
              << std::endl;
    glDeleteTextures(1, &native_texture_id);
    g_object_unref(it_inserted.first->second);
    texture_store_.erase(it_inserted.first);
    return nullptr;
  }

  return flCustomTextureGL;
}

FlCustomTextureGL* FlutterWebviewTextureManager::GetTexture(
    WebviewId webview_id) {
  auto it = texture_store_.find(webview_id);
  if (it == texture_store_.end()) {
    // texture not found for webview_id
    return nullptr;
  }
  return it->second;
}

int64_t FlutterWebviewTextureManager::GetTextureId(FlTexture* fl_texture) {
  static_assert(sizeof(int64_t) >= sizeof(intptr_t),
                "Must be sizeof(int64_t) >= sizeof(intptr_t)");

  // As of Flutter Engine 3.0.4, since the Flutter Linux Embedder does not
  // provide an API to create a texture ID to pass to the Dart side, we have to
  // create it manually. Since the texture ID accepted by the Flutter Linux
  // Embedder is the memory address of the texture, we generate the ID that way.
  // See https://github.com/flutter/engine/pull/24916#discussion_r707007673 and
  // https://github.com/flutter/engine/blob/6ba2af10bb05c88a2731482cedf2cfd11cf5af0b/shell/platform/linux/fl_texture.cc#L20
  // (engine v3.0.4)
  //
  // TODO(Ino): In the near future, after Flutter 3.10.0, the texture ID to be
  // passed to the Dart side will be changed to the return value of the
  // fl_texture_get_id() instead of the texture's memory address. At that time,
  // this GetTextureId() will no longer be necessary and fl_texture_get_id()
  // should be used instead, see https://github.com/flutter/engine/pull/40899.
  // See also https://github.com/flutter/engine/pull/40290 and
  // https://github.com/flutter/flutter/issues/124009.
  return reinterpret_cast<int64_t>(fl_texture);
}

bool FlutterWebviewTextureManager::UnregisterAndDestroyTexture(
    WebviewId webview_id,
    FlTextureRegistrar* texture_registrar) {
  return UnregisterAndDestroyTextureInternal(webview_id, texture_registrar,
                                             false);
}

bool FlutterWebviewTextureManager::UnregisterAndDestroyTextureInternal(
    WebviewId webview_id,
    FlTextureRegistrar* texture_registrar,
    bool skip_unregister_texture) {
  auto it = texture_store_.find(webview_id);
  if (it == texture_store_.end()) {
    // Texture not found
    std::cerr << "Error: The FlCustomTextureGL texture for webview_id="
              << webview_id << " is not found." << std::endl;
    return false;
  }

  FlCustomTextureGL* texture = it->second;

  if (!skip_unregister_texture) {
    if (!fl_texture_registrar_unregister_texture(texture_registrar,
                                                 FL_TEXTURE(texture))) {
      std::cerr << "Warning: fl_texture_registrar_unregister_texture() failed"
                << std::endl;
    }
  }

  GLuint native_texture_id[1] = {texture->native_texture_id};
  glDeleteTextures(1, native_texture_id);
  g_object_unref(texture);
  texture_store_.erase(it);

  return true;
}

void FlutterWebviewTextureManager::UnregisterAndDestroyAllTextures(
    FlTextureRegistrar* texture_registrar,
    bool skip_unregister_texture) {
  std::vector<WebviewId> keys;
  for (auto it = texture_store_.begin(); it != texture_store_.end(); it++) {
    keys.push_back(it->first);
  }
  for (WebviewId key : keys) {
    UnregisterAndDestroyTextureInternal(key, texture_registrar,
                                        skip_unregister_texture);
  }
}
