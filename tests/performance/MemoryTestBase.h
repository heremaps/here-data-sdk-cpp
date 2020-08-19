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

#pragma once

#include <gtest/gtest.h>

#include <olp/core/cache/CacheSettings.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/utils/Dir.h>
#include <testutils/CustomParameters.hpp>

#include "NetworkWrapper.h"
#include "NullCache.h"

using KeyValueCachePtr = std::shared_ptr<olp::cache::KeyValueCache>;
using CacheFactory = std::function<KeyValueCachePtr()>;

struct TestBaseConfiguration {
  std::uint8_t task_scheduler_capacity{5};
  CacheFactory cache_factory{nullptr};
  bool with_http_errors{false};
  bool with_network_timeouts{false};
};

template <typename Param>
class MemoryTestBase : public ::testing::TestWithParam<Param> {
  static_assert(std::is_base_of<TestBaseConfiguration, Param>::value,
                "Param must derive from TestBaseConfiguration");

 protected:
  olp::http::NetworkProxySettings GetLocalhostProxySettings() {
    return olp::http::NetworkProxySettings()
#if ANDROID
        .WithHostname("10.0.2.2")
#else
        .WithHostname("localhost")
#endif
        .WithPort(3000)
        .WithUsername("test_user")
        .WithPassword("test_password")
        .WithType(olp::http::NetworkProxySettings::Type::HTTP);
  }

  olp::client::OlpClientSettings CreateCatalogClientSettings() {
    const auto& parameter = MemoryTestBase<Param>::GetParam();

    std::shared_ptr<olp::thread::TaskScheduler> task_scheduler = nullptr;
    if (parameter.task_scheduler_capacity != 0) {
      task_scheduler =
          olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(
              parameter.task_scheduler_capacity);
    }

    auto network = std::make_shared<Http2HttpNetworkWrapper>();
    network->WithErrors(parameter.with_http_errors);
    network->WithTimeouts(parameter.with_network_timeouts);

    olp::client::AuthenticationSettings auth_settings;
    auth_settings.provider = []() { return "invalid"; };

    olp::client::OlpClientSettings client_settings;
    client_settings.authentication_settings = auth_settings;
    client_settings.task_scheduler = task_scheduler;
    client_settings.network_request_handler = std::move(network);
    client_settings.proxy_settings = GetLocalhostProxySettings();
    client_settings.cache =
        parameter.cache_factory ? parameter.cache_factory() : nullptr;
    client_settings.retry_settings.timeout = 1;
    //client_settings.retry_settings.max_attempts = 0;

    return client_settings;
  }
};

/*
 * Set error flags to test configuration.
 */
inline void SetErrorFlags(TestBaseConfiguration& configuration) {
  configuration.with_http_errors = true;
  configuration.with_network_timeouts = true;
}

/*
 * Set null cache configuration. Cache does not perform any operations.
 */
inline void SetNullCacheConfiguration(TestBaseConfiguration& configuration) {
  configuration.cache_factory = []() { return std::make_shared<NullCache>(); };
}

/*
 * Set default cache configuration. A simple default cache with default
 * settings.
 */
inline void SetDefaultCacheConfiguration(TestBaseConfiguration& configuration) {
  configuration.cache_factory = []() {
    return olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
  };
}

/*
 * Set a disk cache. Disk cache path is computed as `cache_location` env.
 * variable and /memory_test appended. Default cache settings.
 */
inline void SetDiskCacheConfiguration(TestBaseConfiguration& configuration,
                                      olp::cache::CacheSettings settings = {}) {
  auto location = CustomParameters::getArgument("cache_location");
  if (location.empty()) {
    location = olp::utils::Dir::TempDirectory() + "/memory_test";
  }

  settings.disk_path_mutable = location;

  configuration.cache_factory = [settings]() {
    return olp::client::OlpClientSettingsFactory::CreateDefaultCache(settings);
  };
}
