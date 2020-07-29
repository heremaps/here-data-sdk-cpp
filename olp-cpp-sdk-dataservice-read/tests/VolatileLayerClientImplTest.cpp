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

#include <matchers/NetworkUrlMatchers.h>
#include <mocks/CacheMock.h>
#include <mocks/NetworkMock.h>
#include <olp/core/cache/CacheSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/dataservice/read/CatalogClient.h>
#include <olp/dataservice/read/PrefetchTileResult.h>
#include "VolatileLayerClientImpl.h"

namespace {
namespace read = olp::dataservice::read;
namespace model = olp::dataservice::read::model;
using ::testing::_;
using ::testing::Mock;

constexpr auto kUrlVolatileBlobData =
    R"(https://volatile-blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/testlayer/data/4eed6ed1-0d32-43b9-ae79-043cb4256432)";

constexpr auto kUrlLookup =
    R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data::olp-here-test:hereos-internal-test-v2/apis)";

constexpr auto kUrlQueryPartition269 =
    R"(https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2/layers/testlayer/partitions?partition=269)";

constexpr auto kUrlQuadTreeIndexVolatile =
    R"(https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2/layers/testlayer/quadkeys/92259/depths/4)";

constexpr auto kUrlQuadTreeIndexVolatile2 =
    R"(https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2/layers/testlayer/quadkeys/23064/depths/4)";

constexpr auto kHttpResponseLookup =
    R"jsonString([{"api":"query","version":"v1","baseURL":"https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2","parameters":{}},
    {"api":"volatile-blob","version":"v1","baseURL":"https://volatile-blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString";

constexpr auto kHttpResponsePartition269 =
    R"jsonString({ "partitions": [{"version":4,"partition":"269","layer":"testlayer","dataHandle":"4eed6ed1-0d32-43b9-ae79-043cb4256432"}]})jsonString";

constexpr auto kHttpResponseNoPartition =
    R"jsonString({ "partitions": []})jsonString";

constexpr auto kHttpResponseQuadTreeIndexVolatile =
    R"jsonString( {"subQuads": [{"version":4,"subQuadKey":"1","dataHandle":"f9a9fd8e-eb1b-48e5-bfdb-4392b3826443"}, {"version":4,"subQuadKey":"2","dataHandle":"e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1"}],"parentQuads": [{"version":4,"partition":"1476147","dataHandle":"95c5c703-e00e-4c38-841e-e419367474f1"}]})jsonString";

constexpr auto kBlobDataHandle = R"(4eed6ed1-0d32-43b9-ae79-043cb4256432)";

constexpr auto kUrlPrefetchBlobData1 =
    R"(https://volatile-blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/testlayer/data/f9a9fd8e-eb1b-48e5-bfdb-4392b3826443)";

const std::string kCatalog =
    "hrn:here:data::olp-here-test:hereos-internal-test-v2";
const std::string kLayerId = "testlayer";
const auto kHrn = olp::client::HRN::FromString(kCatalog);
const auto kPartitionId = "269";
const auto kTileId = "5904591";
const auto kData1 = "SomeData1";
const auto kTimeout = std::chrono::seconds(5);

template <class T>
void SetupNetworkExpectation(NetworkMock& network_mock, T url, T response,
                             int status) {
  EXPECT_CALL(network_mock, Send(IsGetRequest(url), _, _, _, _))
      .WillOnce(ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(status), response));
}

MATCHER_P(IsKey, key, "") { return key == arg; }

