/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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
 * @brief Represents the application context.
 *
 * Before your application uses any other functionality of the library,
 * initialize the `Context` class.
 */
struct ContextData;

/**
 * @brief Used only for the Android environment to correctly initialize
 * the `NetworkAndroid` class.
 *
 * Initialize the `Context` class before you send network requests in
 * the Android environment.
 * To use the `Context` class, cerate the `Scope` object first.
 */
class CORE_API Context {
 public:
  /**
   * @brief Called when the `Context` object is initialized.
   *
   * Controlled by the `Scope` class.
   *
   * @see `Scope` for more information.
   */
  using InitializedCallback = std::function<void()>;

  /**
   * @brief Called when the `Context` object is deinitialized.
   *
   * Controlled by the `Scope` class.
   *
   * @see `Scope` for more information.
   */
  using DeinitializedCallback = std::function<void()>;

  /**
   * @brief Registers functions that are called when the context is initialized
   * and destroyed.
   *
   * @param initCallback The `InitializedCallback` instance.
   * @param deinitCalback The `DeinitializedCallback` instance.
   */
  static void addInitializeCallbacks(InitializedCallback initCallback,
                                     DeinitializedCallback deinitCalback);

  /**
   * @brief Initializes the `Context` class in its constructor (if it is not
   * already initialized) and deinitializes in its destructor (if there are
   * no other `Scope` instances).
   *
   * Instead of calling `Context::init()` and `Context::deinit()` manually,
   * instantiate the `Context::Scope()` object.
   */
  class CORE_API Scope {
   public:
    /**
     * @brief Creates the `Scope` instance.
     *
     * The `Scope` instance is used to initialize the `Context` class.
     * It also automatically initializes the `Context` callbacks.
     *
     * @see `InitializedCallback` for more information.
     */
    Scope();

#ifdef ANDROID
    /**
     * @brief Creates the `Scope` instance.
     *
     * The `Scope` instance is used to initialize the `Context` class.
     * It also automatically initializes the `Context` callbacks.
     *
     * @see `InitializedCallback` for more information.
     *
     * @param vm The `JavaVM` instance.
     * @param context The `android.content.Context` instance.
     */
    Scope(JavaVM* vm, jobject context);
#endif

    /**
     * @brief The deleted copy constructor.
     */
    Scope(const Scope&) = delete;
    /**
     * @brief The deleted assignment operator.
     */
    Scope& operator=(const Scope&) = delete;

    /**
     * @brief Invokes deinitialized callbacks of the `Context` class.
     *
     * @see `DeinitializedCallback` for more information.
     */
    ~Scope();

   private:
    std::shared_ptr<ContextData> m_cd;
  };

  /**
   * @brief The deleted default constructor.
   */
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
  /**
   * @brief Gets the `JavaVM` object.
   *
   * @note Use it only after you initialize the `Context` class.
   *
   * @return The `JavaVM` object.
   */
  static JavaVM* getJavaVM();
  /**
   * @brief Get the `android.content.Context` instance.
   *
   * @note Use it only after you initialize the `Context` class.
   *
   * @return The `android.content.Context` instance.
   */
  static jobject getAndroidContext();
#endif
};

}  // namespace context
}  // namespace olp
