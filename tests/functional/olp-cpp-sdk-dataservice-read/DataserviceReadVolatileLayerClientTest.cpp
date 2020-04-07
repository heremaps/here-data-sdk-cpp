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
#include <olp/dataservice/write/VolatileLayerClient.h>
#include <olp/dataservice/write/model/PublishPartitionDataRequest.h>
#include <olp/dataservice/write/model/StartBatchRequest.h>
#include <testutils/CustomParameters.hpp>

#include "olp/dataservice/read/PartitionsRequest.h"
#include "olp/dataservice/read/PrefetchTileResult.h"
#include "olp/dataservice/read/VolatileLayerClient.h"

using namespace olp::dataservice::read;
using namespace olp::dataservice::write::model;
using namespace testing;

namespace {

const auto kAppIdEnvName = "dataservice_read_volatile_test_appid";
const auto kAppSecretEnvName = "dataservice_read_volatile_test_secret";
const auto kCatalogEnvName = "dataservice_read_volatile_test_catalog";
const auto kLayerEnvName = "dataservice_read_volatile_layer";

const auto kPrefetchAppId = "dataservice_read_volatile_test_prefetch_appid";
const auto kPrefetchAppSecret =
    "dataservice_read_volatile_test_prefetch_secret";
const auto kPrefetchCatalog = "dataservice_read_volatile_test_prefetch_catalog";
const auto kPrefetchLayer = "dataservice_read_volatile_prefetch_layer";
const auto kPrefetchTile = "23618401";
const auto kPrefetchSubTile1 = "23618410";
const auto kPrefetchSubTile2 = "23618406";
const auto kPrefetchEndpoint = "endpoint";

// The limit for 100 retries is 10 minutes. Therefore, the wait time between
// retries is 6 seconds.
const auto kWaitBeforeRetry = std::chrono::seconds(6);
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

    // prefetch setup
    auto prefetch_app_id = CustomParameters::getArgument(kPrefetchAppId);
    auto prefetch_secret = CustomParameters::getArgument(kPrefetchAppSecret);
    prefetch_catalog_ = CustomParameters::getArgument(kPrefetchCatalog);
    prefetch_layer_ = CustomParameters::getArgument(kPrefetchLayer);

    olp::authentication::Settings prefetch_auth_settings(
        {prefetch_app_id, prefetch_secret});
    prefetch_auth_settings.token_endpoint_url =
        CustomParameters::getArgument(kPrefetchEndpoint);
    prefetch_auth_settings.network_request_handler = network;

    olp::client::AuthenticationSettings prefetch_auth_client_settings;
    prefetch_auth_client_settings.provider =
        olp::authentication::TokenProviderDefault(prefetch_auth_settings);