TEST(VolatileLayerClientImplTest, GetData) {
  std::shared_ptr<NetworkMock> network_mock = std::make_shared<NetworkMock>();
  std::shared_ptr<CacheMock> cache_mock = std::make_shared<CacheMock>();
  olp::client::OlpClientSettings settings;
  settings.network_request_handler = network_mock;
  settings.cache = cache_mock;
  read::VolatileLayerClientImpl client(kHrn, kLayerId, settings);

  {
    SCOPED_TRACE("Get Data with DataHandle");
    EXPECT_CALL(*network_mock, Send(IsGetRequest(kUrlLookup), _, _, _, _))
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kHttpResponseLookup));

    SetupNetworkExpectation(*network_mock, kUrlVolatileBlobData, "someData",
                            olp::http::HttpStatusCode::OK);

    std::promise<read::DataResponse> promise;
    std::future<read::DataResponse> future(promise.get_future());

    auto token = client.GetData(
        read::DataRequest().WithDataHandle(kBlobDataHandle),
        [&](read::DataResponse response) { promise.set_value(response); });

    EXPECT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    ASSERT_TRUE(response.IsSuccessful());

    Mock::VerifyAndClearExpectations(network_mock.get());
  }

  {
    SCOPED_TRACE("Get Data with PartitionId");

    EXPECT_CALL(*network_mock, Send(IsGetRequest(kUrlLookup), _, _, _, _))
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kHttpResponseLookup));
    SetupNetworkExpectation(*network_mock, kUrlQueryPartition269,
                            kHttpResponsePartition269,
                            olp::http::HttpStatusCode::OK);

    SetupNetworkExpectation(*network_mock, kUrlVolatileBlobData, "someData",
                            olp::http::HttpStatusCode::OK);

    std::promise<read::DataResponse> promise;
    std::future<read::DataResponse> future = promise.get_future();

    auto token = client.GetData(
        read::DataRequest().WithPartitionId(kPartitionId),
        [&](read::DataResponse response) { promise.set_value(response); });

    EXPECT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    ASSERT_TRUE(response.IsSuccessful());

    Mock::VerifyAndClearExpectations(network_mock.get());
  }

  {
    SCOPED_TRACE("Get Data with PartitionId and DataHandle");
    std::promise<read::DataResponse> promise;
    std::future<read::DataResponse> future = promise.get_future();

    auto token = client.GetData(
        read::DataRequest()
            .WithPartitionId(kPartitionId)
            .WithDataHandle(kBlobDataHandle),
        [&](read::DataResponse response) { promise.set_value(response); });

    EXPECT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    ASSERT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::PreconditionFailed);
  }

  {
    SCOPED_TRACE("Get Data from non existent partition");

    SetupNetworkExpectation(*network_mock, kUrlLookup, kHttpResponseLookup,
                            olp::http::HttpStatusCode::OK);

    SetupNetworkExpectation(*network_mock, kUrlQueryPartition269,
                            kHttpResponseNoPartition,
                            olp::http::HttpStatusCode::OK);

    std::promise<read::DataResponse> promise;
    std::future<read::DataResponse> future = promise.get_future();

    auto token = client.GetData(
        read::DataRequest().WithPartitionId(kPartitionId),
        [&](read::DataResponse response) { promise.set_value(response); });

    EXPECT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    ASSERT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::NotFound);

    Mock::VerifyAndClearExpectations(network_mock.get());
  }
}

