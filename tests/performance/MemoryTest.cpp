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

#include <chrono>
#include <memory>

#include <gtest/gtest.h>

#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include <olp/core/utils/Dir.h>
#include <olp/dataservice/read/VersionedLayerClient.h>
#include <testutils/CustomParameters.hpp>

#include "NetworkWrapper.h"
#include "NullCache.h"

namespace {

using CacheFactory =
    std::function<std::shared_ptr<olp::cache::KeyValueCache>()>;

struct TestConfiguration {
  std::string configuration_name;
  std::uint16_t requests_per_second;
  std::uint8_t calling_thread_count;
  std::uint8_t task_scheduler_capacity;
  std::chrono::seconds runtime;
  CacheFactory cache_factory;
};

std::ostream& operator<<(std::ostream& os, const TestConfiguration& config) {
  return os << "TestConfiguration("
            << ".configuration_name=" << config.configuration_name
            << ", .calling_thread_count=" << config.calling_thread_count
            << ", .task_scheduler_capacity=" << config.task_scheduler_capacity
            << ", .requests_per_second=" << config.requests_per_second
            << ", .runtime=" << config.runtime.count() << ")";
}

const olp::client::HRN kCatalog("hrn:here:data::olp-here-test:testhrn");
const std::string kVersionedLayerId("versioned_test_layer");

class MemoryTest : public ::testing::TestWithParam<TestConfiguration> {
 public:
  static void SetUpTestSuite();
  static void TearDownTestSuite();

  olp::http::NetworkProxySettings GetLocalhostProxySettings();

  olp::client::OlpClientSettings CreateCatalogClientSettings();

  void SetUp() override;

  void TearDown() override;

  using TestFunction = std::function<void(const std::uint8_t thread_id)>;

  void StartThreads(TestFunction test_body);

  void ReportError(const olp::client::ApiError& error);

 protected:
  static std::shared_ptr<olp::http::Network> s_network;

  std::atomic<olp::http::RequestId> request_counter_;
  std::vector<std::thread> client_threads_;

  std::atomic_size_t total_requests_{0};
  std::atomic_size_t success_responses_{0};
  std::atomic_size_t failed_responses_{0};

