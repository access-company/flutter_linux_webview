# Copyright (c) 2023 ACCESS CO., LTD. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of ACCESS CO., LTD. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


# The Flutter tooling requires that developers have CMake 3.10 or later
# installed. You should not increase this version, as doing so will cause
# the plugin to fail to compile for some customers of the plugin.
cmake_minimum_required(VERSION 3.10)

# Project-level configuration.
set(PROJECT_NAME "flutter_linux_webview")
project(${PROJECT_NAME} LANGUAGES CXX)

# This value is used when generating builds using this plugin, so it must
# not be changed.
set(PLUGIN_NAME "flutter_linux_webview_plugin")

# TODO(Ino): Consider `cmake_policy(SET CMP0074 NEW)` to supress warnings in
# newer versions of CMake

#
# Switches for development of this plugin
#
# You need to run cmake manually to use these switches, since it does not seem
# to be possible to control the cmake parameters when running `flutter run` or
# `flutter build linux`.
#
# Running `flutter build linux -v` will tell you what cmake commands the Flutter
# SDK executes in which working directory, so you can reproduce those commands
# with the switches.
# For example, in `~/flutter_linux_webview/example/build/linux/x64/debug`,
# `cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DFLUTTER_TARGET_PLATFORM=linux-x64 \
# -DFLUTTER_WEBVIEW_DEBUG=ON /home/user/flutter_linux_webview/example/linux`
#
option(USE_CEF_STANDARD_DISTRIB
  "The switch for development of this plugin. \
  \
  If ON, the CEF standard distribution will be downloaded instead of the \
  CEF minimal distribution which only contains Release binaries. For Debug \
  builds of the app, this plugin will link to the Debug version of \
  libcef.so. For Release and Profile builds of the app, this plugin will \
  link to the Release version of libcef.so."
  OFF # Disabled by default
)

option(FLUTTER_WEBVIEW_DEBUG
  "The switch for development of this plugin. \
  \
  If ON, the ifdef preprocessor directives of FLUTTER_WEBVIEW_DEBUG are \
  enabled."
  OFF # Disabled by default
)


# #######################################################################
# Ensure that the Flutter app executable linkes to libcef.so
# #######################################################################
set(libcef_link_include_command_string
  "include(flutter/ephemeral/.plugin_symlinks/flutter_linux_webview/linux/cmake/link_to_cef_library.cmake)")

message(
  "[flutter_linux_webview] "
  "Checking if CMakeLists.txt in the top-level project contains a "
  "target_link_libraries() command to the CEF library.\n"
  "Searching for the following command in '${CMAKE_SOURCE_DIR}/CMakeLists.txt':\n"
  "  '${libcef_link_include_command_string}'")
execute_process(COMMAND

  # execute a shell for piping.
  "/bin/sh" "-c"

  # 1. Extract non-comment lines:
  # $ grep -Ev '^\s*$|^\s*\#' CMakeLists.txt
  # 2. And then, search for an include() command such as:
  # ```
  # include(flutter/ephemeral/.plugin_symlinks/flutter_linux_webview/linux/cmake/link_to_cef_library.cmake)
  # ```
  # but tabs, spaces and breaks around `include`, `(` and `)` are allowed.
  #
  # $ grep -Pzo '[ \t]*include\s*\(\s*\"?flutter\/ephemeral\/\.plugin_symlinks\/flutter_linux_webview\/linux\/cmake\/link_to_cef_library\.cmake\"?\s*\)'
  "grep -Ev '^\\s*$|^\\s*\\#' CMakeLists.txt | grep -Pzo '[ \\t]*include\\s*\\(\\s*\\\"?flutter\\/ephemeral\\/\\.plugin_symlinks\\/flutter_linux_webview\\/linux\\/cmake\\/link_to_cef_library\\.cmake\\\"?\\s*\\)'"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE unused_libcef_link_grep_output
  RESULT_VARIABLE libcef_link_grep_result)