TEST(VolatileLayerClientImplTest, GetDataCancellableFuture) {
  std::shared_ptr<NetworkMock> network_mock = std::make_shared<NetworkMock>();
  std::shared_ptr<CacheMock> cache_mock = std::make_shared<CacheMock>();
  olp::client::OlpClientSettings settings;
  settings.network_request_handler = network_mock;
  settings.cache = cache_mock;

  read::VolatileLayerClientImpl client(kHrn, kLayerId, settings);

  {
    SCOPED_TRACE("Get Data with DataHandle");

    EXPECT_CALL(*network_mock, Send(IsGetRequest(kUrlLookup), _, _, _, _))
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kHttpResponseLookup));

    SetupNetworkExpectation(*network_mock, kUrlVolatileBlobData, "someData",
                            olp::http::HttpStatusCode::OK);

    auto future =
        client.GetData(read::DataRequest().WithDataHandle(kBlobDataHandle))
            .GetFuture();

    EXPECT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    ASSERT_TRUE(response.IsSuccessful());

    Mock::VerifyAndClearExpectations(network_mock.get());
  }

  {
    SCOPED_TRACE("Get Data with PartitionId");

    EXPECT_CALL(*network_mock, Send(IsGetRequest(kUrlLookup), _, _, _, _))
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kHttpResponseLookup));

    SetupNetworkExpectation(*network_mock, kUrlQueryPartition269,
                            kHttpResponsePartition269,
                            olp::http::HttpStatusCode::OK);

    SetupNetworkExpectation(*network_mock, kUrlVolatileBlobData, "someData",
                            olp::http::HttpStatusCode::OK);

    auto future =
        client.GetData(read::DataRequest().WithPartitionId(kPartitionId))
            .GetFuture();

    EXPECT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    ASSERT_TRUE(response.IsSuccessful());

    Mock::VerifyAndClearExpectations(network_mock.get());
  }

  {
    SCOPED_TRACE("Get Data from non existent partition");

    EXPECT_CALL(*network_mock, Send(IsGetRequest(kUrlLookup), _, _, _, _))
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kHttpResponseLookup));

    SetupNetworkExpectation(*network_mock, kUrlQueryPartition269,
                            kHttpResponseNoPartition,
                            olp::http::HttpStatusCode::OK);

    auto future =
        client.GetData(read::DataRequest().WithPartitionId(kPartitionId))
            .GetFuture();

    EXPECT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    ASSERT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::NotFound);

    Mock::VerifyAndClearExpectations(network_mock.get());
  }
}

TEST(VolatileLayerClientImplTest, GetDataCancelOnClientDestroy) {
  std::shared_ptr<NetworkMock> network_mock = std::make_shared<NetworkMock>();
  std::shared_ptr<CacheMock> cache_mock = std::make_shared<CacheMock>();
  olp::client::OlpClientSettings settings;
  settings.network_request_handler = network_mock;
  settings.cache = cache_mock;
  settings.task_scheduler =
      olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);

  {
    // Simulate a loaded queue
    settings.task_scheduler->ScheduleTask(
        []() { std::this_thread::sleep_for(std::chrono::seconds(1)); });

    read::DataResponse data_response;
    {
      // Client owns the task scheduler
      auto caller_thread_id = std::this_thread::get_id();
      read::VolatileLayerClientImpl client(kHrn, kLayerId, std::move(settings));
      client.GetData(read::DataRequest().WithPartitionId(kPartitionId),
                     [&](read::DataResponse response) {
                       data_response = std::move(response);
                       EXPECT_NE(caller_thread_id, std::this_thread::get_id());
                     });
    }

    // Callback must be called during client destructor.
    EXPECT_FALSE(data_response.IsSuccessful());
    EXPECT_EQ(data_response.GetError().GetErrorCode(),
              olp::client::ErrorCode::Cancelled);
  }
}

TEST(VolatileLayerClientImplTest, GetDataCancellableFutureCancel) {
  std::shared_ptr<NetworkMock> network_mock = std::make_shared<NetworkMock>();
  std::shared_ptr<CacheMock> cache_mock = std::make_shared<CacheMock>();
  olp::client::OlpClientSettings settings;
  settings.network_request_handler = network_mock;
  settings.cache = cache_mock;
  settings.task_scheduler =
      olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);
  read::VolatileLayerClientImpl client(kHrn, kLayerId, std::move(settings));

  auto cancellable =
      client.GetData(read::DataRequest().WithPartitionId(kPartitionId));

  auto data_future = cancellable.GetFuture();
  cancellable.GetCancellationToken().Cancel();
  ASSERT_EQ(data_future.wait_for(kTimeout), std::future_status::ready);

  auto data_response = data_future.get();

  // Callback must be called during client destructor.
  EXPECT_FALSE(data_response.IsSuccessful());
  EXPECT_EQ(data_response.GetError().GetErrorCode(),
            olp::client::ErrorCode::Cancelled);
}

