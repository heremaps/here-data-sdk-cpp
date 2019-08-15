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

#include <olp/core/CoreApi.h>
#include <olp/core/thread/TaskScheduler.h>

#include <memory>

namespace olp {
namespace http {
class Network;
}  // namespace http

namespace client {
/**
 * @brief Factory that helps to fill-in OlpClientSettings structure with various
 * default handlers.
 */
class CORE_API OlpClientSettingsFactory final {
 public:
  /**
   * @brief Creates a TaskScheduler instance used for all delayed operations,
   * which is defaulted to olp::thread::ThreadPoolTaskScheduler with one worker
   * thread spawned by default.
   * @return An instance of TaskScheduler.
   */
  static std::unique_ptr<thread::TaskScheduler> CreateDefaultTaskScheduler(
      size_t thread_count = 1u);

  /**
   * @brief Creates a Network instance used for all non-local requests,
   * which is defaulted to platform-specific implementation.
   * @return An instance of Network.
   */
  static std::unique_ptr<http::Network> CreateDefaultNetworkRequestHandler();
};

}  // namespace client
}  // namespace olp
