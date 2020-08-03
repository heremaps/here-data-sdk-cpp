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

namespace http = olp::http;
namespace dataservice_read = olp::dataservice::read;

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
    settings_->task_scheduler =
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler();
  }

  void TearDown() override {
    auto network = std::move(settings_->network_request_handler);
    settings_.reset();
    // when test ends we must be sure that network pointer is not captured
    // anywhere
    ASSERT_EQ(network.use_count(), 1);
  }

  std::string GetTestCatalog() {
    return CustomParameters::getArgument("dataservice_read_test_catalog");
  }

  template <typename T>
  T GetExecutionTime(std::function<T()> func) {
    auto start_time = std::chrono::high_resolution_clock::now();
    auto result = func();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time = end - start_time;
    std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;

    return result;
  }

  std::shared_ptr<olp::client::OlpClientSettings> settings_;
};

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataFromPartitionAsync) {
  auto catalog = olp::client::HRN::FromString(
      CustomParameters::getArgument("dataservice_read_test_versioned_catalog"));
  auto layer =
      CustomParameters::getArgument("dataservice_read_test_versioned_layer");

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, boost::none, *settings_);
  ASSERT_TRUE(catalog_client);

  std::promise<dataservice_read::DataResponse> promise;
  std::future<dataservice_read::DataResponse> future = promise.get_future();
  auto partition = CustomParameters::getArgument(
      "dataservice_read_test_versioned_partition");
  auto token = catalog_client->GetData(
      olp::dataservice::read::DataRequest().WithPartitionId(partition),
      [&promise](dataservice_read::DataResponse response) {
        promise.set_value(response);
      });

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  dataservice_read::DataResponse response = future.get();

  EXPECT_SUCCESS(response);
  ASSERT_TRUE(response.GetResult() != nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataFromPartitionLatestVersionAsync) {
  auto catalog = olp::client::HRN::FromString(
      CustomParameters::getArgument("dataservice_read_test_versioned_catalog"));
  auto layer =
      CustomParameters::getArgument("dataservice_read_test_versioned_layer");

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, boost::none, *settings_);
  ASSERT_TRUE(catalog_client);

  std::promise<dataservice_read::DataResponse> promise;
  std::future<dataservice_read::DataResponse> future = promise.get_future();
  auto partition = CustomParameters::getArgument(
      "dataservice_read_test_versioned_partition");
  auto token = catalog_client->GetData(
      olp::dataservice::read::DataRequest().WithPartitionId(partition),
      [&promise](dataservice_read::DataResponse response) {
        promise.set_value(response);
      });

  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  dataservice_read::DataResponse response = future.get();

  EXPECT_SUCCESS(response);
  ASSERT_TRUE(response.GetResult() != nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataFromPartitionSync) {
  settings_->task_scheduler.reset();

  auto catalog = olp::client::HRN::FromString(
      CustomParameters::getArgument("dataservice_read_test_versioned_catalog"));
  auto layer =
      CustomParameters::getArgument("dataservice_read_test_versioned_layer");

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          catalog, layer, boost::none, *settings_);
  ASSERT_TRUE(catalog_client);

  dataservice_read::DataResponse response;
  auto partition = CustomParameters::getArgument(
      "dataservice_read_test_versioned_partition");
  auto token = catalog_client->GetData(
      olp::dataservice::read::DataRequest().WithPartitionId(partition),
      [&response](dataservice_read::DataResponse resp) {
        response = std::move(resp);
      });
  EXPECT_SUCCESS(response);
  ASSERT_TRUE(response.GetResult() != nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);
}