TEST(VolatileLayerClientImplTest, RemoveFromCachePartition) {
  olp::client::OlpClientSettings settings;
  std::shared_ptr<CacheMock> cache_mock = std::make_shared<CacheMock>();
  settings.cache = cache_mock;

  // successfull mock cache calls
  auto found_cache_response = [](const std::string& /*key*/,
                                 const olp::cache::Decoder& /*encoder*/) {
    auto partition = model::Partition();
    partition.SetPartition(kPartitionId);
    partition.SetDataHandle(kBlobDataHandle);
    return partition;
  };

  auto partition_cache_remove = [&](const std::string& prefix) {
    std::string expected_prefix = kHrn.ToCatalogHRNString() + "::" + kLayerId +
                                  "::" + kPartitionId + "::partition";
    EXPECT_EQ(prefix, expected_prefix);
    return true;
  };

  auto data_cache_remove = [&](const std::string& prefix) {
    std::string expected_prefix = kHrn.ToCatalogHRNString() + "::" + kLayerId +
                                  "::" + kBlobDataHandle + "::Data";
    EXPECT_EQ(prefix, expected_prefix);
    return true;
  };

  read::VolatileLayerClientImpl client(kHrn, kLayerId, settings);
  {
    SCOPED_TRACE("Successfull remove partition from cache");
    EXPECT_CALL(*cache_mock, Get(_, _)).WillOnce(found_cache_response);
    EXPECT_CALL(*cache_mock, RemoveKeysWithPrefix(_))
        .WillOnce(partition_cache_remove)
        .WillOnce(data_cache_remove);
    ASSERT_TRUE(client.RemoveFromCache(kPartitionId));
  }
  {
    SCOPED_TRACE("Remove not existing partition from cache");
    EXPECT_CALL(*cache_mock, Get(_, _))
        .WillOnce([](const std::string&, const olp::cache::Decoder&) {
          return boost::any();
        });
    ASSERT_TRUE(client.RemoveFromCache(kPartitionId));
  }
  {
    SCOPED_TRACE("Partition cache failure");
    EXPECT_CALL(*cache_mock, Get(_, _)).WillOnce(found_cache_response);
    EXPECT_CALL(*cache_mock, RemoveKeysWithPrefix(_))
        .WillOnce([](const std::string&) { return false; });
    ASSERT_FALSE(client.RemoveFromCache(kPartitionId));
  }
  {
    SCOPED_TRACE("Data cache failure");
    EXPECT_CALL(*cache_mock, Get(_, _)).WillOnce(found_cache_response);
    EXPECT_CALL(*cache_mock, RemoveKeysWithPrefix(_))
        .WillOnce(partition_cache_remove)
        .WillOnce([](const std::string&) { return false; });
    ASSERT_FALSE(client.RemoveFromCache(kPartitionId));
  }
}