if("${libcef_link_grep_result}" EQUAL "0")
  message("[flutter_linux_webview] Found include() statement for linking to the CEF library!!")
else()
  message("[flutter_linux_webview] Could not find include() statement for linking to the CEF library...")
  message("[flutter_linux_webview] Append the statement to ${CMAKE_SOURCE_DIR}/CMakeLists.txt.")

  #
  # Append 'include(...link_to_cef_library.cmake)' at the end of CMakeLists.txt in the top-level project.
  #
  set(content_to_append
    "\n\n"
    "# This include() command was automatically added by the flutter_linux_webview\n"
    "# plugin when building the app with the plugin. The app executable must be\n"
    "# linked to the CEF library for the WebView plugin to work. This command links\n"
    "# the app executable to the CEF library.\n"
    "${libcef_link_include_command_string}")
  file(APPEND "${CMAKE_SOURCE_DIR}/CMakeLists.txt" ${content_to_append})

  message(FATAL_ERROR
    "[flutter_linux_webview] "
    "To build a Flutter app with flutter_linux_webview plugin, CMakeLists.txt "
    "in the Flutter app executable must contain the following command to link "
    "to the CEF library:\n"
    "  '${libcef_link_include_command_string}'\n"
    "\n"
    "There is a known issue where the flutter_linux_webview plugin hangs "
    "if the Flutter app executable is not linked to the CEF library, "
    "causing CefInitialize() to fail and never return from there.\n"
    "\n"
    "The above command has been automatically added to the end of "
    "${CMAKE_SOURCE_DIR}/CMakeLists.txt.\n"
    "So please re-build this app project again!")
endif()

# #######################################################################
# Download the CEF binary distribution to the plugin source dir and
# extract it
# #######################################################################
if(${USE_CEF_STANDARD_DISTRIB})
  # USE_CEF_STANDARD_DISTRIB ON
  set(WEBVIEW_CEF_DISTRIB_NAME
    "cef_binary_96.0.18+gfe551e4+chromium-96.0.4664.110_linux64")
  set(WEBVIEW_CEF_DISTRIB_URL
    "https://cef-builds.spotifycdn.com/cef_binary_96.0.18%2Bgfe551e4%2Bchromium-96.0.4664.110_linux64.tar.bz2")
  set(WEBVIEW_CEF_DISTRIB_HASH_SHA1 "5456140d047397c2ce7e2942b636123d67718c7c")

  if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(WEBVIEW_CEF_BUILD_TYPE "Debug")
  else()
    set(WEBVIEW_CEF_BUILD_TYPE "Release")
  endif()
