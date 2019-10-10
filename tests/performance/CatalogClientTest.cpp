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

#include "olp/core/cache/KeyValueCache.h"
#include "olp/core/client/HRN.h"
#include "olp/core/client/OlpClientSettings.h"
#include "olp/core/client/OlpClientSettingsFactory.h"
#include "olp/core/logging/Log.h"
#include "olp/dataservice/read/CatalogClient.h"
#include "olp/dataservice/read/DataRequest.h"

namespace {
struct CatalogClientTestConfiguration {
  std::uint16_t requests_per_second;
  std::uint8_t calling_thread_count;
  std::uint8_t task_scheduler_capacity;
  std::chrono::seconds runtime;
};

std::ostream& operator<<(std::ostream& os,
                         const CatalogClientTestConfiguration& config) {
  return os << "CatalogClientTestConfiguration("
            << ".calling_thread_count=" << config.calling_thread_count
            << ", .task_scheduler_capacity=" << config.task_scheduler_capacity
            << ", .requests_per_second=" << config.requests_per_second
            << ", .runtime=" << config.runtime.count() << ")\n";
}

const std::string kVersionedLayerId("versioned_test_layer");

class CatalogClientTest
    : public ::testing::TestWithParam<CatalogClientTestConfiguration> {
 public:
  static void SetUpTestSuite() {
    s_network = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();
  }
  static void TearDownTestSuite() { s_network.reset(); }

  olp::http::NetworkProxySettings GetLocalhostProxySettings() {
    return olp::http::NetworkProxySettings()
        .WithHostname("http://localhost:3000")
        .WithUsername("test_user")
        .WithPassword("test_password")
        .WithType(olp::http::NetworkProxySettings::Type::HTTP);
  }

  olp::client::OlpClientSettings CreateCatalogClientSettings() {
    auto parameter = GetParam();

    std::shared_ptr<olp::thread::TaskScheduler> task_scheduler = nullptr;
    if (parameter.task_scheduler_capacity != 0) {
      task_scheduler =
          olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(
              parameter.task_scheduler_capacity);
    }

    olp::client::AuthenticationSettings auth_settings;
    auth_settings.provider = []() { return "invalid"; };

    olp::cache::CacheSettings cache_settings{};
    olp::client::OlpClientSettings client_settings;
    client_settings.authentication_settings = auth_settings;
    client_settings.task_scheduler = task_scheduler;
    client_settings.network_request_handler = s_network;
    client_settings.proxy_settings = GetLocalhostProxySettings();
    client_settings.cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache(
            cache_settings);

    return client_settings;
  }

 protected:
  static std::shared_ptr<olp::http::Network> s_network;
};

std::shared_ptr<olp::http::Network> CatalogClientTest::s_network;

namespace {

void ClientThread(
    const std::uint8_t client_id,
    std::shared_ptr<olp::dataservice::read::CatalogClient> service_client,
    std::string layer_id, std::chrono::milliseconds sleep_interval,
    std::chrono::seconds runtime,
    std::atomic<olp::http::RequestId>& request_counter) {
  std::atomic_int success_responses{0};
  std::atomic_int failed_responses{0};

  std::mutex errors_mutex;
  std::map<int, int> errors;

  const auto end_timestamp = std::chrono::steady_clock::now() + runtime;

  while (end_timestamp > std::chrono::steady_clock::now()) {
    const auto partition_id = request_counter.fetch_add(1);

    auto request = olp::dataservice::read::DataRequest()
                       .WithLayerId(layer_id)
                       .WithPartitionId(std::to_string(partition_id));

    service_client->GetData(
        request, [&](olp::dataservice::read::DataResponse response) {
          if (response.IsSuccessful()) {
            success_responses.fetch_add(1);
          } else {
            failed_responses.fetch_add(1);

            // Collect errors information
            std::lock_guard<std::mutex> lock(errors_mutex);
            const auto& error = response.GetError();
            const auto error_code = static_cast<int>(error.GetErrorCode());
            const auto error_it = errors.find(error_code);
            if (error_it != errors.end()) {
              error_it->second++;
            } else {
              errors[error_code] = 1;
            }
          }
        });

    std::this_thread::sleep_for(sleep_interval);
  }

  OLP_SDK_LOG_CRITICAL_INFO_F(
      "ClientThread",
      "Client %d finished, succeed responses %d, failed responses %d",
      client_id, success_responses.load(), failed_responses.load());

  for (const auto& error : errors) {
    OLP_SDK_LOG_CRITICAL_INFO_F("ClientThread",
                                "Client %d, error %d - count %d", client_id,
                                error.first, error.second);
  }
}
}  // namespace

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
TEST_P(CatalogClientTest, ReadNPartitionsFromVersionedLayer) {
  using namespace olp;

  const auto& parameter = GetParam();

  const auto sleep_interval =
      std::chrono::milliseconds(1000 / parameter.requests_per_second);

  const auto client_settings = CreateCatalogClientSettings();

  client::HRN hrn("hrn:here:data:::testhrn");

  std::vector<std::thread> client_threads;

  std::atomic<http::RequestId> request_counter;

  request_counter.store(
      static_cast<http::RequestId>(http::RequestIdConstants::RequestIdMin));

  for (std::uint8_t i = 0; i < parameter.calling_thread_count; ++i) {
    // Will be removed after API alignment
    auto service_client = std::make_shared<dataservice::read::CatalogClient>(
        hrn, client_settings);
    auto thread = std::thread(ClientThread, i, std::move(service_client),
                              kVersionedLayerId, sleep_interval,
                              parameter.runtime, std::ref(request_counter));
    client_threads.push_back(std::move(thread));
  }

  for (auto& thread : client_threads) {
    thread.join();
  }
}

//  std::uint16_t requests_per_second;
//  std::uint8_t calling_thread_count;
//  std::uint8_t task_scheduler_capacity;
//  std::chrono::seconds runtime;
INSTANTIATE_TEST_SUITE_P(MemoryUsage, CatalogClientTest,
                         ::testing::Values(CatalogClientTestConfiguration{
                             3, 5, 5, std::chrono::hours(10)}));
}  // namespace