TEST(VolatileLayerClientImplTest, RemoveFromCacheTileKey) {
  olp::client::OlpClientSettings settings;
  std::shared_ptr<CacheMock> cache_mock = std::make_shared<CacheMock>();
  settings.cache = cache_mock;

  // successfull mock cache calls
  auto found_cache_response = [](const std::string& /*key*/,
                                 const olp::cache::Decoder& /*encoder*/) {
    auto partition = model::Partition();
    partition.SetPartition(kPartitionId);
    partition.SetDataHandle(kBlobDataHandle);
    return partition;
  };

  auto partition_cache_remove = [&](const std::string& prefix) {
    std::string expected_prefix = kHrn.ToCatalogHRNString() + "::" + kLayerId +
                                  "::" + kPartitionId + "::partition";
    EXPECT_EQ(prefix, expected_prefix);
    return true;
  };

  auto data_cache_remove = [&](const std::string& prefix) {
    std::string expected_prefix = kHrn.ToCatalogHRNString() + "::" + kLayerId +
                                  "::" + kBlobDataHandle + "::Data";
    EXPECT_EQ(prefix, expected_prefix);
    return true;
  };

  auto tile_key = olp::geo::TileKey::FromHereTile(kPartitionId);
  read::VolatileLayerClientImpl client(kHrn, kLayerId, settings);
  {
    SCOPED_TRACE("Successfull remove partition from cache");
    EXPECT_CALL(*cache_mock, Get(_, _)).WillOnce(found_cache_response);
    EXPECT_CALL(*cache_mock, RemoveKeysWithPrefix(_))
        .WillOnce(partition_cache_remove)
        .WillOnce(data_cache_remove);
    ASSERT_TRUE(client.RemoveFromCache(tile_key));
  }
  {
    SCOPED_TRACE("Remove not existing partition from cache");
    EXPECT_CALL(*cache_mock, Get(_, _))
        .WillOnce([](const std::string&, const olp::cache::Decoder&) {
          return boost::any();
        });
    ASSERT_TRUE(client.RemoveFromCache(tile_key));
  }
  {
    SCOPED_TRACE("Partition cache failure");
    EXPECT_CALL(*cache_mock, Get(_, _)).WillOnce(found_cache_response);
    EXPECT_CALL(*cache_mock, RemoveKeysWithPrefix(_))
        .WillOnce([](const std::string&) { return false; });
    ASSERT_FALSE(client.RemoveFromCache(tile_key));
  }
  {
    SCOPED_TRACE("Data cache failure");
    EXPECT_CALL(*cache_mock, Get(_, _)).WillOnce(found_cache_response);
    EXPECT_CALL(*cache_mock, RemoveKeysWithPrefix(_))
        .WillOnce(partition_cache_remove)
        .WillOnce([](const std::string&) { return false; });
    ASSERT_FALSE(client.RemoveFromCache(tile_key));
  }
}