TEST_F(DataserviceReadVersionedLayerClientTest, PrefetchWideRange) {
  const auto catalog =
      olp::client::HRN::FromString(CustomParameters::getArgument(
          "dataservice_read_test_versioned_prefetch_catalog"));
  const auto kLayerId = CustomParameters::getArgument(
      "dataservice_read_test_versioned_prefetch_layer");
  const auto kTileId = CustomParameters::getArgument(
      "dataservice_read_test_versioned_prefetch_tile");

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, kLayerId, boost::none, *settings_);

  {
    SCOPED_TRACE("Prefetch tiles online and store them in memory cache");
    std::vector<olp::geo::TileKey> tile_keys = {
        olp::geo::TileKey::FromHereTile(kTileId)};

    auto request = olp::dataservice::read::PrefetchTilesRequest()
                       .WithTileKeys(tile_keys)
                       .WithMinLevel(6)
                       .WithMaxLevel(12);

    std::promise<dataservice_read::PrefetchTilesResponse> promise;
    auto future = promise.get_future();
    auto token = client->PrefetchTiles(
        request, [&promise](dataservice_read::PrefetchTilesResponse response) {
          promise.set_value(std::move(response));
        });

    ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
    dataservice_read::PrefetchTilesResponse response = future.get();
    EXPECT_SUCCESS(response);
    ASSERT_FALSE(response.GetResult().empty());

    const auto& result = response.GetResult();

    for (auto tile_result : result) {
      EXPECT_SUCCESS(*tile_result);
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
    }
  }

  {
    SCOPED_TRACE("Read cached data from the same partition");
    std::promise<dataservice_read::DataResponse> promise;
    auto future = promise.get_future();
    client->GetData(olp::dataservice::read::TileRequest()
                        .WithTileKey(olp::geo::TileKey::FromHereTile(kTileId))
                        .WithFetchOption(dataservice_read::CacheOnly),
                    [&promise](dataservice_read::DataResponse resp) {
                      promise.set_value(std::move(resp));
                    });

    auto response = future.get();

    EXPECT_SUCCESS(response);
    ASSERT_TRUE(response.GetResult() != nullptr);
    ASSERT_NE(response.GetResult()->size(), 0u);
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest, PrefetchWrongLevels) {
  const auto catalog =
      olp::client::HRN::FromString(CustomParameters::getArgument(
          "dataservice_read_test_versioned_prefetch_catalog"));
  const auto kLayerId = CustomParameters::getArgument(
      "dataservice_read_test_versioned_prefetch_layer");
  const auto kTileId = CustomParameters::getArgument(
      "dataservice_read_test_versioned_prefetch_tile");

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, kLayerId, boost::none, *settings_);

  {
    SCOPED_TRACE("Prefetch tiles online and store them in memory cache");
    std::vector<olp::geo::TileKey> tile_keys = {
        olp::geo::TileKey::FromHereTile(kTileId)};

    {
      SCOPED_TRACE("min/max levels default");
      auto request =
          olp::dataservice::read::PrefetchTilesRequest().WithTileKeys(
              tile_keys);

      std::promise<dataservice_read::PrefetchTilesResponse> promise;
      auto future = promise.get_future();
      auto token = client->PrefetchTiles(
          request,
          [&promise](dataservice_read::PrefetchTilesResponse response) {
            promise.set_value(std::move(response));
          });

      ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
      dataservice_read::PrefetchTilesResponse response = future.get();
      EXPECT_SUCCESS(response);
      const auto& result = response.GetResult();

      for (auto tile_result : result) {
        EXPECT_SUCCESS(*tile_result);
        ASSERT_TRUE(tile_result->tile_key_.IsValid());
      }

      ASSERT_EQ(tile_keys.size(), result.size());
    }

    {
      SCOPED_TRACE(" min level greater than max level");
      auto request = olp::dataservice::read::PrefetchTilesRequest()
                         .WithTileKeys(tile_keys)
                         .WithMinLevel(-1)
                         .WithMaxLevel(0);

      std::promise<dataservice_read::PrefetchTilesResponse> promise;
      auto future = promise.get_future();
      auto token = client->PrefetchTiles(
          request,
          [&promise](dataservice_read::PrefetchTilesResponse response) {
            promise.set_value(std::move(response));
          });

      ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
      dataservice_read::PrefetchTilesResponse response = future.get();
      EXPECT_TRUE(response.IsSuccessful());
      const auto& result = response.GetResult();

      for (auto tile_result : result) {
        EXPECT_SUCCESS(*tile_result);
        ASSERT_TRUE(tile_result->tile_key_.IsValid());
      }

      ASSERT_EQ(tile_keys.size(), result.size());
    }
    {
      SCOPED_TRACE(" min/max levels invalid, but not equal");
      auto request = olp::dataservice::read::PrefetchTilesRequest()
                         .WithTileKeys(tile_keys)
                         .WithMinLevel(0)
                         .WithMaxLevel(-1);

      std::promise<dataservice_read::PrefetchTilesResponse> promise;
      auto future = promise.get_future();
      auto token = client->PrefetchTiles(
          request,
          [&promise](dataservice_read::PrefetchTilesResponse response) {
            promise.set_value(std::move(response));
          });

      ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
      dataservice_read::PrefetchTilesResponse response = future.get();
      EXPECT_SUCCESS(response);
      const auto& result = response.GetResult();

      for (auto tile_result : result) {
        EXPECT_SUCCESS(*tile_result);
        ASSERT_TRUE(tile_result->tile_key_.IsValid());
      }

      ASSERT_EQ(tile_keys.size(), result.size());
    }
    {
      SCOPED_TRACE(" min level is zero");
      auto request = olp::dataservice::read::PrefetchTilesRequest()
                         .WithTileKeys(tile_keys)
                         .WithMinLevel(0)
                         .WithMaxLevel(3);

      std::promise<dataservice_read::PrefetchTilesResponse> promise;
      auto future = promise.get_future();
      auto token = client->PrefetchTiles(
          request,
          [&promise](dataservice_read::PrefetchTilesResponse response) {
            promise.set_value(std::move(response));
          });

      ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
      dataservice_read::PrefetchTilesResponse response = future.get();
      ASSERT_TRUE(response.IsSuccessful());
      ASSERT_TRUE(response.GetResult().empty());
    }
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
      catalog, kLayerId, boost::none, *settings_);

  std::vector<olp::geo::TileKey> tile_keys = {
      olp::geo::TileKey::FromHereTile(kTileId)};

  auto request = olp::dataservice::read::PrefetchTilesRequest()
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(10)
                     .WithMaxLevel(12);
  auto cancel_future = client->PrefetchTiles(std::move(request));

  auto raw_future = cancel_future.GetFuture();
  ASSERT_NE(raw_future.wait_for(kWaitTimeout), std::future_status::timeout);
  dataservice_read::PrefetchTilesResponse response = raw_future.get();
  EXPECT_SUCCESS(response);
  ASSERT_FALSE(response.GetResult().empty());

  const auto& result = response.GetResult();

  for (auto tile_result : result) {
    EXPECT_SUCCESS(*tile_result);
    ASSERT_TRUE(tile_result->tile_key_.IsValid());
  }
  // one tile on level 10 and 11 and 4 on level 12
  ASSERT_EQ(6u, result.size());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitionsWithInvalidHrn) {
  olp::client::HRN hrn("hrn:here:data::olp-here-test:nope-test-v2");

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "testlayer", boost::none, *settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  auto partitions_response =
      GetExecutionTime<olp::dataservice::read::PartitionsResponse>([&] {
        auto future = catalog_client->GetPartitions(request);
        return future.GetFuture().get();
      });

  ASSERT_FALSE(partitions_response.IsSuccessful());
  ASSERT_EQ(http::HttpStatusCode::FORBIDDEN,
            partitions_response.GetError().GetHttpStatusCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitions) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "testlayer", boost::none, *settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  auto partitions_response =
      GetExecutionTime<olp::dataservice::read::PartitionsResponse>([&] {
        auto future = catalog_client->GetPartitions(request);
        return future.GetFuture().get();
      });

  EXPECT_SUCCESS(partitions_response);
  ASSERT_EQ(4u, partitions_response.GetResult().GetPartitions().size());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitionsForInvalidLayer) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "invalidLayer", boost::none, *settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  auto partitions_response =
      GetExecutionTime<olp::dataservice::read::PartitionsResponse>([&] {
        auto future = catalog_client->GetPartitions(request);
        return future.GetFuture().get();
      });

  ASSERT_FALSE(partitions_response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::BadRequest,
            partitions_response.GetError().GetErrorCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataWithInvalidHrn) {
  olp::client::HRN hrn("hrn:here:data::olp-here-test:nope-test-v2");

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "testlayer", boost::none, *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithDataHandle("d5d73b64-7365-41c3-8faf-aa6ad5bab135");
  auto data_response = GetExecutionTime<dataservice_read::DataResponse>([&] {
    auto future = catalog_client->GetData(request);
    return future.GetFuture().get();
  });

  ASSERT_FALSE(data_response.IsSuccessful());
  ASSERT_EQ(http::HttpStatusCode::FORBIDDEN,
            data_response.GetError().GetHttpStatusCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataWithHandle) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "testlayer", boost::none, *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithDataHandle("d5d73b64-7365-41c3-8faf-aa6ad5bab135");
  auto data_response = GetExecutionTime<dataservice_read::DataResponse>([&] {
    auto future = catalog_client->GetData(request);
    return future.GetFuture().get();
  });

  EXPECT_SUCCESS(data_response);
  ASSERT_TRUE(data_response.GetResult() != nullptr);
  ASSERT_LT(0, data_response.GetResult()->size());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ("DT_2_0031", data_string);
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataWithInvalidDataHandle) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "testlayer", boost::none, *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithDataHandle("invalidDataHandle");
  auto data_response = GetExecutionTime<dataservice_read::DataResponse>([&] {
    auto future = catalog_client->GetData(request);
    return future.GetFuture().get();
  });

  ASSERT_FALSE(data_response.IsSuccessful());
  ASSERT_EQ(http::HttpStatusCode::NOT_FOUND,
            data_response.GetError().GetHttpStatusCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataHandleWithInvalidLayer) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "invalidLayer", boost::none, *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithDataHandle("invalidDataHandle");
  auto data_response = GetExecutionTime<dataservice_read::DataResponse>([&] {
    auto future = catalog_client->GetData(request);
    return future.GetFuture().get();
  });

  ASSERT_FALSE(data_response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::NotFound,
            data_response.GetError().GetErrorCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataWithPartitionId) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "testlayer", boost::none, *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithPartitionId("269");
  auto data_response = GetExecutionTime<dataservice_read::DataResponse>([&] {
    auto future = catalog_client->GetData(request);
    return future.GetFuture().get();
  });

  EXPECT_SUCCESS(data_response);
  ASSERT_TRUE(data_response.GetResult() != nullptr);
  ASSERT_LT(0, data_response.GetResult()->size());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ("DT_2_0031", data_string);
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataWithPartitionIdVersion2) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "testlayer", 2, *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithPartitionId("269");
  auto data_response = GetExecutionTime<dataservice_read::DataResponse>([&] {
    auto future = catalog_client->GetData(request);
    return future.GetFuture().get();
  });

  EXPECT_SUCCESS(data_response);
  ASSERT_TRUE(data_response.GetResult() != nullptr);
  ASSERT_LT(0, data_response.GetResult()->size());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ("DT_2_0031", data_string);
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataWithPartitionIdInvalidVersion) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "testlayer", 10, *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithPartitionId("269");
  auto data_response = GetExecutionTime<dataservice_read::DataResponse>([&] {
    auto future = catalog_client->GetData(request);
    return future.GetFuture().get();
  });

  ASSERT_FALSE(data_response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::BadRequest,
            data_response.GetError().GetErrorCode());
  ASSERT_EQ(http::HttpStatusCode::BAD_REQUEST,
            data_response.GetError().GetHttpStatusCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitionsVersion2) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "testlayer", 2, *settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  auto partitions_response =
      GetExecutionTime<olp::dataservice::read::PartitionsResponse>([&] {
        auto future = catalog_client->GetPartitions(request);
        return future.GetFuture().get();
      });

  EXPECT_SUCCESS(partitions_response);
  ASSERT_LT(0, partitions_response.GetResult().GetPartitions().size());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetPartitionsInvalidVersion) {
  olp::client::HRN hrn(GetTestCatalog());

  {
    auto catalog_client =
        std::make_unique<olp::dataservice::read::VersionedLayerClient>(
            hrn, "testlayer", 10, *settings_);

    auto request = olp::dataservice::read::PartitionsRequest();
    auto partitions_response =
        GetExecutionTime<olp::dataservice::read::PartitionsResponse>([&] {
          auto future = catalog_client->GetPartitions(request);
          return future.GetFuture().get();
        });

    ASSERT_FALSE(partitions_response.IsSuccessful());
    ASSERT_EQ(olp::client::ErrorCode::BadRequest,
              partitions_response.GetError().GetErrorCode());
    ASSERT_EQ(http::HttpStatusCode::BAD_REQUEST,
              partitions_response.GetError().GetHttpStatusCode());
  }

  {
    auto catalog_client =
        std::make_unique<olp::dataservice::read::VersionedLayerClient>(
            hrn, "testlayer", -2, *settings_);
    auto request = olp::dataservice::read::PartitionsRequest();
    auto partitions_response =
        GetExecutionTime<olp::dataservice::read::PartitionsResponse>([&] {
          auto future = catalog_client->GetPartitions(request);
          return future.GetFuture().get();
        });

    ASSERT_FALSE(partitions_response.IsSuccessful());
    ASSERT_EQ(olp::client::ErrorCode::BadRequest,
              partitions_response.GetError().GetErrorCode());
    ASSERT_EQ(http::HttpStatusCode::BAD_REQUEST,
              partitions_response.GetError().GetHttpStatusCode());
  }
}

