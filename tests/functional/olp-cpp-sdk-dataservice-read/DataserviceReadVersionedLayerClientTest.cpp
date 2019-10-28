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
#include <string>

#include <olp/authentication/AuthenticationCredentials.h>
#include <olp/authentication/Settings.h>
#include <olp/authentication/TokenProvider.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include <olp/dataservice/read/VersionedLayerClient.h>
#include <testutils/CustomParameters.hpp>
#include "Utils.h"

using namespace olp::dataservice::read;
using namespace testing;

namespace {

constexpr auto kWaitTimeout = std::chrono::seconds(10);

class DataserviceReadVersionedLayerClientTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto network = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();

    auto appid =
        CustomParameters::getArgument("dataservice_read_test_versioned_appid");
    auto secret =
        CustomParameters::getArgument("dataservice_read_test_versioned_secret");
    olp::authentication::Settings auth_settings({appid, secret});
    auth_settings.network_request_handler = network;

    olp::authentication::TokenProviderDefault provider(auth_settings);
    olp::client::AuthenticationSettings auth_client_settings;
    auth_client_settings.provider = provider;

    settings_ = std::make_shared<olp::client::OlpClientSettings>();
    settings_->network_request_handler = network;
    settings_->authentication_settings = auth_client_settings;
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

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataFromPartitionAsync) {
  settings_->task_scheduler =
      olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);

  auto catalog = olp::client::HRN::FromString(
      CustomParameters::getArgument("dataservice_read_test_versioned_catalog"));
  auto layer =
      CustomParameters::getArgument("dataservice_read_test_versioned_layer");
  auto version = std::stoi(CustomParameters::getArgument(
      "dataservice_read_test_versioned_layer_version"));

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, *settings_);
  ASSERT_TRUE(catalog_client);

  std::promise<VersionedLayerClient::CallbackResponse> promise;
  std::future<VersionedLayerClient::CallbackResponse> future =
      promise.get_future();
  auto partition = CustomParameters::getArgument(
      "dataservice_read_test_versioned_partition");
  auto token = catalog_client->GetData(
      olp::dataservice::read::DataRequest()
          .WithVersion(version)
          .WithPartitionId(partition),
      [&promise](VersionedLayerClient::CallbackResponse response) {
        promise.set_value(response);
      });

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  VersionedLayerClient::CallbackResponse response = future.get();

  EXPECT_SUCCESS(response);
  ASSERT_TRUE(response.GetResult() != nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataFromPartitionLatestVersionAsync) {
  settings_->task_scheduler =
      olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);

  auto catalog = olp::client::HRN::FromString(
      CustomParameters::getArgument("dataservice_read_test_versioned_catalog"));
  auto layer =
      CustomParameters::getArgument("dataservice_read_test_versioned_layer");

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, *settings_);
  ASSERT_TRUE(catalog_client);

  std::promise<VersionedLayerClient::CallbackResponse> promise;
  std::future<VersionedLayerClient::CallbackResponse> future =
      promise.get_future();
  auto partition = CustomParameters::getArgument(
      "dataservice_read_test_versioned_partition");
  auto token = catalog_client->GetData(
      olp::dataservice::read::DataRequest().WithPartitionId(partition),
      [&promise](VersionedLayerClient::CallbackResponse response) {
        promise.set_value(response);
      });

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  VersionedLayerClient::CallbackResponse response = future.get();

  EXPECT_SUCCESS(response);
  ASSERT_TRUE(response.GetResult() != nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataFromPartitionSync) {
  auto catalog = olp::client::HRN::FromString(
      CustomParameters::getArgument("dataservice_read_test_versioned_catalog"));
  auto layer =
      CustomParameters::getArgument("dataservice_read_test_versioned_layer");
  auto version = 0;

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, *settings_);
  ASSERT_TRUE(catalog_client);

  VersionedLayerClient::CallbackResponse response;
  auto partition = CustomParameters::getArgument(
      "dataservice_read_test_versioned_partition");
  auto token = catalog_client->GetData(
      olp::dataservice::read::DataRequest()
          .WithVersion(version)
          .WithPartitionId(partition),
      [&response](VersionedLayerClient::CallbackResponse resp) {
        response = std::move(resp);
      });
  EXPECT_SUCCESS(response);
  ASSERT_TRUE(response.GetResult() != nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);
}

