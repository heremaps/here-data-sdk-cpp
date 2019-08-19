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

// Helper scoped smart pointer to manage JNI local reference lifetime
struct JNIScopedLocalReference final {
  JNIScopedLocalReference(JNIEnv* env, jstring string)
      : env(env), obj(string) {}

  JNIScopedLocalReference(JNIEnv* env, jobjectArray obj_array)
      : env(env), obj(obj_array) {}

  JNIScopedLocalReference(JNIEnv* env, jobject obj) : env(env), obj(obj) {}

  ~JNIScopedLocalReference() {
    if (obj) {
      env->DeleteLocalRef(obj);
    }
  }

  // Non-copyable, non-movable
  JNIScopedLocalReference(const JNIScopedLocalReference& other) = delete;
  JNIScopedLocalReference(JNIScopedLocalReference&& other) = delete;
  JNIScopedLocalReference& operator=(const JNIScopedLocalReference& other) =
      delete;
  JNIScopedLocalReference& operator=(JNIScopedLocalReference&& other) = delete;

  JNIEnv* env;
  jobject obj;
};

}  // namespace utils
}  // namespace http
}  // namespace olp

#endif