else()
  # USE_CEF_STANDARD_DISTRIB OFF
  if("${CMAKE_HOST_SYSTEM_PROCESSOR}" STREQUAL "aarch64")
    # linux arm64
    set(WEBVIEW_CEF_DISTRIB_NAME
      "cef_binary_96.0.18+gfe551e4+chromium-96.0.4664.110_linuxarm64_minimal")
    set(WEBVIEW_CEF_DISTRIB_URL
      "https://cef-builds.spotifycdn.com/cef_binary_96.0.18%2Bgfe551e4%2Bchromium-96.0.4664.110_linuxarm64_minimal.tar.bz2")
    set(WEBVIEW_CEF_DISTRIB_HASH_SHA1 "604b8ee00ecb9957d1826a8a5f30882cd8018652")
    set(WEBVIEW_CEF_BUILD_TYPE "Release")
    # The CEF binary distrib requires the PROJECT_ARCH variable
    set(PROJECT_ARCH "arm64")
  else()
    # linux x86_64
    set(WEBVIEW_CEF_DISTRIB_NAME
      "cef_binary_96.0.18+gfe551e4+chromium-96.0.4664.110_linux64_minimal")
    set(WEBVIEW_CEF_DISTRIB_URL
      "https://cef-builds.spotifycdn.com/cef_binary_96.0.18%2Bgfe551e4%2Bchromium-96.0.4664.110_linux64_minimal.tar.bz2")
    set(WEBVIEW_CEF_DISTRIB_HASH_SHA1 "2f944adae71dd1a6c4d795373166a09f543c7cbd")
    set(WEBVIEW_CEF_BUILD_TYPE "Release")
  endif()
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${WEBVIEW_CEF_DISTRIB_NAME}")
  # Download the CEF binary distribution
  file(DOWNLOAD "${WEBVIEW_CEF_DISTRIB_URL}"
    "${CMAKE_CURRENT_SOURCE_DIR}/${WEBVIEW_CEF_DISTRIB_NAME}.tar.bz2"
    EXPECTED_HASH SHA1=${WEBVIEW_CEF_DISTRIB_HASH_SHA1}
    SHOW_PROGRESS)

  # Extract the CEF binary distribution
  message(STATUS "[flutter_linux_webview] Extracting the CEF binary \
    distribution archive (this may take a while): \
    '${CMAKE_CURRENT_SOURCE_DIR}/${WEBVIEW_CEF_DISTRIB_NAME}.tar.bz2'")
  execute_process(
    COMMAND "tar" "-xf" "${CMAKE_CURRENT_SOURCE_DIR}/${WEBVIEW_CEF_DISTRIB_NAME}.tar.bz2"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

set(WEBVIEW_SUBPROCESS_PROJECT_NAME "flutter_webview_subprocess")

function(patch_cef_distrib)
  set(webview_subprocess_project_dest_dir
    "${CMAKE_CURRENT_SOURCE_DIR}/${WEBVIEW_CEF_DISTRIB_NAME}/tests/${WEBVIEW_SUBPROCESS_PROJECT_NAME}")
  set(webview_subprocess_source_dir
    "${CMAKE_CURRENT_SOURCE_DIR}/subprocess")
  file(MAKE_DIRECTORY ${webview_subprocess_project_dest_dir})
  set(WEBVIEW_SUBPROCESS_SOURCES
    flutter_webview_render_app.cc
    flutter_webview_render_app.h
    flutter_webview_subprocess.cc
    flutter_webview_process_messages.cc
    flutter_webview_process_messages.h
  )

  foreach(subprocess_file ${WEBVIEW_SUBPROCESS_SOURCES})
    message(STATUS "[flutter_linux_webview] Creating a symbolic link\n"
      "  '${webview_subprocess_project_dest_dir}/${subprocess_file}'\n"
      "    -> '${webview_subprocess_source_dir}/src/${subprocess_file}'")
    execute_process(WORKING_DIRECTORY ${webview_subprocess_project_dest_dir}
      COMMAND "ln" "--symbolic" "--force" "${webview_subprocess_source_dir}/src/${subprocess_file}")
  endforeach(subprocess_file)

  message(STATUS "[flutter_linux_webview] Creating a symbolic link\n"
    "  '${webview_subprocess_project_dest_dir}/CMakeLists.txt'\n"
    "    -> '${webview_subprocess_source_dir}/src/CMakeLists_subprocess_project.txt'")
  execute_process(WORKING_DIRECTORY ${webview_subprocess_project_dest_dir}
    COMMAND "ln" "--symbolic" "--force"
    "${webview_subprocess_source_dir}/src/CMakeLists_subprocess_project.txt" "CMakeLists.txt")

  message(STATUS "[flutter_linux_webview] Patching CMakeLists.txt so that"
    "it builds flutter_webview_subprocess target.")
  execute_process(
    COMMAND "patch" "-N"
    ${CMAKE_CURRENT_SOURCE_DIR}/${WEBVIEW_CEF_DISTRIB_NAME}/CMakeLists.txt
    ${webview_subprocess_source_dir}/cef_distrib_patch/build_flutter_webview_subprocess.patch)
endfunction(patch_cef_distrib)

patch_cef_distrib()

# TODO(Ino): strip Release/libcef.so since it is 1.2GB large

# Configure CMake to build the CEF binary distrib
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/${WEBVIEW_CEF_DISTRIB_NAME}")

# Export libcef.so as a target 'cef_library' so that the top-level exe project
# can link to it.
add_library(cef_library SHARED IMPORTED GLOBAL)
set_target_properties(cef_library PROPERTIES IMPORTED_LOCATION
  "${CMAKE_CURRENT_SOURCE_DIR}/${WEBVIEW_CEF_DISTRIB_NAME}/${WEBVIEW_CEF_BUILD_TYPE}/libcef.so")

# #######################################################################
# Plugin build configurations
# #######################################################################

# Define the plugin library target. Its name must not be changed (see comment
# on PLUGIN_NAME above).
#
# Any new source files that you add to the plugin should be added here.
add_library(${PLUGIN_NAME} SHARED
  "flutter_linux_webview_plugin.cc"
  "fl_custom_texture_gl.cc"
  "flutter_webview_texture_manager.cc"
  "flutter_webview_app.cc"
  "flutter_webview_controller.cc"
  "flutter_webview_handler.cc"
  "flutter_webview_types.cc"
  "subprocess/src/flutter_webview_process_messages.cc"
)

# Apply a standard set of build settings that are configured in the
# application-level CMakeLists.txt. This can be removed for plugins that want
# full control over build settings.
apply_standard_settings(${PLUGIN_NAME})

# Symbols are hidden by default to reduce the chance of accidental conflicts
# between plugins. This should not be removed; any symbols that should be
# exported should be explicitly exported with the FLUTTER_PLUGIN_EXPORT macro.
set_target_properties(${PLUGIN_NAME} PROPERTIES
  CXX_VISIBILITY_PRESET hidden)
target_compile_definitions(${PLUGIN_NAME} PRIVATE FLUTTER_PLUGIN_IMPL)

if (FLUTTER_WEBVIEW_DEBUG)
  target_compile_definitions(${PLUGIN_NAME} PRIVATE FLUTTER_WEBVIEW_DEBUG)
  message("[flutter_linux_webview] FLUTTER_WEBVIEW_DEBUG is ON.")
endif()

# Source include directories and library dependencies. Add any plugin-specific
# dependencies here.
target_include_directories(${PLUGIN_NAME} PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(${PLUGIN_NAME} PRIVATE flutter)
target_link_libraries(${PLUGIN_NAME} PRIVATE PkgConfig::GTK)

# The include directory for the CEF headers
target_include_directories(${PLUGIN_NAME} PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/${WEBVIEW_CEF_DISTRIB_NAME}")

target_link_libraries(${PLUGIN_NAME} PRIVATE libcef_dll_wrapper)
target_link_libraries(${PLUGIN_NAME} PRIVATE cef_library)

# Import GL
pkg_check_modules(GL REQUIRED gl)
target_include_directories(${PLUGIN_NAME} PRIVATE ${GL_INCLUDE_DIRS})
target_link_libraries(${PLUGIN_NAME} PRIVATE ${GL_LIBRARIES})

# #######################################################################
# Installing
# #######################################################################

# The location in the Flutter app bundle where the libraries are installed.
# The default CMakeLists.txt for Flutter Linux apps generated by
# the Flutter SDK installs libraries in '/bundle/lib', so this plugin
# also installs the CEF libraries in '/bundle/lib'.
set(INSTALL_BUNDLE_LIB_DIR "${CMAKE_BINARY_DIR}/bundle/lib")

#
# Install the plugin library with modifying RPATH to "$ORIGIN"
#
# By default, the RPATH will be the build tree path (issue reported in
# https://github.com/flutter/flutter/issues/65400)
set_target_properties(${PLUGIN_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")
install(TARGETS ${PLUGIN_NAME} LIBRARY DESTINATION "${INSTALL_BUNDLE_LIB_DIR}"
  COMPONENT Runtime)

#
# Install executables
#
function(install_executables)
  # The list of executables to be copied with `INSTALL(PROGRAMS)` command,
  # which copies the files and gives them execute permission.
  set(WEBVIEW_EXECUTABLE_LIST_TO_DEPLOY)

  # The path of the sub-process executable file
  set(subprocess_exe_path
    "${CMAKE_CURRENT_BINARY_DIR}/${WEBVIEW_CEF_DISTRIB_NAME}/tests/${WEBVIEW_SUBPROCESS_PROJECT_NAME}/$<CONFIG>/${WEBVIEW_SUBPROCESS_PROJECT_NAME}")
  list(APPEND WEBVIEW_EXECUTABLE_LIST_TO_DEPLOY "${subprocess_exe_path}")

  # install the executables
  install(PROGRAMS ${WEBVIEW_EXECUTABLE_LIST_TO_DEPLOY} DESTINATION ${INSTALL_BUNDLE_LIB_DIR} COMPONENT Runtime)
endfunction(install_executables)

install_executables()

#
# Install directories and regular (non-executable) files
#
function(install_dirs_and_files)
  # A list of object paths to be installed
  set(webview_object_list)

  set(CEF_BINARY_FILES
    libcef.so
    snapshot_blob.bin
    v8_context_snapshot.bin
    libEGL.so
    libGLESv2.so
    libvk_swiftshader.so
    libvulkan.so.1
    vk_swiftshader_icd.json

    # Not required to run:
    # chrome-sandbox
    # swiftshader
  )

  set(CEF_RESOURCE_FILES
    chrome_100_percent.pak
    chrome_200_percent.pak
    resources.pak
    icudtl.dat
    locales
  )

  set(cef_distrib_dir "${CMAKE_CURRENT_SOURCE_DIR}/${WEBVIEW_CEF_DISTRIB_NAME}")

  foreach(binary_file ${CEF_BINARY_FILES})
    list(APPEND webview_object_list ${cef_distrib_dir}/${WEBVIEW_CEF_BUILD_TYPE}/${binary_file})
  endforeach(binary_file)

  foreach(resource_file ${CEF_RESOURCE_FILES})
    list(APPEND webview_object_list ${cef_distrib_dir}/Resources/${resource_file})
  endforeach(resource_file)

  # The list of directories to be copied with `INSTALL(DIRECTORY)` command.
  set(WEBVIEW_DIR_LIST_TO_DEPLOY)

  # The list of files to be copied with `INSTALL(FILES)` command.
  set(WEBVIEW_FILE_LIST_TO_DEPLOY)

  # Assign the paths to *_TO_DEPLOY
  foreach(object ${webview_object_list})
    if(IS_DIRECTORY "${object}")
      list(APPEND WEBVIEW_DIR_LIST_TO_DEPLOY ${object})
    else()
      list(APPEND WEBVIEW_FILE_LIST_TO_DEPLOY ${object})
    endif()
  endforeach(object)

  install(DIRECTORY ${WEBVIEW_DIR_LIST_TO_DEPLOY} DESTINATION ${INSTALL_BUNDLE_LIB_DIR}
    USE_SOURCE_PERMISSIONS COMPONENT Runtime)
  install(FILES ${WEBVIEW_FILE_LIST_TO_DEPLOY} DESTINATION ${INSTALL_BUNDLE_LIB_DIR} COMPONENT Runtime)
endfunction(install_dirs_and_files)

install_dirs_and_files()
# The variable *_bundled_libraries is the standard way provided by the
# Flutter SDK to tell CMakeLists.txt in the Flutter app project which
# files should be bundled together.
#
# However, this plugin, flutter_webview_plugin, does not use that
# variable to bundle the CEF objects for two reasons:
# 1. Because it does not copy executables with execute permission even
# if the file is an executable. The sub-process executable this
# plugin uses must have execute permission.
# 2. Because it does not copy directories recursively.
set(flutter_linux_webview_bundled_libraries
  ""
  PARENT_SCOPE
)
