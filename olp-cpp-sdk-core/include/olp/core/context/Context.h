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

#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include <olp/core/CoreApi.h>

#ifdef ANDROID
#include <jni.h>
#endif

namespace olp {
namespace context {
/**
 * @brief The Context class represents the application context
 *
 * Client application must initialize its Context before using any other
 * functionality of the library.
 */

struct ContextData;

class CORE_API Context {
 public:
  using InitializedCallback = std::function<void()>;
  using DeinitializedCallback = std::function<void()>;

  /**
   * @brief addInitializeCallbacks Register functions to be called after the
   * context is initialized and destroyed.
   * @param initCallback
   * @param deinitCalback
   */
  static void addInitializeCallbacks(InitializedCallback initCallback,
                                     DeinitializedCallback deinitCalback);

  /**
   * @brief The Scope class initializes the context in its constructor (if not
   * already initialized) and deinitializes in the destructor (when no other
   * Scope instances exist). Instead of calling Context::init() and
   * Context::deinit() manually, simply instantiate a Context::Scope() object.
   */
  class CORE_API Scope {
   public:
    /// see Context::init()
    Scope();

#ifdef ANDROID
    /// see Context::init()
    Scope(JavaVM* vm, jobject context);
#endif

    /// deleted copy constructor
    Scope(const Scope&) = delete;
    /// deleted assignment operator
    Scope& operator=(const Scope&) = delete;

    /// see Context::deinit()
    ~Scope();

   private:
    std::shared_ptr<ContextData> m_cd;
  };

  /// deleted default constructor
  Context() = delete;

  friend class Context::Scope;

 private:
  /// initialize the Context
  static void init();

  /// deinitialize the Context
  static void deinit();

#ifdef ANDROID
  /// initialize the Context
  static void init(JavaVM* vm, jobject application);
#endif

 public:
#ifdef ANDROID
  /// returns a Java VM object
  static JavaVM* getJavaVM();
  static jobject getAndroidContext();
#endif
};

}  // namespace context
}  // namespace olp