  std::mutex errors_mutex_;
  std::map<int, int> errors_;
};

std::shared_ptr<olp::http::Network> MemoryTest::s_network;

void MemoryTest::SetUpTestSuite() {
  auto network = std::make_shared<olp::tests::http::Http2HttpNetworkWrapper>();
  network->EnableErrors(true);
  network->EnableTimeouts(true);
  s_network = std::move(network);
}

void MemoryTest::TearDownTestSuite() { s_network.reset(); }

olp::http::NetworkProxySettings MemoryTest::GetLocalhostProxySettings() {
  return olp::http::NetworkProxySettings()
      .WithHostname("localhost")
      .WithPort(3000)
      .WithUsername("test_user")
      .WithPassword("test_password")
      .WithType(olp::http::NetworkProxySettings::Type::HTTP);
}

olp::client::OlpClientSettings MemoryTest::CreateCatalogClientSettings() {
  auto parameter = GetParam();

  std::shared_ptr<olp::thread::TaskScheduler> task_scheduler = nullptr;
  if (parameter.task_scheduler_capacity != 0) {
    task_scheduler =
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(
            parameter.task_scheduler_capacity);
  }

  olp::client::AuthenticationSettings auth_settings;
  auth_settings.provider = []() { return "invalid"; };

  olp::client::OlpClientSettings client_settings;
  client_settings.authentication_settings = auth_settings;
  client_settings.task_scheduler = task_scheduler;
  client_settings.network_request_handler = s_network;
  client_settings.proxy_settings = GetLocalhostProxySettings();
  client_settings.cache = parameter.cache_factory();
  client_settings.retry_settings.timeout = 1;

  return client_settings;
}

void MemoryTest::SetUp() {
  using namespace olp;
  request_counter_.store(
      static_cast<http::RequestId>(http::RequestIdConstants::RequestIdMin));
  total_requests_.store(0);
  success_responses_.store(0);
  failed_responses_.store(0);
  errors_.clear();
}

void MemoryTest::TearDown() {
  for (auto& thread : client_threads_) {
    thread.join();
  }
  client_threads_.clear();

  OLP_SDK_LOG_CRITICAL_INFO_F("MemoryTest",
                              "Test finished. Total requests %zu, succeed "
                              "responses %zu, failed responses %zu",
                              total_requests_.load(), success_responses_.load(),
                              failed_responses_.load());

  for (const auto& error : errors_) {
    OLP_SDK_LOG_CRITICAL_INFO_F("MemoryTest", "error %d - count %d",
                                error.first, error.second);
  }

  size_t total_requests = success_responses_.load() + failed_responses_.load();
  EXPECT_EQ(total_requests_.load(), total_requests);
}

void MemoryTest::StartThreads(TestFunction test_body) {
  using namespace olp;

  const auto& parameter = GetParam();

  for (std::uint8_t i = 0; i < parameter.calling_thread_count; ++i) {
    client_threads_.emplace_back(std::thread(test_body, i));
  }
}

void MemoryTest::ReportError(const olp::client::ApiError& error) {
  const auto error_code = static_cast<int>(error.GetErrorCode());

  std::lock_guard<std::mutex> lock(errors_mutex_);
  const auto error_it = errors_.find(error_code);
  if (error_it != errors_.end()) {
    error_it->second++;
  } else {
    errors_[error_code] = 1;
  }
}

/*
 * Test performs N requests per second for M duration. All requests are unique.
 * Total requests number calculated as:
 *
 * total_count = requests_per_second * calling_thread_count
 *
 * By default, SDK initialized in-memory cache with 1 MB capacity.
 * To run the test, you need to start a local OLP mock server first.
 * Valgrind, heaptrack, other tools are used to collect the output.
 */
TEST_P(MemoryTest, ReadNPartitionsFromVersionedLayer) {
  const auto& parameter = GetParam();

  const auto sleep_interval =
      std::chrono::milliseconds(1000 / parameter.requests_per_second);

  auto settings = CreateCatalogClientSettings();

  StartThreads([=](const std::uint8_t thread_id) {
    olp::dataservice::read::VersionedLayerClient service_client(
        kCatalog, kVersionedLayerId, settings);

    const auto end_timestamp =
        std::chrono::steady_clock::now() + parameter.runtime;

    while (end_timestamp > std::chrono::steady_clock::now()) {
      const auto partition_id = request_counter_.fetch_add(1);

      auto request = olp::dataservice::read::DataRequest().WithPartitionId(
          std::to_string(partition_id));
      total_requests_.fetch_add(1);
      service_client.GetData(
          request, [&](olp::dataservice::read::DataResponse response) {
            if (response.IsSuccessful()) {
              success_responses_.fetch_add(1);
            } else {
              failed_responses_.fetch_add(1);
              ReportError(response.GetError());
            }
          });

      std::this_thread::sleep_for(sleep_interval);
    }
  });
}

TEST_P(MemoryTest, PrefetchPartitionsFromVersionedLayer) {
  const auto& parameter = GetParam();

  const auto sleep_interval =
      std::chrono::milliseconds(1000 / parameter.requests_per_second);

  auto settings = CreateCatalogClientSettings();

  StartThreads([=](const std::uint8_t thread_id) {
    olp::dataservice::read::VersionedLayerClient service_client(
        kCatalog, kVersionedLayerId, settings);

    const auto end_timestamp =
        std::chrono::steady_clock::now() + parameter.runtime;

    while (end_timestamp > std::chrono::steady_clock::now()) {
      const auto partition_id = request_counter_.fetch_add(1);

      const auto level = 10;
      const auto tile_count = 1 << level;

      std::vector<olp::geo::TileKey> tile_keys = {
          olp::geo::TileKey::FromRowColumnLevel(rand() % tile_count,
                                                rand() % tile_count, level)};

      auto request = olp::dataservice::read::PrefetchTilesRequest()
                         .WithMaxLevel(level + 2)
                         .WithMinLevel(level)
                         .WithVersion(17)
                         .WithTileKeys(tile_keys);
      total_requests_.fetch_add(1);
      service_client.PrefetchTiles(
          std::move(request),
          [&](olp::dataservice::read::PrefetchTilesResponse response) {
            if (response.IsSuccessful()) {
              success_responses_.fetch_add(1);
            } else {
              failed_responses_.fetch_add(1);
              ReportError(response.GetError());
            }
          });

      std::this_thread::sleep_for(sleep_interval);
    }
  });
}

/*
 * 10 hours stability test with default constructed cache.
 */
TestConfiguration LongRunningTest() {
  return TestConfiguration{"10h_test", 3, 5, 5, std::chrono::hours(10),
                           nullptr};
}

/*
 * Short 5 minutes test to collect SDK allocations without cache.
 */
TestConfiguration ShortRunningTestWithNullCache() {
  auto cache_factory = []() { return std::make_shared<NullCache>(); };
  return TestConfiguration{
      "short_test_null_cache", 3, 5, 5, std::chrono::minutes(5),
      std::move(cache_factory)};
}

/*
 * Short 5 minutes test to collect SDK allocations with in memory cache.
 */
TestConfiguration ShortRunningTestWithMemoryCache() {
  using namespace olp;
  auto cache_factory = []() {
    cache::CacheSettings settings;
    return client::OlpClientSettingsFactory::CreateDefaultCache(settings);
  };
  return TestConfiguration{
      "short_test_memory_cache", 3, 5, 5, std::chrono::minutes(5),
      std::move(cache_factory)};
}

/*
 * Short 5 minutes test to collect SDK allocations with both in memory cache and
 * disk cache.
 */
TestConfiguration ShortRunningTestWithDiskCache() {
  using namespace olp;
  auto cache_factory = []() {
    cache::CacheSettings settings;
    auto location = CustomParameters::getArgument("cache_location");
    if (location.empty()) {
      location = utils::Dir::TempDirectory() + "/performance_test";
    }
    settings.disk_path = location;
    return client::OlpClientSettingsFactory::CreateDefaultCache(settings);
  };
  return TestConfiguration{
      "short_test_disk_cache", 3, 5, 5, std::chrono::minutes(5),
      std::move(cache_factory)};
}

std::vector<TestConfiguration> Configurations() {
  std::vector<TestConfiguration> configurations;
  configurations.emplace_back(ShortRunningTestWithNullCache());
  configurations.emplace_back(ShortRunningTestWithMemoryCache());
  configurations.emplace_back(ShortRunningTestWithDiskCache());
  configurations.emplace_back(LongRunningTest());
  return configurations;
}

std::string TestName(const testing::TestParamInfo<TestConfiguration>& info) {
  return info.param.configuration_name;
}

//  std::uint16_t requests_per_second;
//  std::uint8_t calling_thread_count;
//  std::uint8_t task_scheduler_capacity;
//  std::chrono::seconds runtime;
INSTANTIATE_TEST_SUITE_P(MemoryUsage, MemoryTest,
                         ::testing::ValuesIn(Configurations()), TestName);
}  // namespace
