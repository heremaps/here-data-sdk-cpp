/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
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

#include "olp/core/client/OlpClientSettingsFactory.h"

#include "olp/core/cache/CacheSettings.h"
#include "olp/core/cache/DefaultCache.h"
#include "olp/core/http/Network.h"
#include "olp/core/logging/Log.h"
#include "olp/core/porting/make_unique.h"
#include "olp/core/thread/ThreadPoolTaskScheduler.h"

namespace olp {
namespace client {

auto constexpr kLogTag = "OlpClientSettingsFactory";

std::unique_ptr<thread::TaskScheduler>
OlpClientSettingsFactory::CreateDefaultTaskScheduler(size_t thread_count) {
  return std::make_unique<thread::ThreadPoolTaskScheduler>(thread_count);
}

std::shared_ptr<http::Network>
OlpClientSettingsFactory::CreateDefaultNetworkRequestHandler(
    size_t max_requests_count) {
  return http::CreateDefaultNetwork(max_requests_count);
}

std::unique_ptr<cache::KeyValueCache>
OlpClientSettingsFactory::CreateDefaultCache(cache::CacheSettings settings) {
  auto cache = std::make_unique<cache::DefaultCache>(std::move(settings));
  auto error = cache->Open();
  if (error == cache::DefaultCache::StorageOpenResult::OpenDiskPathFailure) {
    OLP_SDK_LOG_FATAL_F(
        kLogTag,
        "Error opening disk cache, disk_path_mutable=%s, "
        "disk_path_protected=%s",
        settings.disk_path_mutable.get_value_or("(empty)").c_str(),
        settings.disk_path_protected.get_value_or("(empty)").c_str());
    return nullptr;
  }
  return cache;
}

}  // namespace client
}  // namespace olp
