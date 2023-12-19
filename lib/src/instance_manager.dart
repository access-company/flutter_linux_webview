// Copyright 2013 The Flutter Authors. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// This file was copied from https://github.com/flutter/plugins/blob/webview_flutter-v3.0.4/packages/webview_flutter/webview_flutter_android/lib/src/instance_manager.dart

/// Maintains instances stored to communicate with java objects.
class InstanceManager {
  final Map<int, Object> _instanceIdsToInstances = <int, Object>{};
  final Map<Object, int> _instancesToInstanceIds = <Object, int>{};

  int _nextInstanceId = 0;

  /// Global instance of [InstanceManager].
  static final InstanceManager instance = InstanceManager();

  /// Attempt to add a new instance.
  ///
  /// Returns new if [instance] has already been added. Otherwise, it is added
  /// with a new instance id.
  int? tryAddInstance(Object instance) {
    if (_instancesToInstanceIds.containsKey(instance)) {
      return null;
    }

    final int instanceId = _nextInstanceId++;
    _instancesToInstanceIds[instance] = instanceId;
    _instanceIdsToInstances[instanceId] = instance;
    return instanceId;
  }

  /// Remove the instance from the manager.
  ///
  /// Returns null if the instance is removed. Otherwise, return the instanceId
  /// of the removed instance.
  int? removeInstance(Object instance) {
    final int? instanceId = _instancesToInstanceIds[instance];
    if (instanceId != null) {
      _instancesToInstanceIds.remove(instance);
      _instanceIdsToInstances.remove(instanceId);
    }
    return instanceId;
  }

  /// Retrieve the Object paired with instanceId.
  Object? getInstance(int instanceId) {
    return _instanceIdsToInstances[instanceId];
  }

  /// Retrieve the instanceId paired with instance.
  int? getInstanceId(Object instance) {
    return _instancesToInstanceIds[instance];
  }
}