TEST(VolatileLayerClientImplTest, PrefetchTiles) {
  std::shared_ptr<NetworkMock> network_mock = std::make_shared<NetworkMock>();
  std::shared_ptr<CacheMock> cache_mock = std::make_shared<CacheMock>();

  olp::client::OlpClientSettings settings;
  settings.network_request_handler = network_mock;
  settings.cache = cache_mock;
  olp::client::HRN catalog(kCatalog);
  read::VolatileLayerClientImpl client(catalog, kLayerId, settings);
  {
    SCOPED_TRACE("Prefetch tiles and store them in memory cache");

    SetupNetworkExpectation(*network_mock, kUrlQuadTreeIndexVolatile,
                            kHttpResponseQuadTreeIndexVolatile,
                            olp::http::HttpStatusCode::OK);
    SetupNetworkExpectation(*network_mock, kUrlPrefetchBlobData1, kData1,
                            olp::http::HttpStatusCode::OK);

    EXPECT_CALL(*network_mock, Send(IsGetRequest(kUrlLookup), _, _, _, _))
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kHttpResponseLookup));

    std::vector<olp::geo::TileKey> tile_keys = {
        olp::geo::TileKey::FromHereTile(kTileId)};

    auto request = read::PrefetchTilesRequest()
                       .WithTileKeys(tile_keys)
                       .WithMinLevel(8)
                       .WithMaxLevel(12);

    auto promise =
        std::make_shared<std::promise<read::PrefetchTilesResponse>>();
    std::future<read::PrefetchTilesResponse> future = promise->get_future();
    auto token = client.PrefetchTiles(
        request, [promise](read::PrefetchTilesResponse response) {
          promise->set_value(std::move(response));
        });

    ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
    read::PrefetchTilesResponse response = future.get();
    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    const auto& result = response.GetResult();
    ASSERT_FALSE(result.empty());
    for (auto tile_result : result) {
      std::string str = tile_result->tile_key_.ToHereTile();
      ASSERT_TRUE(tile_result->IsSuccessful());
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
    }
  }
  {
    SCOPED_TRACE("Prefetch tiles with default levels");

    SetupNetworkExpectation(*network_mock, kUrlQuadTreeIndexVolatile2,
                            kHttpResponseQuadTreeIndexVolatile,
                            olp::http::HttpStatusCode::OK);

    EXPECT_CALL(*network_mock, Send(IsGetRequest(kUrlLookup), _, _, _, _))
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kHttpResponseLookup));

    std::vector<olp::geo::TileKey> tile_keys = {
        olp::geo::TileKey::FromHereTile(kTileId)};

    auto request = read::PrefetchTilesRequest().WithTileKeys(tile_keys);

    auto promise =
        std::make_shared<std::promise<read::PrefetchTilesResponse>>();
    std::future<read::PrefetchTilesResponse> future = promise->get_future();
    auto token = client.PrefetchTiles(
        request, [promise](read::PrefetchTilesResponse response) {
          promise->set_value(std::move(response));
        });

    ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
    read::PrefetchTilesResponse response = future.get();
    ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  }
  {
    SCOPED_TRACE("Levels not specified.");

    SetupNetworkExpectation(*network_mock, kUrlQuadTreeIndexVolatile2,
                            kHttpResponseQuadTreeIndexVolatile,
                            olp::http::HttpStatusCode::OK);

    EXPECT_CALL(*network_mock, Send(IsGetRequest(kUrlLookup), _, _, _, _))
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kHttpResponseLookup));

    std::vector<olp::geo::TileKey> tile_keys = {
        olp::geo::TileKey::FromHereTile(kTileId)};

    auto request = read::PrefetchTilesRequest().WithTileKeys(tile_keys);

    auto promise =
        std::make_shared<std::promise<read::PrefetchTilesResponse>>();
    std::future<read::PrefetchTilesResponse> future = promise->get_future();
    auto token = client.PrefetchTiles(
        request, [promise](read::PrefetchTilesResponse response) {
          promise->set_value(std::move(response));
        });

    ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
    read::PrefetchTilesResponse response = future.get();
    ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  }
  // negative tests
  {
    SCOPED_TRACE("No tiles in the request");

    auto request =
        read::PrefetchTilesRequest().WithMinLevel(11).WithMaxLevel(12);

    auto promise =
        std::make_shared<std::promise<read::PrefetchTilesResponse>>();
    std::future<read::PrefetchTilesResponse> future = promise->get_future();
    auto token = client.PrefetchTiles(
        request, [promise](read::PrefetchTilesResponse response) {
          promise->set_value(std::move(response));
        });

    ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
    read::PrefetchTilesResponse response = future.get();
    ASSERT_FALSE(response.IsSuccessful());
    const auto& error = response.GetError();

    ASSERT_EQ(olp::client::ErrorCode::InvalidArgument, error.GetErrorCode());
  }
  {
    SCOPED_TRACE("Max level < min level.");

    auto request =
        read::PrefetchTilesRequest().WithMinLevel(12).WithMaxLevel(11);

    auto promise =
        std::make_shared<std::promise<read::PrefetchTilesResponse>>();
    std::future<read::PrefetchTilesResponse> future = promise->get_future();
    auto token = client.PrefetchTiles(
        request, [promise](read::PrefetchTilesResponse response) {
          promise->set_value(std::move(response));
        });

    ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
    read::PrefetchTilesResponse response = future.get();
    ASSERT_FALSE(response.IsSuccessful());
    const auto& error = response.GetError();

    ASSERT_EQ(olp::client::ErrorCode::InvalidArgument, error.GetErrorCode());
  }
  {
    SCOPED_TRACE("Invalid levels.");

    auto request =
        read::PrefetchTilesRequest().WithMinLevel(-1).WithMaxLevel(-1);

    auto promise =
        std::make_shared<std::promise<read::PrefetchTilesResponse>>();
    std::future<read::PrefetchTilesResponse> future = promise->get_future();
    auto token = client.PrefetchTiles(
        request, [promise](read::PrefetchTilesResponse response) {
          promise->set_value(std::move(response));
        });

    ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
    read::PrefetchTilesResponse response = future.get();
    ASSERT_FALSE(response.IsSuccessful());
    const auto& error = response.GetError();

    ASSERT_EQ(olp::client::ErrorCode::InvalidArgument, error.GetErrorCode());
  }
}