TEST_F(DataserviceReadVersionedLayerClientTest, Prefetch) {
  const auto catalog =
      olp::client::HRN::FromString(CustomParameters::getArgument(
          "dataservice_read_test_versioned_prefetch_catalog"));
  const auto kLayerId = CustomParameters::getArgument(
      "dataservice_read_test_versioned_prefetch_layer");
  const auto kTileId = CustomParameters::getArgument(
      "dataservice_read_test_versioned_prefetch_tile");

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, kLayerId, *settings_);

  {
    SCOPED_TRACE("Prefetch tiles online and store them in memory cache");
    std::vector<olp::geo::TileKey> tile_keys = {
        olp::geo::TileKey::FromHereTile(kTileId)};

    auto request = olp::dataservice::read::PrefetchTilesRequest()
                       .WithTileKeys(tile_keys)
                       .WithMinLevel(10)
                       .WithMaxLevel(12);

    std::promise<VersionedLayerClient::PrefetchTilesResponse> promise;
    std::future<VersionedLayerClient::PrefetchTilesResponse> future =
        promise.get_future();
    auto token = client->PrefetchTiles(
        request,
        [&promise](VersionedLayerClient::PrefetchTilesResponse response) {
          promise.set_value(std::move(response));
        });

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    VersionedLayerClient::PrefetchTilesResponse response = future.get();
    EXPECT_SUCCESS(response);
    ASSERT_FALSE(response.GetResult().empty());

    const auto& result = response.GetResult();

    for (auto tile_result : result) {
      EXPECT_SUCCESS(*tile_result);
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
    }

    ASSERT_EQ(6u, result.size());
  }

  {
    SCOPED_TRACE("Read cached data from the same partition");
    VersionedLayerClient::CallbackResponse response;
    auto token = client->GetData(
        olp::dataservice::read::DataRequest()
            .WithPartitionId(kTileId)
            .WithFetchOption(CacheOnly),
        [&response](VersionedLayerClient::CallbackResponse resp) {
          response = std::move(resp);
        });
    EXPECT_SUCCESS(response);
    ASSERT_TRUE(response.GetResult() != nullptr);
    ASSERT_NE(response.GetResult()->size(), 0u);
  }

  {
    SCOPED_TRACE("Read cached data from pre-fetched sub-partition #1");
    const auto kSubPartitionId1 = CustomParameters::getArgument(
        "dataservice_read_test_versioned_prefetch_subpartition1");
    VersionedLayerClient::CallbackResponse response;
    auto token = client->GetData(
        olp::dataservice::read::DataRequest()
            .WithPartitionId(kSubPartitionId1)
            .WithFetchOption(CacheOnly),
        [&response](VersionedLayerClient::CallbackResponse resp) {
          response = std::move(resp);
        });
    EXPECT_SUCCESS(response);
    ASSERT_TRUE(response.GetResult() != nullptr);
    ASSERT_NE(response.GetResult()->size(), 0u);
  }

  {
    SCOPED_TRACE("Read cached data from pre-fetched sub-partition #2");
    const auto kSubPartitionId2 = CustomParameters::getArgument(
        "dataservice_read_test_versioned_prefetch_subpartition2");
    VersionedLayerClient::CallbackResponse response;
    auto token = client->GetData(
        olp::dataservice::read::DataRequest()
            .WithPartitionId(kSubPartitionId2)
            .WithFetchOption(CacheOnly),
        [&response](VersionedLayerClient::CallbackResponse resp) {
          response = std::move(resp);
        });
    EXPECT_SUCCESS(response);
    ASSERT_TRUE(response.GetResult() != nullptr);
    ASSERT_NE(response.GetResult()->size(), 0u);
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, PrefetchWithCancellableFuture) {
  const auto catalog =
      olp::client::HRN::FromString(CustomParameters::getArgument(
          "dataservice_read_test_versioned_prefetch_catalog"));
  const auto kLayerId = CustomParameters::getArgument(
      "dataservice_read_test_versioned_prefetch_layer");
  const auto kTileId = CustomParameters::getArgument(
      "dataservice_read_test_versioned_prefetch_tile");

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, kLayerId, *settings_);

  std::vector<olp::geo::TileKey> tile_keys = {
      olp::geo::TileKey::FromHereTile(kTileId)};

  auto request = olp::dataservice::read::PrefetchTilesRequest()
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(10)
                     .WithMaxLevel(12);
  auto cancel_future = client->PrefetchTiles(std::move(request));

  auto raw_future = cancel_future.GetFuture();
  ASSERT_NE(raw_future.wait_for(kWaitTimeout), std::future_status::timeout);
  VersionedLayerClient::PrefetchTilesResponse response = raw_future.get();
  EXPECT_SUCCESS(response);
  ASSERT_FALSE(response.GetResult().empty());

  const auto& result = response.GetResult();

  for (auto tile_result : result) {
    EXPECT_SUCCESS(*tile_result);
    ASSERT_TRUE(tile_result->tile_key_.IsValid());
  }

  ASSERT_EQ(6u, result.size());
}

}  // namespace
