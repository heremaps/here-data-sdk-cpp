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

#include <chrono>
#include <memory>

#include <gtest/gtest.h>
#include <olp/core/client/HRN.h>
#include <olp/core/logging/Log.h>
#include <olp/dataservice/read/VersionedLayerClient.h>

#include "MemoryTestBase.h"

namespace {
using TestFunction = std::function<void(uint8_t thread_id)>;

struct TestConfiguration : public TestBaseConfiguration {
  std::string configuration_name;
  std::uint16_t requests_per_second = 3;
  std::uint8_t calling_thread_count = 5;
  std::chrono::seconds runtime = std::chrono::minutes(5);
  float cancelation_chance = 0.f;
};

std::ostream& operator<<(std::ostream& os, const TestConfiguration& config) {
  return os << "TestConfiguration("
            << ".configuration_name=" << config.configuration_name
            << ", .calling_thread_count=" << config.calling_thread_count
            << ", .task_scheduler_capacity=" << config.task_scheduler_capacity
            << ", .requests_per_second=" << config.requests_per_second
            << ", .runtime=" << config.runtime.count() << ")";
}

constexpr std::chrono::milliseconds GetSleepPeriod(uint32_t requests) {
  return std::chrono::milliseconds(1000 / requests);
}

bool ShouldCancel(const TestConfiguration& configuration) {
  float cancel_chance = configuration.cancelation_chance;
  cancel_chance = std::min(cancel_chance, 1.f);
  cancel_chance = std::max(cancel_chance, 0.f);
  int thresold = static_cast<int>(cancel_chance * 100);
  return rand() % 100 <= thresold;
}

constexpr auto kLogTag = "MemoryTest";
const olp::client::HRN kCatalog("hrn:here:data::olp-here-test:testhrn");
const std::string kVersionedLayerId("versioned_test_layer");

class MemoryTest : public MemoryTestBase<TestConfiguration> {
 public:
  void SetUp() override;
  void TearDown() override;

  void StartThreads(TestFunction test_body);
  void ReportError(const olp::client::ApiError& error);

  void RandomlyCancel(olp::client::CancellationToken token);

 protected:
  std::atomic<olp::http::RequestId> request_counter_;
  std::vector<std::thread> client_threads_;

  std::atomic_size_t total_requests_{0};
  std::atomic_size_t success_responses_{0};
  std::atomic_size_t failed_responses_{0};

  std::mutex errors_mutex_;
  std::map<int, int> errors_;
};

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

  OLP_SDK_LOG_CRITICAL_INFO_F(kLogTag,
                              "Test finished, total requests %zu, succeed "
                              "responses %zu, failed responses %zu",
                              total_requests_.load(), success_responses_.load(),
                              failed_responses_.load());

  for (const auto& error : errors_) {
    OLP_SDK_LOG_CRITICAL_INFO_F(kLogTag, "error %d - count %d", error.first,
                                error.second);
  }

  size_t total_requests = success_responses_.load() + failed_responses_.load();
  EXPECT_EQ(total_requests_.load(), total_requests);
}

