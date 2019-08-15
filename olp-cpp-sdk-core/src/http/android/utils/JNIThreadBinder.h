/*
 * Copyright (C) 2019 HERE Europe B.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 * License-Filename: LICENSE
 */

#pragma once

#ifdef __ANDROID__

#include <jni.h>

namespace olp {
namespace http {
namespace utils {

// Scoped helper class to perform attaching / detaching of current C++
// thread into JVM
class JNIThreadBinder final {
 public:
  explicit JNIThreadBinder(JavaVM* vm)
      : attached_(false), jni_env_(nullptr), jni_vm_(vm) {
    if (jni_vm_->GetEnv(reinterpret_cast<void**>(&jni_env_), JNI_VERSION_1_6) !=
        JNI_OK) {
      if (jni_vm_->AttachCurrentThread(&jni_env_, nullptr) == JNI_OK) {
        attached_ = true;
      }
    }
  }

  ~JNIThreadBinder() {
    if (attached_) {
      jni_vm_->DetachCurrentThread();
    }
  }

  /// Non-movable, non-copyable
  JNIThreadBinder(JNIThreadBinder&& other) = delete;
  JNIThreadBinder(const JNIThreadBinder& other) = delete;
  JNIThreadBinder& operator=(JNIThreadBinder&& other) = delete;
  JNIThreadBinder& operator=(const JNIThreadBinder& other) = delete;

  JNIEnv* GetEnv() const { return jni_env_; }

 private:
  bool attached_;
  JNIEnv* jni_env_;
  JavaVM* jni_vm_;
};

}  // namespace utils
}  // namespace http
}  // namespace olp

#endif
