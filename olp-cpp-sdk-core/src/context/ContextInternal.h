/*
 * Copyright (C) 2023 HERE Europe B.V.
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

#include <olp/core/context/Context.h>

#include <memory>
#include <vector>
#if defined(__APPLE__)
#include <list>
#include <map>
#include <string>
#endif

#if defined(ANDROID) || defined(ANDROID_HOST)
#include <jni.h>
#endif

#if defined(__APPLE__)
#include <olp/core/context/EnterBackgroundSubscriber.h>
#endif

namespace olp {
namespace context {

// Because of static initializer orders, we need to have this on the heap.
struct ContextData {
  std::vector<olp::context::Context::InitializedCallback> init_vector;
  std::vector<olp::context::Context::DeinitializedCallback> deinit_vector;

  std::mutex context_mutex;
  size_t context_instance_counter = 0;
#if defined(ANDROID) || defined(ANDROID_HOST)
  JavaVM* java_vm = nullptr;
  jobject context = nullptr;
#elif defined(__APPLE__)
  std::list<std::weak_ptr<EnterBackgroundSubscriber> >
      enter_background_subscibers;
  std::map<std::string, std::function<void()> > completion_handlers;
#endif
};

std::shared_ptr<ContextData> Instance();

#if defined(__APPLE__)
/**
 * @brief Represents API related to the `Context` not intended to be public
 */
class ContextInternal {
 public:
  /**
   * @brief The deleted default constructor.
   */
  ContextInternal() = delete;

  /**
   * @brief Subscribe to be informed when the `Context` is entering or exiting
   * background mode.
   *
   * @param subscriber Weak pointer to the `EnterBackgroundSubscriber` object
   * that wants to be informed when the application is entering and exiting
   * background.
   *
   * @note Use it only after you initialize the `Context` class.
   */
  static void SubscribeEnterBackground(
      std::weak_ptr<EnterBackgroundSubscriber> subscriber);

  /**
   * @brief Unsubscribe from being informed when the `Context` is entering or
   * exiting background mode. Not valid subscribers are excluded as well.
   *
   * @param subscriber Weak pointer to the `EnterBackgroundSubscriber` object
   * that do not want to be informed when the application is entering and
   * exiting background anymore.
   *
   * @note Use it only after you initialize the `Context` class.
   */
  static void UnsubscribeEnterBackground(
      std::weak_ptr<EnterBackgroundSubscriber> subscriber);

  /**
   * @brief Call completion handler stored in the `Context`.
   * Basically informs iOS the planned background activity has been done.
   * See iOS downloading files in the background documentation for more details.
   *
   * @param session_name Represents session name to call completion handler for.
   */
  static void CallBackgroundSessionCompletionHandler(
      const std::string& session_name);
};
#endif  // __APPLE__
}  // namespace context
}  // namespace olp
