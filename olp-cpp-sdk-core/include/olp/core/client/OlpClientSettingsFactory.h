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
namespace cache {
class KeyValueCache;
struct CacheSettings;
}  // namespace cache

namespace http {
class Network;
}  // namespace http

namespace client {
/**
 * @brief Fills in the `OlpClientSettings` structure with
 * default handlers.
 */
class CORE_API OlpClientSettingsFactory final {
 public:
  /**
   * @brief Creates the `TaskScheduler` instance used for all the delayed
   * operations.
   *
   * Defaulted to `olp::thread::ThreadPoolTaskScheduler` with one worker
   * thread spawned by default.
   *
   * @return The `TaskScheduler` instance.
   */
  static std::unique_ptr<thread::TaskScheduler> CreateDefaultTaskScheduler(
      size_t thread_count = 1u);

  /**
   * @brief Creates the `Network` instance used for all the non-local requests.
   *
   * Defaulted to platform-specific implementation.
   *
   * @return The `Network` instance.
   */
  static std::shared_ptr<http::Network> CreateDefaultNetworkRequestHandler(
      size_t max_requests_count = 30u);

  /**
   * @brief Creates the `KeyValueCache` instance that includes both a small
   * in-memory LRU cache and a larger persistent database cache.
   *
   * The returned cache instance is initialized, opened, and ready to be used.
   *
   * @note The database cache is only created if the provided `CacheSettings`
   * instance includes a valid disk path with the corresponding write
   * permissions set.
   *
   * @param[in] settings The `CacheSettings` instance.
   *
   * @return The `KeyValueCache` instance.
   */
  static std::unique_ptr<cache::KeyValueCache> CreateDefaultCache(
      const cache::CacheSettings& settings);
};

}  // namespace client
}  // namespace olp
