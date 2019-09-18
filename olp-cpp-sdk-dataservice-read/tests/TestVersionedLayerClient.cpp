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
#include <string>

#include <gmock/gmock.h>

#include <olp/authentication/AuthenticationCredentials.h>
#include <olp/authentication/Settings.h>
#include <olp/authentication/TokenProvider.h>

#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include <olp/core/client/OlpClientSettingsFactory.h>

#include <olp/dataservice/read/VersionedLayerClient.h>

#include "testutils/CustomParameters.hpp"

using namespace olp::dataservice::read;
using namespace testing;

namespace {

constexpr auto kWaitTimeout = std::chrono::seconds(1);

}  // namespace

class VersionedLayerClientTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto network = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();

    olp::authentication::Settings auth_settings;
    auth_settings.network_request_handler = network;

    olp::authentication::TokenProviderDefault provider(
        CustomParameters::getArgument("appid"),
        CustomParameters::getArgument("secret"), auth_settings);
    olp::client::AuthenticationSettings auth_client_settings;
    auth_client_settings.provider = provider;

    settings_ = std::make_shared<olp::client::OlpClientSettings>();
    settings_->network_request_handler = network;
    settings_->authentication_settings = auth_client_settings;
    settings_->task_scheduler =
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);
  }

  void TearDown() override {
    auto network = std::move(settings_->network_request_handler);
    settings_.reset();
    // when test ends we must be sure that network pointer is not captured
    // anywhere
    ASSERT_EQ(network.use_count(), 1);
  }

 protected:
  std::shared_ptr<olp::client::OlpClientSettings> settings_;
};

TEST_F(VersionedLayerClientTest, GetDataFromTestCatalog) {
  auto catalog = CustomParameters::getArgument("catalog");
  auto layer = CustomParameters::getArgument("layer");
  auto version = 0;

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          settings_, catalog, layer, version);
  ASSERT_TRUE(catalog_client);

  std::mutex m;
  std::condition_variable cv;
  auto partition = CustomParameters::getArgument("partition");
  auto token = catalog_client->GetDataByPartitionId(
      partition, [](DataResponse response) {
        EXPECT_TRUE(response.IsSuccessful());
        EXPECT_TRUE(response.GetResult() != nullptr);
        EXPECT_NE(response.GetResult()->size(), 0u);
      });
  std::unique_lock<std::mutex> lock(m);
  ASSERT_TRUE(cv.wait_for(lock, kWaitTimeout) == std::cv_status::no_timeout);
}