TEST_F(DataserviceReadVersionedLayerClientTest,
       GetDataWithNonExistentPartitionId) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "testlayer", boost::none, *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithPartitionId("noPartition");
  auto data_response = GetExecutionTime<dataservice_read::DataResponse>([&] {
    auto future = catalog_client->GetData(request);
    return future.GetFuture().get();
  });

  EXPECT_FALSE(data_response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::NotFound,
            data_response.GetError().GetErrorCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataWithInvalidLayerId) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "invalidLayer", boost::none, *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithPartitionId("269");
  auto data_response = GetExecutionTime<dataservice_read::DataResponse>([&] {
    auto future = catalog_client->GetData(request);
    return future.GetFuture().get();
  });

  ASSERT_FALSE(data_response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::BadRequest,
            data_response.GetError().GetErrorCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataWithEmptyField) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "testlayer", boost::none, *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithPartitionId("1");
  auto data_response = GetExecutionTime<dataservice_read::DataResponse>([&] {
    auto future = catalog_client->GetData(request);
    return future.GetFuture().get();
  });

  EXPECT_FALSE(data_response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::NotFound,
            data_response.GetError().GetErrorCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetDataCompressed) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "testlayer", boost::none, *settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithPartitionId("here_van_wc2018_pool");
  auto data_response = GetExecutionTime<dataservice_read::DataResponse>([&] {
    auto future = catalog_client->GetData(request);
    return future.GetFuture().get();
  });

  EXPECT_SUCCESS(data_response);
  ASSERT_TRUE(data_response.GetResult() != nullptr);
  ASSERT_LT(0u, data_response.GetResult()->size());

  catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "testlayer_gzip", boost::none, *settings_);
  auto request_compressed = olp::dataservice::read::DataRequest();
  request_compressed.WithPartitionId("here_van_wc2018_pool");
  auto data_response_compressed =
      GetExecutionTime<dataservice_read::DataResponse>([&] {
        auto future = catalog_client->GetData(request_compressed);
        return future.GetFuture().get();
      });

  EXPECT_SUCCESS(data_response_compressed);
  ASSERT_TRUE(data_response_compressed.GetResult() != nullptr);
  ASSERT_LT(0u, data_response_compressed.GetResult()->size());
  ASSERT_EQ(data_response.GetResult()->size(),
            data_response_compressed.GetResult()->size());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetTile) {
  const auto catalog =
      olp::client::HRN::FromString(CustomParameters::getArgument(
          "dataservice_read_test_versioned_prefetch_catalog"));
  const auto kLayerId = CustomParameters::getArgument(
      "dataservice_read_test_versioned_prefetch_layer");
  const auto kTileId = CustomParameters::getArgument(
      "dataservice_read_test_versioned_prefetch_tile");

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, kLayerId, boost::none, *settings_);

  auto request = olp::dataservice::read::TileRequest().WithTileKey(
      olp::geo::TileKey::FromHereTile(kTileId));

  auto data_response_compressed =
      GetExecutionTime<dataservice_read::DataResponse>([&] {
        auto future = client->GetData(request);
        return future.GetFuture().get();
      });

  EXPECT_SUCCESS(data_response_compressed);
  ASSERT_TRUE(data_response_compressed.GetResult() != nullptr);
  ASSERT_EQ(140u, data_response_compressed.GetResult()->size());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetTileWithInvalidLayerId) {
  const auto catalog =
      olp::client::HRN::FromString(CustomParameters::getArgument(
          "dataservice_read_test_versioned_prefetch_catalog"));
  const auto kTileId = CustomParameters::getArgument(
      "dataservice_read_test_versioned_prefetch_tile");

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, "invalidLayer", boost::none, *settings_);

  auto request = olp::dataservice::read::TileRequest().WithTileKey(
      olp::geo::TileKey::FromHereTile(kTileId));

  auto data_response = GetExecutionTime<dataservice_read::DataResponse>([&] {
    auto future = client->GetData(request);
    return future.GetFuture().get();
  });

  ASSERT_FALSE(data_response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::BadRequest,
            data_response.GetError().GetErrorCode());
}

TEST_F(DataserviceReadVersionedLayerClientTest, GetTileEmptyField) {
  const auto catalog =
      olp::client::HRN::FromString(CustomParameters::getArgument(
          "dataservice_read_test_versioned_prefetch_catalog"));
  const auto kLayerId = CustomParameters::getArgument(
      "dataservice_read_test_versioned_prefetch_layer");

  auto client = std::make_unique<olp::dataservice::read::VersionedLayerClient>(
      catalog, kLayerId, boost::none, *settings_);

  auto request = olp::dataservice::read::TileRequest().WithTileKey(
      olp::geo::TileKey::FromHereTile(""));

  auto data_response_compressed =
      GetExecutionTime<dataservice_read::DataResponse>([&] {
        auto future = client->GetData(request);
        return future.GetFuture().get();
      });

  EXPECT_FALSE(data_response_compressed.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::InvalidArgument,
            data_response_compressed.GetError().GetErrorCode());
}

}  // namespace