TEST(VolatileLayerClientImplTest, PrefetchTilesCancellableFuture) {
  std::shared_ptr<NetworkMock> network_mock = std::make_shared<NetworkMock>();
  std::shared_ptr<CacheMock> cache_mock = std::make_shared<CacheMock>();

  olp::client::OlpClientSettings settings;
  settings.network_request_handler = network_mock;
  settings.cache = cache_mock;
  olp::client::HRN catalog(kCatalog);
  read::VolatileLayerClientImpl client(catalog, kLayerId, settings);
  {
    SCOPED_TRACE("Prefetch tiles and store them in memory cache");

    SetupNetworkExpectation(*network_mock, kUrlQuadTreeIndexVolatile,
                            kHttpResponseQuadTreeIndexVolatile,
                            olp::http::HttpStatusCode::OK);
    SetupNetworkExpectation(*network_mock, kUrlPrefetchBlobData1, kData1,
                            olp::http::HttpStatusCode::OK);

    EXPECT_CALL(*network_mock, Send(IsGetRequest(kUrlLookup), _, _, _, _))
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kHttpResponseLookup));

    std::vector<olp::geo::TileKey> tile_keys = {
        olp::geo::TileKey::FromHereTile(kTileId)};

    auto request = read::PrefetchTilesRequest()
                       .WithTileKeys(tile_keys)
                       .WithMinLevel(8)
                       .WithMaxLevel(12);

    auto cancellable = client.PrefetchTiles(request);
    auto future = cancellable.GetFuture();
    ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
    read::PrefetchTilesResponse response = future.get();
    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    const auto& result = response.GetResult();
    ASSERT_FALSE(result.empty());
    for (auto tile_result : result) {
      std::string str = tile_result->tile_key_.ToHereTile();
      ASSERT_TRUE(tile_result->IsSuccessful());
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
    }
  }
  {
    SCOPED_TRACE("Prefetch tiles with default levels");
    SetupNetworkExpectation(*network_mock, kUrlQuadTreeIndexVolatile2,
                            kHttpResponseQuadTreeIndexVolatile,
                            olp::http::HttpStatusCode::OK);

    EXPECT_CALL(*network_mock, Send(IsGetRequest(kUrlLookup), _, _, _, _))
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kHttpResponseLookup));

    std::vector<olp::geo::TileKey> tile_keys = {
        olp::geo::TileKey::FromHereTile(kTileId)};

    auto request = read::PrefetchTilesRequest().WithTileKeys(tile_keys);

    auto cancellable = client.PrefetchTiles(request);
    auto future = cancellable.GetFuture();

    ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
    read::PrefetchTilesResponse response = future.get();
    ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  }
  {
    SCOPED_TRACE("Levels not specified.");
    SetupNetworkExpectation(*network_mock, kUrlQuadTreeIndexVolatile2,
                            kHttpResponseQuadTreeIndexVolatile,
                            olp::http::HttpStatusCode::OK);

    EXPECT_CALL(*network_mock, Send(IsGetRequest(kUrlLookup), _, _, _, _))
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kHttpResponseLookup));

    std::vector<olp::geo::TileKey> tile_keys = {
        olp::geo::TileKey::FromHereTile(kTileId)};

    auto request = read::PrefetchTilesRequest().WithTileKeys(tile_keys);

    auto cancellable = client.PrefetchTiles(request);
    auto future = cancellable.GetFuture();

    ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
    read::PrefetchTilesResponse response = future.get();
    ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  }
  // negative tests
  {
    SCOPED_TRACE("No tiles in the request");

    auto request =
        read::PrefetchTilesRequest().WithMinLevel(11).WithMaxLevel(12);

    auto cancellable = client.PrefetchTiles(request);
    auto future = cancellable.GetFuture();

    ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
    read::PrefetchTilesResponse response = future.get();
    ASSERT_FALSE(response.IsSuccessful());
    const auto& error = response.GetError();

    ASSERT_EQ(olp::client::ErrorCode::InvalidArgument, error.GetErrorCode());
  }
  {
    SCOPED_TRACE("Max level < min level.");

    auto request =
        read::PrefetchTilesRequest().WithMinLevel(12).WithMaxLevel(11);

    auto cancellable = client.PrefetchTiles(request);
    auto future = cancellable.GetFuture();

    ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
    read::PrefetchTilesResponse response = future.get();
    ASSERT_FALSE(response.IsSuccessful());
    const auto& error = response.GetError();

    ASSERT_EQ(olp::client::ErrorCode::InvalidArgument, error.GetErrorCode());
  }
  {
    SCOPED_TRACE("Invalid levels.");

    auto request =
        read::PrefetchTilesRequest().WithMinLevel(-1).WithMaxLevel(-1);

    auto cancellable = client.PrefetchTiles(request);
    auto future = cancellable.GetFuture();

    ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
    read::PrefetchTilesResponse response = future.get();
    ASSERT_FALSE(response.IsSuccessful());
    const auto& error = response.GetError();

    ASSERT_EQ(olp::client::ErrorCode::InvalidArgument, error.GetErrorCode());
  }
}

