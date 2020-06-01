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
#include "olp/core/client/OlpClientSettings.h"
#include "olp/core/http/Network.h"
#include "olp/core/http/NetworkConstants.h"
#include "olp/core/logging/Log.h"
#include "olp/core/porting/make_unique.h"
#include "olp/core/thread/ThreadPoolTaskScheduler.h"

namespace {
auto constexpr kLogTag = "OlpClientSettingsFactory";
}

namespace olp {
namespace client {

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

void OlpClientSettingsFactory::PrewarmConnection(
    const OlpClientSettings& settings, const std::string& url) {
  if (url.empty() || !settings.network_request_handler) {
    OLP_SDK_LOG_WARNING_F(kLogTag, "PrewarmConnection: invalid input, url='%s'",
                          url.c_str());
    return;
  }

  const auto& retry_settings = settings.retry_settings;
  auto request_settings =
      http::NetworkSettings()
          .WithTransferTimeout(retry_settings.timeout)
          .WithConnectionTimeout(retry_settings.timeout)
          .WithProxySettings(
              settings.proxy_settings.value_or(http::NetworkProxySettings()));

  auto request =
      http::NetworkRequest(url)
          .WithVerb(http::NetworkRequest::HttpVerb::OPTIONS)
          .WithBody(nullptr)
          .WithSettings(std::move(request_settings))
          .WithHeader(http::kUserAgentHeader, http::kOlpSdkUserAgent);

  auto outcome = settings.network_request_handler->Send(
      request, nullptr, [url](http::NetworkResponse response) {
        OLP_SDK_LOG_INFO_F(
            kLogTag,
            "PrewarmConnection: completed for url='%s', status='%d %s'",
            url.c_str(), response.GetStatus(), response.GetError().c_str());
      });

  if (!outcome.IsSuccessful()) {
    OLP_SDK_LOG_WARNING_F(
        kLogTag,
        "PrewarmConnection: sending OPTIONS failed, url='%s', error='%s'",
        url.c_str(), ErrorCodeToString(outcome.GetErrorCode()).c_str());
  }
}

}  // namespace client
}  // namespace olp