    prefetch_settings_.authentication_settings = prefetch_auth_client_settings;
    prefetch_settings_.network_request_handler = network;
    prefetch_settings_.task_scheduler =
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);
  }

  void TearDown() override {
    auto network = std::move(settings_.network_request_handler);
    // when test ends we must be sure that network pointer is not captured
    // anywhere. Also network is still used in authentication settings and in
    // TokenProvider internally so it needs to be cleared.
    settings_ = olp::client::OlpClientSettings();
    prefetch_settings_ = olp::client::OlpClientSettings();
    EXPECT_EQ(network.use_count(), 1);
  }

  std::string GetTestCatalog() {
    return CustomParameters::getArgument(kCatalogEnvName);
  }

  std::string GetTestLayer() {
    return CustomParameters::getArgument(kLayerEnvName);
  }

  void WritePrefetchTilesData() {
    auto hrn = olp::client::HRN{prefetch_catalog_};
    // write desired partitions into the layer
    olp::dataservice::write::VolatileLayerClient write_client(
        hrn, prefetch_settings_);

    {
      auto batch_request = StartBatchRequest().WithLayers({prefetch_layer_});

      auto response = write_client.StartBatch(batch_request).GetFuture().get();

      EXPECT_TRUE(response.IsSuccessful());
      ASSERT_TRUE(response.GetResult().GetId());
      ASSERT_NE("", response.GetResult().GetId().value());

      std::vector<PublishPartitionDataRequest> partition_requests;
      PublishPartitionDataRequest partition_request;
      partition_requests.push_back(
          partition_request.WithLayerId(prefetch_layer_)
              .WithPartitionId(kPrefetchTile));
      partition_requests.push_back(
          partition_request.WithPartitionId(kPrefetchSubTile1));
      partition_requests.push_back(
          partition_request.WithPartitionId(kPrefetchSubTile2));

      auto publish_to_batch_response =
          write_client.PublishToBatch(response.GetResult(), partition_requests)
              .GetFuture()
              .get();
      EXPECT_TRUE(publish_to_batch_response.IsSuccessful());

      // publish data blobs
      std::vector<unsigned char> data = {1, 2, 3};
      auto data_ptr = std::make_shared<std::vector<unsigned char>>(data);
      for (auto& partition : partition_requests) {
        partition.WithData(data_ptr);
        auto publish_data_response =
            write_client.PublishPartitionData(partition).GetFuture().get();
        EXPECT_TRUE(publish_data_response.IsSuccessful());
      }

      auto complete_batch_response =
          write_client.CompleteBatch(response.GetResult()).GetFuture().get();
      EXPECT_TRUE(complete_batch_response.IsSuccessful());

      olp::dataservice::write::GetBatchResponse get_batch_response;
      for (int i = 0; i < 100; ++i) {
        get_batch_response =
            write_client.GetBatch(response.GetResult()).GetFuture().get();

        EXPECT_TRUE(get_batch_response.IsSuccessful());
        ASSERT_EQ(response.GetResult().GetId().value(),
                  get_batch_response.GetResult().GetId().value());
        if (get_batch_response.GetResult().GetDetails()->GetState() !=
            "succeeded") {
          ASSERT_EQ("submitted",
                    get_batch_response.GetResult().GetDetails()->GetState());
          std::this_thread::sleep_for(kWaitBeforeRetry);
        } else {
          break;
        }
      }

      ASSERT_EQ("succeeded",
                get_batch_response.GetResult().GetDetails()->GetState());
    }
  }

 protected:
  olp::client::OlpClientSettings settings_;
  olp::client::OlpClientSettings prefetch_settings_;
  std::string prefetch_catalog_;
  std::string prefetch_layer_;
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
    SCOPED_TRACE("Get Partitions Online if not found");

    VolatileLayerClient client(hrn, GetTestLayer(), settings_);

    olp::client::Condition condition;
    PartitionsResponse partitions_response;

    auto callback = [&](PartitionsResponse response) {
      partitions_response = std::move(response);
      condition.Notify();
    };
    client.GetPartitions(
        PartitionsRequest().WithFetchOption(FetchOptions::OnlineIfNotFound),
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

TEST_F(VolatileLayerClientTest, Prefetch) {
  WritePrefetchTilesData();

  olp::client::HRN hrn(prefetch_catalog_);
  VolatileLayerClient client(hrn, prefetch_layer_, prefetch_settings_);
  {
    SCOPED_TRACE("Prefetch tiles online and store them in memory cache");
    std::vector<olp::geo::TileKey> tile_keys = {
        olp::geo::TileKey::FromHereTile(kPrefetchTile)};
    std::vector<olp::geo::TileKey> expected_tile_keys = {
        olp::geo::TileKey::FromHereTile(kPrefetchTile),
        olp::geo::TileKey::FromHereTile(kPrefetchSubTile1),
        olp::geo::TileKey::FromHereTile(kPrefetchSubTile2)};

    auto request = olp::dataservice::read::PrefetchTilesRequest()
                       .WithTileKeys(tile_keys)
                       .WithMinLevel(10)
                       .WithMaxLevel(12);

    auto future = client.PrefetchTiles(request).GetFuture();

    ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
    PrefetchTilesResponse response = future.get();
    EXPECT_TRUE(response.IsSuccessful());
    ASSERT_FALSE(response.GetResult().empty());

    const auto& result = response.GetResult();

    for (auto tile_result : result) {
      EXPECT_TRUE(tile_result->IsSuccessful());
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
      auto it = std::find(expected_tile_keys.begin(), expected_tile_keys.end(),
                          tile_result->tile_key_);
      ASSERT_NE(it, expected_tile_keys.end());
    }

    ASSERT_EQ(expected_tile_keys.size(), result.size());
  }

  {
    SCOPED_TRACE("min/max levels are 0");

    std::vector<olp::geo::TileKey> tile_keys = {
        olp::geo::TileKey::FromHereTile(kPrefetchTile)};
    auto request = olp::dataservice::read::PrefetchTilesRequest()
                       .WithTileKeys(tile_keys)
                       .WithMinLevel(0)
                       .WithMaxLevel(0);

    auto future = client.PrefetchTiles(request).GetFuture();

    ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
    PrefetchTilesResponse response = future.get();
    EXPECT_TRUE(response.IsSuccessful());
    const auto& result = response.GetResult();

    for (auto tile_result : result) {
      EXPECT_TRUE(tile_result->IsSuccessful());
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
      auto it =
          std::find(tile_keys.begin(), tile_keys.end(), tile_result->tile_key_);
      ASSERT_NE(it, tile_keys.end());
    }

    ASSERT_EQ(tile_keys.size(), result.size());
  }

  {
    SCOPED_TRACE("min/max levels are equal");

    std::vector<olp::geo::TileKey> tile_keys = {
        olp::geo::TileKey::FromHereTile(kPrefetchTile)};
    auto request = olp::dataservice::read::PrefetchTilesRequest()
                       .WithTileKeys(tile_keys)
                       .WithMinLevel(12)
                       .WithMaxLevel(12);

    auto future = client.PrefetchTiles(request).GetFuture();

    ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
    PrefetchTilesResponse response = future.get();
    EXPECT_TRUE(response.IsSuccessful());
    const auto& result = response.GetResult();

    for (auto tile_result : result) {
      EXPECT_TRUE(tile_result->IsSuccessful());
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
      auto it =
          std::find(tile_keys.begin(), tile_keys.end(), tile_result->tile_key_);
      ASSERT_NE(it, tile_keys.end());
    }

    ASSERT_EQ(tile_keys.size(), result.size());
  }
}

}  // namespace