TEST(VolatileLayerClientImplTest, PrefetchTilesCancelOnClientDestroy) {
  std::shared_ptr<NetworkMock> network_mock = std::make_shared<NetworkMock>();
  std::shared_ptr<CacheMock> cache_mock = std::make_shared<CacheMock>();
  olp::client::OlpClientSettings settings;
  settings.network_request_handler = network_mock;
  settings.cache = cache_mock;
  settings.task_scheduler =
      olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);

  {
    // Simulate a loaded queue
    settings.task_scheduler->ScheduleTask(
        []() { std::this_thread::sleep_for(std::chrono::seconds(1)); });

    read::PrefetchTilesResponse response;
    {
      // Client owns the task scheduler
      auto caller_thread_id = std::this_thread::get_id();
      read::VolatileLayerClientImpl client(kHrn, kLayerId, std::move(settings));
      std::vector<olp::geo::TileKey> tile_keys = {
          olp::geo::TileKey::FromHereTile(kTileId)};
      auto request = read::PrefetchTilesRequest()
                         .WithTileKeys(tile_keys)
                         .WithMinLevel(11)
                         .WithMaxLevel(12);

      client.PrefetchTiles(
          request, [&](read::PrefetchTilesResponse prefetch_response) {
            response = std::move(prefetch_response);
            EXPECT_NE(caller_thread_id, std::this_thread::get_id());
          });
    }

    // Callback must be called during client destructor.
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::Cancelled);
  }
}

TEST(VolatileLayerClientImplTest, PrefetchTilesCancellableFutureCancel) {
  std::shared_ptr<NetworkMock> network_mock = std::make_shared<NetworkMock>();
  std::shared_ptr<CacheMock> cache_mock = std::make_shared<CacheMock>();
  olp::client::OlpClientSettings settings;
  settings.network_request_handler = network_mock;
  settings.cache = cache_mock;
  settings.task_scheduler =
      olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);

  std::promise<void> block_promise;
  auto future = block_promise.get_future();
  settings.task_scheduler->ScheduleTask([&future]() { future.get(); });

  std::vector<olp::geo::TileKey> tile_keys = {
      olp::geo::TileKey::FromHereTile(kTileId)};
  read::VolatileLayerClientImpl client(kHrn, kLayerId, std::move(settings));
  auto cancellable = client.PrefetchTiles(
      read::PrefetchTilesRequest().WithTileKeys(tile_keys));

  // cancel the request and unblock queue
  cancellable.GetCancellationToken().Cancel();
  block_promise.set_value();
  auto data_future = cancellable.GetFuture();

  ASSERT_EQ(data_future.wait_for(kTimeout), std::future_status::ready);

  auto data_response = data_future.get();

  EXPECT_FALSE(data_response.IsSuccessful());
  EXPECT_EQ(data_response.GetError().GetErrorCode(),
            olp::client::ErrorCode::Cancelled);
}
}  // namespace