void MemoryTest::StartThreads(TestFunction test_body) {
  const auto& parameter = GetParam();

  for (uint8_t i = 0; i < parameter.calling_thread_count; ++i) {
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

void MemoryTest::RandomlyCancel(olp::client::CancellationToken token) {
  const auto& parameter = GetParam();
  if (ShouldCancel(parameter)) {
    std::this_thread::sleep_for(std::chrono::microseconds(rand() % 3000));
    token.Cancel();
  }
}

///
/// VersionedLayerClient
///

/*
 * Test performs N requests per second for M duration. All requests are
 * unique. Total requests number calculated as:
 *
 * total_count = requests_per_second * calling_thread_count
 *
 * By default, the SDK initializes a memory cache with 1 MB capacity.
 * To run the test, you need to start a local OLP mock server first.
 * Valgrind, heaptrack, and other tools are used to collect the output.
 */
TEST_P(MemoryTest, ReadNPartitionsFromVersionedLayer) {
  // Enable only errors to have a short output.
  olp::logging::Log::setLevel(olp::logging::Level::Warning);

  const auto& parameter = GetParam();

  auto settings = CreateCatalogClientSettings();

  StartThreads([=](uint8_t /*thread_id*/) {
    olp::dataservice::read::VersionedLayerClient service_client(
        kCatalog, kVersionedLayerId, boost::none, settings);

    const auto end_timestamp =
        std::chrono::steady_clock::now() + parameter.runtime;

    while (end_timestamp > std::chrono::steady_clock::now()) {
      const auto partition_id = request_counter_.fetch_add(1);

      auto request = olp::dataservice::read::DataRequest().WithPartitionId(
          std::to_string(partition_id));
      total_requests_.fetch_add(1);
      auto token = service_client.GetData(
          request, [&](olp::dataservice::read::DataResponse response) {
            if (response.IsSuccessful()) {
              success_responses_.fetch_add(1);
            } else {
              failed_responses_.fetch_add(1);
              ReportError(response.GetError());
            }
          });

      RandomlyCancel(std::move(token));

      std::this_thread::sleep_for(
          GetSleepPeriod(parameter.requests_per_second));
    }
  });
}

TEST_P(MemoryTest, PrefetchPartitionsFromVersionedLayer) {
  // Enable only errors to have a short output.
  olp::logging::Log::setLevel(olp::logging::Level::Warning);

  const auto& parameter = GetParam();

  auto settings = CreateCatalogClientSettings();

  StartThreads([=](uint8_t /*thread_id*/) {
    olp::dataservice::read::VersionedLayerClient service_client(
        kCatalog, kVersionedLayerId, boost::none, settings);

    const auto end_timestamp =
        std::chrono::steady_clock::now() + parameter.runtime;

    while (end_timestamp > std::chrono::steady_clock::now()) {
      const auto level = 10;
      const auto tile_count = 1 << level;

      std::vector<olp::geo::TileKey> tile_keys = {
          olp::geo::TileKey::FromRowColumnLevel(rand() % tile_count,
                                                rand() % tile_count, level)};

      auto request = olp::dataservice::read::PrefetchTilesRequest()
                         .WithMaxLevel(level + 2)
                         .WithMinLevel(level)
                         .WithTileKeys(tile_keys);
      total_requests_.fetch_add(1);
      auto token = service_client.PrefetchTiles(
          std::move(request),
          [&](olp::dataservice::read::PrefetchTilesResponse response) {
            if (response.IsSuccessful()) {
              success_responses_.fetch_add(1);
            } else {
              failed_responses_.fetch_add(1);
              ReportError(response.GetError());
            }
          });

      RandomlyCancel(std::move(token));

      std::this_thread::sleep_for(
          GetSleepPeriod(parameter.requests_per_second));
    }
  });
}

/*
 * 10 hours stability test with default constructed cache.
 */
TestConfiguration LongRunningTest() {
  TestConfiguration configuration;
  SetErrorFlags(configuration);
  SetDiskCacheConfiguration(configuration);
  configuration.configuration_name = "10h_test";
  configuration.runtime = std::chrono::hours(10);
  configuration.cancelation_chance = 0.25f;
  return configuration;
}

/*
 * Short 5 minutes test to collect SDK allocations without cache.
 */
TestConfiguration ShortRunningTestWithNullCache() {
  TestConfiguration configuration;
  SetNullCacheConfiguration(configuration);
  configuration.configuration_name = "short_test_null_cache";
  return configuration;
}

/*
 * Short 5 minutes test to collect SDK allocations with in memory cache.
 */
TestConfiguration ShortRunningTestWithMemoryCache() {
  TestConfiguration configuration;
  SetDefaultCacheConfiguration(configuration);
  configuration.configuration_name = "short_test_memory_cache";
  return configuration;
}

/*
 * Short 5 minutes test to collect SDK allocations with both in memory cache
 * and disk cache.
 */
TestConfiguration ShortRunningTestWithMutableCache() {
  TestConfiguration configuration;
  SetDiskCacheConfiguration(configuration);
  configuration.configuration_name = "short_test_disk_cache";
  return configuration;
}

std::vector<TestConfiguration> Configurations() {
  std::vector<TestConfiguration> configurations;
  configurations.emplace_back(ShortRunningTestWithNullCache());
  configurations.emplace_back(ShortRunningTestWithMemoryCache());
  configurations.emplace_back(ShortRunningTestWithMutableCache());
  configurations.emplace_back(LongRunningTest());
  return configurations;
}

std::string TestName(const testing::TestParamInfo<TestConfiguration>& info) {
  return info.param.configuration_name;
}

INSTANTIATE_TEST_SUITE_P(MemoryUsage, MemoryTest,
                         ::testing::ValuesIn(Configurations()), TestName);
}  // namespace
