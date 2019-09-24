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

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <mutex>

#include <olp/authentication/TokenProvider.h>

#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>

#include <olp/dataservice/read/CatalogClient.h>
#include <olp/dataservice/read/CatalogRequest.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/PartitionsRequest.h>

namespace {
struct CatalogClientTestConfiguration {
  int requests_per_second;
  int calling_thread_count;
  int task_scheduler_capacity;
  int runtime;
};

std::ostream& operator<<(std::ostream& stream,
                         const CatalogClientTestConfiguration& config) {
  return stream << "CatalogClientTestConfiguration("
                << ".calling_thread_count=" << config.calling_thread_count
                << ", .task_scheduler_capacity="
                << config.task_scheduler_capacity
                << ", .requests_per_second=" << config.requests_per_second
                << ", .runtime=" << config.runtime << ")";
}

const std::string kKeyId("123");
const std::string kKeySecret("456");
const std::string kLayerId("versioned_test_layer");
const std::string kPartitionId("123456789");
}  // namespace

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

  std::shared_ptr<olp::client::OlpClientSettings>
  CreateCatalogClientSettings() {
    auto parameter = GetParam();

    std::shared_ptr<olp::thread::TaskScheduler> task_scheduler = nullptr;
    if (parameter.task_scheduler_capacity != 0) {
      task_scheduler =
          olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(
              parameter.task_scheduler_capacity);
    }
    olp::authentication::Settings settings;
    settings.task_scheduler = task_scheduler;
    settings.network_request_handler = s_network;

    olp::client::AuthenticationSettings auth_settings;
    auth_settings.provider = []() { return "invalid"; };

    auto client_settings = std::make_shared<olp::client::OlpClientSettings>();
    client_settings->authentication_settings = auth_settings;
    client_settings->task_scheduler = task_scheduler;
    client_settings->network_request_handler = s_network;
    client_settings->proxy_settings = GetLocalhostProxySettings();

    return client_settings;
  }

 protected:
  static std::shared_ptr<olp::http::Network> s_network;
};

std::shared_ptr<olp::http::Network> CatalogClientTest::s_network;

namespace {

void TestClientThread(
    std::shared_ptr<olp::dataservice::read::CatalogClient> service_client,
    std::chrono::duration<double, std::milli> sleep_interval, int runtime) {
  auto start = std::chrono::steady_clock::now();
  int partition_id = 0;

  while (true) {
    partition_id++;

    auto request = olp::dataservice::read::DataRequest()
                       .WithLayerId(kLayerId)
                       .WithPartitionId(std::to_string(partition_id))
                       .WithBillingTag(boost::none);

    service_client->GetData(
        request, [&](olp::dataservice::read::DataResponse response) {
          if (!response.IsSuccessful()) {
            auto error_string = response.GetError().GetMessage();
            std::cout << "Error: " << error_string << "\n";
          }
        });

    std::this_thread::sleep_for(sleep_interval);

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    if (elapsed.count() > runtime * 1000) {
      return;
    }
  }
}
}  // namespace

TEST_P(CatalogClientTest, ReadPartitions) {
  auto parameter = GetParam();
  std::chrono::duration<double, std::milli> sleep_interval(
      1000. / parameter.requests_per_second);

  auto olp_client_settings = CreateCatalogClientSettings();
  olp::client::HRN hrn("hrn:here:data:::testhrn");

  std::vector<std::thread> client_threads;

  for (int client_id = 0; client_id < parameter.calling_thread_count;
       client_id++) {
    auto service_client =
        std::make_shared<olp::dataservice::read::CatalogClient>(
            hrn, olp_client_settings);
    auto thread = std::thread(TestClientThread, std::move(service_client),
                              sleep_interval, parameter.runtime);
    client_threads.push_back(std::move(thread));
  }

  for (auto& thread : client_threads) {
    thread.join();
  }
}

//   int requests_per_second;
//   int calling_thread_count;
//   int task_scheduler_capacity;
//   int runtime;
INSTANTIATE_TEST_SUITE_P(
    Performance, CatalogClientTest,
    ::testing::Values(CatalogClientTestConfiguration{400, 1, 20, 30},
                      CatalogClientTestConfiguration{1, 1, 0, 1}));
