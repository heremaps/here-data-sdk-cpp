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

  // One prefetch request for one tile with depth of four results in 341 tiles
  // to download. (1 + 4 + 16 + 64 + 256).
  // Five parallel prefetch requests for 2 tiles completes in ~95 sec.
  std::uint8_t calling_thread_count = 5;
  std::uint16_t number_of_tiles = 2;
};

std::ostream& operator<<(std::ostream& os, const TestConfiguration& config) {
  return os << "TestConfiguration("
            << ".configuration_name=" << config.configuration_name
            << ", .calling_thread_count=" << config.calling_thread_count
            << ", .task_scheduler_capacity=" << config.task_scheduler_capacity
            << ")";
}

constexpr auto kLogTag = "PrefetchTest";
const olp::client::HRN kCatalog("hrn:here:data::olp-here-test:testhrn");
const std::string kVersionedLayerId("versioned_test_layer");

class PrefetchTest : public MemoryTestBase<TestConfiguration> {
 public:
  void SetUp() override;
  void TearDown() override;

  void StartThreads(TestFunction test_body);
  void ReportError(const olp::client::ApiError& error);

 protected:
  std::atomic<olp::http::RequestId> request_counter_;
  std::vector<std::thread> client_threads_;

  std::atomic_size_t total_requests_{0};
  std::atomic_size_t success_responses_{0};
  std::atomic_size_t failed_responses_{0};

  std::mutex errors_mutex_;
  std::map<int, int> errors_;
};

void PrefetchTest::SetUp() {
  request_counter_.store(static_cast<olp::http::RequestId>(
      olp::http::RequestIdConstants::RequestIdMin));
  total_requests_.store(0);
  success_responses_.store(0);
  failed_responses_.store(0);
  errors_.clear();
}

void PrefetchTest::TearDown() {
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

void PrefetchTest::StartThreads(TestFunction test_body) {
  const auto& parameter = GetParam();

  for (uint8_t i = 0; i < parameter.calling_thread_count; ++i) {
    client_threads_.emplace_back(std::thread(test_body, i));
  }
}

void PrefetchTest::ReportError(const olp::client::ApiError& error) {
  const auto error_code = static_cast<int>(error.GetErrorCode());

  std::lock_guard<std::mutex> lock(errors_mutex_);
  const auto error_it = errors_.find(error_code);
  if (error_it != errors_.end()) {
    error_it->second++;
  } else {
    errors_[error_code] = 1;
  }
}

///
/// VersionedLayerClient
///
TEST_P(PrefetchTest, PrefetchPartitionsFromVersionedLayer) {
  // Enable only errors to have a short output.
  olp::logging::Log::setLevel(olp::logging::Level::Warning);

  const auto& parameter = GetParam();

  auto settings = CreateCatalogClientSettings();

  StartThreads([=](uint8_t thread_id) {
    olp::dataservice::read::VersionedLayerClient service_client(
        kCatalog, kVersionedLayerId, boost::none, settings);

    const auto level = 10;

    // Generate N tiles with diagonal col/rows (unique for each thread id)
    std::vector<olp::geo::TileKey> tile_keys(parameter.number_of_tiles);
    const auto offset = thread_id * parameter.number_of_tiles;
    for (auto index = 0; index < parameter.number_of_tiles; ++index) {
      tile_keys[index] =
          olp::geo::TileKey::FromRowColumnLevel(offset + index, offset, level);
    }

    auto request = olp::dataservice::read::PrefetchTilesRequest()
                       .WithMaxLevel(level + 4)
                       .WithMinLevel(level)
                       .WithTileKeys(tile_keys);

    total_requests_.fetch_add(1);
    auto result = service_client.PrefetchTiles(std::move(request));
    auto response = result.GetFuture().get();

    if (response.IsSuccessful()) {
      success_responses_.fetch_add(1);
    } else {
      failed_responses_.fetch_add(1);
      ReportError(response.GetError());
    }
  });
}

/*
 * Test to collect SDK allocations with in memory cache.
 */
TestConfiguration ShortRunningTestWithMemoryCache() {
  TestConfiguration configuration;
  SetDefaultCacheConfiguration(configuration);
  configuration.task_scheduler_capacity =
      configuration.calling_thread_count * 3;
  configuration.configuration_name = "short_test_memory_cache";
  return configuration;
}

/*
 * Test to collect SDK allocations with both in memory cache and disk cache.
 */
TestConfiguration ShortRunningTestWithMutableCache() {
  TestConfiguration configuration;
  SetDiskCacheConfiguration(configuration);
  configuration.task_scheduler_capacity =
      configuration.calling_thread_count * 3;
  configuration.configuration_name = "short_test_disk_cache";
  return configuration;
}

std::vector<TestConfiguration> Configurations() {
  std::vector<TestConfiguration> configurations;
  configurations.emplace_back(ShortRunningTestWithMemoryCache());
  configurations.emplace_back(ShortRunningTestWithMutableCache());
  return configurations;
}

std::string TestName(const testing::TestParamInfo<TestConfiguration>& info) {
  return info.param.configuration_name;
}

INSTANTIATE_TEST_SUITE_P(MemoryUsage, PrefetchTest,
                         ::testing::ValuesIn(Configurations()), TestName);
}  // namespace
