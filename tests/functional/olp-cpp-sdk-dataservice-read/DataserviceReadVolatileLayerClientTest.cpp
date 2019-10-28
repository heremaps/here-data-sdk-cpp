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
#include <iostream>
#include <string>

#include <gtest/gtest.h>
#include <olp/authentication/Settings.h>
#include <olp/authentication/TokenProvider.h>
#include <olp/core/cache/DefaultCache.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/Condition.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include <testutils/CustomParameters.hpp>

#include "olp/dataservice/read/PartitionsRequest.h"
#include "olp/dataservice/read/VolatileLayerClient.h"

using namespace olp::dataservice::read;
using namespace testing;

namespace {

const auto kAppIdEnvName = "dataservice_read_test_appid";
const auto kAppSecretEnvName = "dataservice_read_test_secret";
const auto kCatalogEnvName = "dataservice_read_test_catalog";
const auto kLayerEnvName = "dataservice_read_volatile_layer";
const auto kTimeout = std::chrono::seconds(5);

class VolatileLayerClientTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto network = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();

    const auto key_id = CustomParameters::getArgument(kAppIdEnvName);
    const auto secret = CustomParameters::getArgument(kAppSecretEnvName);

    olp::authentication::Settings authentication_settings({key_id, secret});
    authentication_settings.network_request_handler = network;

    olp::authentication::TokenProviderDefault provider(authentication_settings);
    olp::client::AuthenticationSettings auth_client_settings;
    auth_client_settings.provider = provider;
    settings_ = olp::client::OlpClientSettings();
    settings_.network_request_handler = network;
    settings_.authentication_settings = auth_client_settings;
    olp::cache::CacheSettings cache_settings;
    settings_.cache = olp::client::OlpClientSettingsFactory::CreateDefaultCache(
        cache_settings);
  }

  void TearDown() override {
    auto network = std::move(settings_.network_request_handler);
    // when test ends we must be sure that network pointer is not captured
    // anywhere. Also network is still used in authentication settings and in
    // TokenProvider internally so it needs to be cleared.
    settings_ = olp::client::OlpClientSettings();
    EXPECT_EQ(network.use_count(), 1);
  }

  std::string GetTestCatalog() {
    return CustomParameters::getArgument(kCatalogEnvName);
  }

  std::string GetTestLayer() {
    return CustomParameters::getArgument(kLayerEnvName);
  }

 protected:
  olp::client::OlpClientSettings settings_;
};

TEST_F(VolatileLayerClientTest, GetPartitions) {
  olp::client::HRN hrn(GetTestCatalog());
  {
    SCOPED_TRACE("Get Partitions Test");

    VolatileLayerClient client(hrn, GetTestLayer(), settings_);

    olp::client::Condition condition;
    PartitionsResponse partitions_response;

    auto callback = [&](PartitionsResponse response) {
      partitions_response = std::move(response);
      condition.Notify();
    };
    client.GetPartitions(PartitionsRequest(), std::move(callback));
    ASSERT_TRUE(condition.Wait(kTimeout));
    EXPECT_TRUE(partitions_response.IsSuccessful());
  }

  {
    SCOPED_TRACE("Get Partitions Test With CacheAndUpdate option");

    VolatileLayerClient client(hrn, GetTestLayer(), settings_);

    olp::client::Condition condition;
    PartitionsResponse partitions_response;

    auto callback = [&](PartitionsResponse response) {
      partitions_response = std::move(response);
      condition.Notify();
    };
    client.GetPartitions(
        PartitionsRequest().WithFetchOption(FetchOptions::CacheWithUpdate),
        std::move(callback));
    ASSERT_TRUE(condition.Wait(kTimeout));
    EXPECT_TRUE(partitions_response.IsSuccessful());
  }

  {
    SCOPED_TRACE("Get Partitions Invalid Layer Test");

    VolatileLayerClient client(hrn, "InvalidLayer", settings_);

    olp::client::Condition condition;
    PartitionsResponse partitions_response;

    auto callback = [&](PartitionsResponse response) {
      partitions_response = std::move(response);
      condition.Notify();
    };
    client.GetPartitions(PartitionsRequest(), std::move(callback));
    ASSERT_TRUE(condition.Wait(kTimeout));
    EXPECT_FALSE(partitions_response.IsSuccessful());
  }

  {
    SCOPED_TRACE("Get Partitions Invalid HRN Test");

    VolatileLayerClient client(olp::client::HRN("Invalid"), GetTestLayer(),
                               settings_);

    olp::client::Condition condition;
    PartitionsResponse partitions_response;

    auto callback = [&](PartitionsResponse response) {
      partitions_response = std::move(response);
      condition.Notify();
    };
    client.GetPartitions(PartitionsRequest(), std::move(callback));
    ASSERT_TRUE(condition.Wait(kTimeout));
    EXPECT_FALSE(partitions_response.IsSuccessful());
  }
}

TEST_F(VolatileLayerClientTest, GetPartitionsDifferentFetchOptions) {
  olp::client::HRN hrn(GetTestCatalog());
  {
    SCOPED_TRACE("Get Partitions Online Only");

    VolatileLayerClient client(hrn, GetTestLayer(), settings_);

    olp::client::Condition condition;
    PartitionsResponse partitions_response;

    auto callback = [&](PartitionsResponse response) {
      partitions_response = std::move(response);
      condition.Notify();
    };
    client.GetPartitions(
        PartitionsRequest().WithFetchOption(FetchOptions::OnlineOnly),
        std::move(callback));
    ASSERT_TRUE(condition.Wait(kTimeout));
    EXPECT_TRUE(partitions_response.IsSuccessful());
  }
  {
    SCOPED_TRACE("Get Partitions Cache Only");

    VolatileLayerClient client(hrn, GetTestLayer(), settings_);

    olp::client::Condition condition;
    PartitionsResponse partitions_response;

    auto callback = [&](PartitionsResponse response) {
      partitions_response = std::move(response);
      condition.Notify();
    };
    client.GetPartitions(
        PartitionsRequest().WithFetchOption(FetchOptions::CacheOnly),
        std::move(callback));
    ASSERT_TRUE(condition.Wait(kTimeout));
    EXPECT_TRUE(partitions_response.IsSuccessful());
  }
}

}  // namespace
