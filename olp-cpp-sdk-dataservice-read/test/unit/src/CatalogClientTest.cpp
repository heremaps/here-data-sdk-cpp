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
#include <regex>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <olp/authentication/AuthenticationCredentials.h>
#include <olp/authentication/Settings.h>
#include <olp/authentication/TokenEndpoint.h>
#include <olp/authentication/TokenProvider.h>
#include <olp/authentication/TokenResult.h>
#include <olp/core/cache/DefaultCache.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientFactory.h>
#include <olp/core/logging/Log.h>
#include <olp/core/network/HttpResponse.h>
#include <olp/core/network/Network.h>
#include <olp/core/network/NetworkConfig.h>
#include <olp/core/network/NetworkRequest.h>
#include <olp/core/porting/make_unique.h>
#include <olp/core/utils/Dir.h>
#include <olp/dataservice/read/PrefetchTilesRequest.h>
#include "HttpResponses.h"
#include "olp/dataservice/read/CatalogClient.h"
#include "olp/dataservice/read/CatalogRequest.h"
#include "olp/dataservice/read/CatalogVersionRequest.h"
#include "olp/dataservice/read/DataRequest.h"
#include "olp/dataservice/read/PartitionsRequest.h"
#include "olp/dataservice/read/model/Catalog.h"
#include "testutils/CustomParameters.hpp"

using namespace olp::dataservice::read;
using namespace testing;

#ifdef _WIN32
const std::string k_client_test_dir("\\catalog_client_test");
const std::string k_client_test_cache_dir("\\catalog_client_test\\cache");
#else
const std::string k_client_test_dir("/catalog_client_test");
const std::string k_client_test_cache_dir("/catalog_client_test/cache");
#endif

class MockHandler {
 public:
  MOCK_METHOD3(op, olp::client::CancellationToken(
                       const olp::network::NetworkRequest& request,
                       const olp::network::NetworkConfig& config,
                       const olp::client::NetworkAsyncCallback& callback));
  olp::client::CancellationToken operator()(
      const olp::network::NetworkRequest& request,
      const olp::network::NetworkConfig& config,
      const olp::client::NetworkAsyncCallback& callback) {
    return op(request, config, callback);
  }
};

enum CacheType { InMemory = 0, Disk, Both };
using ClientTestParameter = std::pair<bool, CacheType>;

class CatalogClientTestBase
    : public ::testing::TestWithParam<ClientTestParameter> {
 protected:
  virtual bool isOnlineTest() { return std::get<0>(GetParam()); }

  std::string GetTestCatalog() {
    static std::string mockCatalog{"hrn:here:data:::hereos-internal-test-v2"};
    return isOnlineTest() ? CustomParameters::getArgument("catalog")
                          : mockCatalog;
  }

  std::string PrintError(const olp::client::ApiError& error) {
    std::ostringstream resultStream;
    resultStream << "ERROR: code: " << static_cast<int>(error.GetErrorCode())
                 << ", status: " << error.GetHttpStatusCode()
                 << ", message: " << error.GetMessage();
    return resultStream.str();
  }

  template <typename T>
  T GetExecutionTime(std::function<T()> func) {
    auto startTime = std::chrono::high_resolution_clock::now();
    auto result = func();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time = end - startTime;
    std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;

    return result;
  }

 protected:
  std::shared_ptr<olp::client::OlpClientSettings> settings_;
  std::shared_ptr<olp::client::OlpClient> client_;
  std::shared_ptr<MockHandler> handler_;
};

class CatalogClientOnlineTest : public CatalogClientTestBase {
  void SetUp() {
    //    olp::logging::Log::setLevel(olp::logging::Level::Trace);
    handler_ = std::make_shared<MockHandler>();

    olp::authentication::TokenProviderDefault provider(
        CustomParameters::getArgument("appid"),
        CustomParameters::getArgument("secret"));
    olp::client::AuthenticationSettings authSettings;
    authSettings.provider = provider;
    settings_ = std::make_shared<olp::client::OlpClientSettings>();
    settings_->authentication_settings = authSettings;
    client_ = olp::client::OlpClientFactory::Create(*settings_);
  }
};

INSTANTIATE_TEST_SUITE_P(TestOnline, CatalogClientOnlineTest,
                         ::testing::Values(std::make_pair(true, Both)));

TEST_P(CatalogClientOnlineTest, GetCatalog) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::CatalogRequest();

  auto catalogResponse =
      GetExecutionTime<olp::dataservice::read::CatalogResponse>([&] {
        auto future = catalogClient->GetCatalog(request);
        return future.GetFuture().get();
      });

  ASSERT_TRUE(catalogResponse.IsSuccessful())
      << PrintError(catalogResponse.GetError());
}

TEST_P(CatalogClientOnlineTest, GetPartitionsWithInvalidHrn) {
  olp::client::HRN hrn("hrn:here:data:::nope-test-v2");

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer");
  auto partitionsResponse =
      GetExecutionTime<olp::dataservice::read::PartitionsResponse>([&] {
        auto future = catalogClient->GetPartitions(request);
        return future.GetFuture().get();
      });

  ASSERT_FALSE(partitionsResponse.IsSuccessful());
  ASSERT_EQ(403, partitionsResponse.GetError().GetHttpStatusCode());
}

TEST_P(CatalogClientOnlineTest, GetPartitions) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer");
  auto partitionsResponse =
      GetExecutionTime<olp::dataservice::read::PartitionsResponse>([&] {
        auto future = catalogClient->GetPartitions(request);
        return future.GetFuture().get();
      });

  ASSERT_TRUE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(4u, partitionsResponse.GetResult().GetPartitions().size());
}

TEST_P(CatalogClientOnlineTest, GetPartitionsForInvalidLayer) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("invalidLayer");
  auto partitionsResponse =
      GetExecutionTime<olp::dataservice::read::PartitionsResponse>([&] {
        auto future = catalogClient->GetPartitions(request);
        return future.GetFuture().get();
      });

  ASSERT_FALSE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(olp::client::ErrorCode::InvalidArgument,
            partitionsResponse.GetError().GetErrorCode());
}

TEST_P(CatalogClientOnlineTest, GetDataWithInvalidHrn) {
  olp::client::HRN hrn("hrn:here:data:::nope-test-v2");

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer")
      .WithDataHandle("d5d73b64-7365-41c3-8faf-aa6ad5bab135");
  auto dataResponse =
      GetExecutionTime<olp::dataservice::read::DataResponse>([&] {
        auto future = catalogClient->GetData(request);
        return future.GetFuture().get();
      });

  ASSERT_FALSE(dataResponse.IsSuccessful());
  ASSERT_EQ(403, dataResponse.GetError().GetHttpStatusCode());
}

TEST_P(CatalogClientOnlineTest, GetDataWithHandle) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer")
      .WithDataHandle("d5d73b64-7365-41c3-8faf-aa6ad5bab135");
  auto dataResponse =
      GetExecutionTime<olp::dataservice::read::DataResponse>([&] {
        auto future = catalogClient->GetData(request);
        return future.GetFuture().get();
      });

  ASSERT_TRUE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_LT(0, dataResponse.GetResult()->size());
  std::string dataStr(dataResponse.GetResult()->begin(),
                      dataResponse.GetResult()->end());
  ASSERT_EQ("DT_2_0031", dataStr);
}

TEST_P(CatalogClientOnlineTest, GetDataWithInvalidDataHandle) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithDataHandle("invalidDataHandle");
  auto dataResponse =
      GetExecutionTime<olp::dataservice::read::DataResponse>([&] {
        auto future = catalogClient->GetData(request);
        return future.GetFuture().get();
      });

  ASSERT_FALSE(dataResponse.IsSuccessful());
  ASSERT_EQ(404, dataResponse.GetError().GetHttpStatusCode());
}

TEST_P(CatalogClientOnlineTest, GetDataHandleWithInvalidLayer) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("invalidLayer").WithDataHandle("invalidDataHandle");
  auto dataResponse =
      GetExecutionTime<olp::dataservice::read::DataResponse>([&] {
        auto future = catalogClient->GetData(request);
        return future.GetFuture().get();
      });

  ASSERT_FALSE(dataResponse.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::InvalidArgument,
            dataResponse.GetError().GetErrorCode());
}

TEST_P(CatalogClientOnlineTest, GetDataWithPartitionId) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");
  auto dataResponse =
      GetExecutionTime<olp::dataservice::read::DataResponse>([&] {
        auto future = catalogClient->GetData(request);
        return future.GetFuture().get();
      });

  ASSERT_TRUE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_LT(0, dataResponse.GetResult()->size());
  std::string dataStr(dataResponse.GetResult()->begin(),
                      dataResponse.GetResult()->end());
  ASSERT_EQ("DT_2_0031", dataStr);
}

TEST_P(CatalogClientOnlineTest, GetDataWithPartitionIdVersion2) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269").WithVersion(2);
  auto dataResponse =
      GetExecutionTime<olp::dataservice::read::DataResponse>([&] {
        auto future = catalogClient->GetData(request);
        return future.GetFuture().get();
      });

  ASSERT_TRUE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_LT(0, dataResponse.GetResult()->size());
  std::string dataStr(dataResponse.GetResult()->begin(),
                      dataResponse.GetResult()->end());
  ASSERT_EQ("DT_2_0031", dataStr);
}

TEST_P(CatalogClientOnlineTest, GetDataWithPartitionIdInvalidVersion) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269").WithVersion(10);
  auto dataResponse =
      GetExecutionTime<olp::dataservice::read::DataResponse>([&] {
        auto future = catalogClient->GetData(request);
        return future.GetFuture().get();
      });

  ASSERT_FALSE(dataResponse.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::BadRequest,
            dataResponse.GetError().GetErrorCode());
  ASSERT_EQ(400, dataResponse.GetError().GetHttpStatusCode());

  request.WithVersion(-1);
  dataResponse = GetExecutionTime<olp::dataservice::read::DataResponse>([&] {
    auto future = catalogClient->GetData(request);
    return future.GetFuture().get();
  });

  ASSERT_FALSE(dataResponse.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::BadRequest,
            dataResponse.GetError().GetErrorCode());
  ASSERT_EQ(400, dataResponse.GetError().GetHttpStatusCode());
}

TEST_P(CatalogClientOnlineTest, GetPartitionsVersion2) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer").WithVersion(2);
  auto partitionsResponse =
      GetExecutionTime<olp::dataservice::read::PartitionsResponse>([&] {
        auto future = catalogClient->GetPartitions(request);
        return future.GetFuture().get();
      });

  ASSERT_TRUE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_LT(0, partitionsResponse.GetResult().GetPartitions().size());
}

TEST_P(CatalogClientOnlineTest, GetPartitionsInvalidVersion) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer").WithVersion(10);
  auto partitionsResponse =
      GetExecutionTime<olp::dataservice::read::PartitionsResponse>([&] {
        auto future = catalogClient->GetPartitions(request);
        return future.GetFuture().get();
      });

  ASSERT_FALSE(partitionsResponse.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::BadRequest,
            partitionsResponse.GetError().GetErrorCode());
  ASSERT_EQ(400, partitionsResponse.GetError().GetHttpStatusCode());

  request.WithVersion(-1);
  partitionsResponse =
      GetExecutionTime<olp::dataservice::read::PartitionsResponse>([&] {
        auto future = catalogClient->GetPartitions(request);
        return future.GetFuture().get();
      });

  ASSERT_FALSE(partitionsResponse.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::BadRequest,
            partitionsResponse.GetError().GetErrorCode());
  ASSERT_EQ(400, partitionsResponse.GetError().GetHttpStatusCode());
}

TEST_P(CatalogClientOnlineTest, GetDataWithNonExistentPartitionId) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("noPartition");
  auto dataResponse =
      GetExecutionTime<olp::dataservice::read::DataResponse>([&] {
        auto future = catalogClient->GetData(request);
        return future.GetFuture().get();
      });

  ASSERT_TRUE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_FALSE(dataResponse.GetResult());
}

TEST_P(CatalogClientOnlineTest, GetDataWithInvalidLayerId) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("invalidLayer").WithPartitionId("269");
  auto dataResponse =
      GetExecutionTime<olp::dataservice::read::DataResponse>([&] {
        auto future = catalogClient->GetData(request);
        return future.GetFuture().get();
      });

  ASSERT_FALSE(dataResponse.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::InvalidArgument,
            dataResponse.GetError().GetErrorCode());
}

TEST_P(CatalogClientOnlineTest, GetDataWithInlineField) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("3");
  auto dataResponse =
      GetExecutionTime<olp::dataservice::read::DataResponse>([&] {
        auto future = catalogClient->GetData(request);
        return future.GetFuture().get();
      });

  ASSERT_TRUE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_LT(0, dataResponse.GetResult()->size());
  std::string dataStr(dataResponse.GetResult()->begin(),
                      dataResponse.GetResult()->end());
  ASSERT_EQ(0u, dataStr.find("data:"));
}

TEST_P(CatalogClientOnlineTest, GetDataWithEmptyField) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("1");
  auto dataResponse =
      GetExecutionTime<olp::dataservice::read::DataResponse>([&] {
        auto future = catalogClient->GetData(request);
        return future.GetFuture().get();
      });

  ASSERT_TRUE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_FALSE(dataResponse.GetResult());
}

TEST_P(CatalogClientOnlineTest, GetDataCompressed) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("here_van_wc2018_pool");
  auto dataResponse =
      GetExecutionTime<olp::dataservice::read::DataResponse>([&] {
        auto future = catalogClient->GetData(request);
        return future.GetFuture().get();
      });

  ASSERT_TRUE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_LT(0u, dataResponse.GetResult()->size());

  auto requestCompressed = olp::dataservice::read::DataRequest();
  requestCompressed.WithLayerId("testlayer_gzip")
      .WithPartitionId("here_van_wc2018_pool");
  auto dataResponseCompressed =
      GetExecutionTime<olp::dataservice::read::DataResponse>([&] {
        auto future = catalogClient->GetData(requestCompressed);
        return future.GetFuture().get();
      });

  ASSERT_TRUE(dataResponseCompressed.IsSuccessful())
      << PrintError(dataResponseCompressed.GetError());
  ASSERT_LT(0u, dataResponseCompressed.GetResult()->size());
  ASSERT_EQ(dataResponse.GetResult()->size(),
            dataResponseCompressed.GetResult()->size());
}

void dumpTileKey(const olp::geo::TileKey& tileKey) {
  std::cout << "Tile: " << tileKey.ToHereTile()
            << ", level: " << tileKey.Level()
            << ", parent: " << tileKey.Parent().ToHereTile() << std::endl;
}

TEST_P(CatalogClientOnlineTest, Prefetch) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  std::vector<olp::geo::TileKey> tileKeys;
  tileKeys.emplace_back(olp::geo::TileKey::FromHereTile("5904591"));

  auto request = olp::dataservice::read::PrefetchTilesRequest()
                     .WithLayerId("hype-test-prefetch")
                     .WithTileKeys(tileKeys)
                     .WithMinLevel(10)
                     .WithMaxLevel(12);

  auto future = catalogClient->PrefetchTiles(request);

  auto response = future.GetFuture().get();
  ASSERT_TRUE(response.IsSuccessful());

  auto& result = response.GetResult();

  for (auto tileResult : result) {
    ASSERT_TRUE(tileResult->IsSuccessful());
    ASSERT_TRUE(tileResult->tile_key_.IsValid());

    dumpTileKey(tileResult->tile_key_);
  }
  ASSERT_EQ(6u, result.size());

  // Second part, use the cache, fetch a partition that's the child of 5904591
  {
    auto request = olp::dataservice::read::DataRequest();
    request.WithLayerId("hype-test-prefetch")
        .WithPartitionId("23618365")
        .WithFetchOption(FetchOptions::CacheOnly);
    auto future = catalogClient->GetData(request);

    auto dataResponse = future.GetFuture().get();

    ASSERT_TRUE(dataResponse.IsSuccessful())
        << PrintError(dataResponse.GetError());
    ASSERT_LT(0, dataResponse.GetResult()->size());
  }
  // The parent of 5904591 should be fetched too
  {
    auto request = olp::dataservice::read::DataRequest();
    request.WithLayerId("hype-test-prefetch")
        .WithPartitionId("1476147")
        .WithFetchOption(FetchOptions::CacheOnly);
    auto future = catalogClient->GetData(request);

    auto dataResponse = future.GetFuture().get();

    ASSERT_TRUE(dataResponse.IsSuccessful())
        << PrintError(dataResponse.GetError());
    ASSERT_LT(0, dataResponse.GetResult()->size());
  }
}

//** ** ** ** MOCK TESTS ** ** ** ** **//

MATCHER_P(IsGetRequest, url, "") {
  // uri, verb, null body
  return olp::network::NetworkRequest::HttpVerb::GET == arg.Verb() &&
         url == arg.Url() && (!arg.Content() || arg.Content()->empty());
}

olp::client::NetworkAsyncHandler setsPromiseWaitsAndReturns(
    std::shared_ptr<std::promise<void>> preSignal,
    std::shared_ptr<std::promise<void>> waitForSignal,
    olp::network::HttpResponse response,
    std::shared_ptr<std::promise<void>> postSignal =
        std::make_shared<std::promise<void>>()) {
  return [preSignal, waitForSignal, response, postSignal](
             const olp::network::NetworkRequest& request,
             const olp::network::NetworkConfig& /*config*/,
             const olp::client::NetworkAsyncCallback& callback)
             -> olp::client::CancellationToken {
    auto completed = std::make_shared<std::atomic_bool>(false);

    std::thread([request, preSignal, waitForSignal, completed, callback,
                 response, postSignal]() {
      // emulate a small response delay
      std::this_thread::sleep_for(std::chrono::milliseconds(50));

      preSignal->set_value();
      waitForSignal->get_future().get();

      if (!completed->exchange(true)) {
        callback(response);
      }

      postSignal->set_value();
    })
        .detach();

    return olp::client::CancellationToken([request, completed, callback,
                                           postSignal]() {
      if (!completed->exchange(true)) {
        callback({olp::network::Network::ErrorCode::Cancelled, "Cancelled"});
      }
    });
  };
}

olp::client::NetworkAsyncHandler returnsResponse(
    olp::network::HttpResponse response) {
  return [=](const olp::network::NetworkRequest& /*request*/,
             const olp::network::NetworkConfig& /*config*/,
             const olp::client::NetworkAsyncCallback& callback)
             -> olp::client::CancellationToken {
    std::thread([callback, response]() { callback(response); }).detach();
    return olp::client::CancellationToken();
  };
}

class CatalogClientMockTest : public CatalogClientTestBase {
 protected:
  void SetUp() {
    handler_ = std::make_shared<MockHandler>();

    auto weakHandler = std::weak_ptr<MockHandler>(handler_);
    auto handle = [weakHandler](
                      const olp::network::NetworkRequest& request,
                      const olp::network::NetworkConfig& config,
                      const olp::client::NetworkAsyncCallback& callback)
        -> olp::client::CancellationToken {
      auto sharedHandler = weakHandler.lock();
      if (sharedHandler) {
        return (*sharedHandler)(request, config, callback);
      }
      return olp::client::CancellationToken();
    };
    settings_ = std::make_shared<olp::client::OlpClientSettings>();
    settings_->network_async_handler = handle;
    client_ = olp::client::OlpClientFactory::Create(*settings_);

    SetUpCommonNetworkMockCalls();
  }

  void SetUpCommonNetworkMockCalls() {
    ON_CALL(*handler_,
            op(IsGetRequest(URL_LOOKUP_CONFIG), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_LOOKUP_CONFIG})));

    ON_CALL(*handler_, op(IsGetRequest(URL_CONFIG), testing::_, testing::_))
        .WillByDefault(
            testing::Invoke(returnsResponse({200, HTTP_RESPONSE_CONFIG})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_LOOKUP_METADATA), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_LOOKUP_METADATA})));

    ON_CALL(*handler_, op(IsGetRequest(URL_LATEST_CATALOG_VERSION), testing::_,
                          testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_LATEST_CATALOG_VERSION})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_LAYER_VERSIONS), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_LAYER_VERSIONS})));

    ON_CALL(*handler_, op(IsGetRequest(URL_PARTITIONS), testing::_, testing::_))
        .WillByDefault(
            testing::Invoke(returnsResponse({200, HTTP_RESPONSE_PARTITIONS})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_LOOKUP_QUERY), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_LOOKUP_QUERY})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_QUERY_PARTITION_269), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_PARTITION_269})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_LOOKUP_BLOB), testing::_, testing::_))
        .WillByDefault(
            testing::Invoke(returnsResponse({200, HTTP_RESPONSE_LOOKUP_BLOB})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_BLOB_DATA_269), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_BLOB_DATA_269})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_PARTITION_3), testing::_, testing::_))
        .WillByDefault(
            testing::Invoke(returnsResponse({200, HTTP_RESPONSE_PARTITION_3})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_LOOKUP_VOLATILE_BLOB), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_LOOKUP_VOLATILE_BLOB})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_LAYER_VERSIONS_V2), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_LAYER_VERSIONS_V2})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_PARTITIONS_V2), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_PARTITIONS_V2})));

    ON_CALL(*handler_, op(IsGetRequest(URL_QUERY_PARTITION_269_V2), testing::_,
                          testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_PARTITION_269_V2})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_BLOB_DATA_269_V2), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_BLOB_DATA_269_V2})));

    ON_CALL(*handler_, op(IsGetRequest(URL_QUERY_PARTITION_269_V10), testing::_,
                          testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({400, HTTP_RESPONSE_INVALID_VERSION_V10})));

    ON_CALL(*handler_, op(IsGetRequest(URL_QUERY_PARTITION_269_VN1), testing::_,
                          testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({400, HTTP_RESPONSE_INVALID_VERSION_VN1})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_LAYER_VERSIONS_V10), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({400, HTTP_RESPONSE_INVALID_VERSION_V10})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_LAYER_VERSIONS_VN1), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({400, HTTP_RESPONSE_INVALID_VERSION_VN1})));

    ON_CALL(*handler_, op(IsGetRequest(URL_CONFIG_V2), testing::_, testing::_))
        .WillByDefault(
            testing::Invoke(returnsResponse({200, HTTP_RESPONSE_CONFIG_V2})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_QUADKEYS_23618364), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_QUADKEYS_23618364})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_QUADKEYS_1476147), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_QUADKEYS_1476147})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_QUADKEYS_5904591), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_QUADKEYS_5904591})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_QUADKEYS_369036), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_QUADKEYS_369036})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_BLOB_DATA_PREFETCH_1), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_BLOB_DATA_PREFETCH_1})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_BLOB_DATA_PREFETCH_2), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_BLOB_DATA_PREFETCH_2})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_BLOB_DATA_PREFETCH_3), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_BLOB_DATA_PREFETCH_3})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_BLOB_DATA_PREFETCH_4), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_BLOB_DATA_PREFETCH_4})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_BLOB_DATA_PREFETCH_5), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_BLOB_DATA_PREFETCH_5})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_BLOB_DATA_PREFETCH_6), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_BLOB_DATA_PREFETCH_6})));

    ON_CALL(*handler_,
            op(IsGetRequest(URL_BLOB_DATA_PREFETCH_7), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_BLOB_DATA_PREFETCH_7})));

    // Catch any non-interesting network calls that don't need to be verified
    EXPECT_CALL(*handler_, op(testing::_, testing::_, testing::_))
        .Times(testing::AtLeast(0));
  }
};

INSTANTIATE_TEST_SUITE_P(TestMock, CatalogClientMockTest,
                         ::testing::Values(std::make_pair(false, Both)));

TEST_P(CatalogClientMockTest, GetCatalog) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_CONFIG), testing::_, testing::_))
      .Times(1);

  auto catalogClient = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = CatalogRequest();
  auto future = catalogClient->GetCatalog(request);
  CatalogResponse catalogResponse = future.GetFuture().get();

  ASSERT_TRUE(catalogResponse.IsSuccessful())
      << PrintError(catalogResponse.GetError());
}

TEST_P(CatalogClientMockTest, GetCatalogCallback) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_CONFIG), testing::_, testing::_))
      .Times(1);

  auto catalogClient = std::make_unique<CatalogClient>(hrn, settings_);

  auto request = CatalogRequest();

  std::promise<CatalogResponse> promise;
  CatalogResponseCallback callback = [&promise](CatalogResponse response) {
    promise.set_value(response);
  };
  catalogClient->GetCatalog(request, callback);
  CatalogResponse catalogResponse = promise.get_future().get();
  ASSERT_TRUE(catalogResponse.IsSuccessful())
      << PrintError(catalogResponse.GetError());
}

TEST_P(CatalogClientMockTest, GetCatalog403) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_CONFIG), testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(returnsResponse({403, HTTP_RESPONSE_403})));

  auto catalogClient = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = CatalogRequest();
  auto future = catalogClient->GetCatalog(request);
  CatalogResponse catalogResponse = future.GetFuture().get();

  ASSERT_FALSE(catalogResponse.IsSuccessful())
      << PrintError(catalogResponse.GetError());
  ASSERT_EQ(403, catalogResponse.GetError().GetHttpStatusCode());
}

TEST_P(CatalogClientMockTest, GetPartitions) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_PARTITIONS), testing::_, testing::_))
      .Times(1);

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer");
  auto future = catalogClient->GetPartitions(request);
  auto partitionsResponse = future.GetFuture().get();

  ASSERT_TRUE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(4u, partitionsResponse.GetResult().GetPartitions().size());
}

TEST_P(CatalogClientMockTest, GetDataWithPartitionId) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_BLOB_DATA_269), testing::_, testing::_))
      .Times(1);

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");
  auto future = catalogClient->GetData(request);

  auto dataResponse = future.GetFuture().get();

  ASSERT_TRUE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_LT(0, dataResponse.GetResult()->size());
  std::string dataStr(dataResponse.GetResult()->begin(),
                      dataResponse.GetResult()->end());
  ASSERT_EQ("DT_2_0031", dataStr);
}

TEST_P(CatalogClientMockTest, GetDataWithInlineField) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_PARTITION_3), testing::_, testing::_))
      .Times(1);

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("3");
  auto future = catalogClient->GetData(request);

  auto dataResponse = future.GetFuture().get();

  ASSERT_TRUE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_LT(0, dataResponse.GetResult()->size());
  std::string dataStr(dataResponse.GetResult()->begin(),
                      dataResponse.GetResult()->end());
  ASSERT_EQ(0u, dataStr.find("data:"));
}

TEST_P(CatalogClientMockTest, GetEmptyPartitions) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_PARTITIONS), testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(
          returnsResponse({200, HTTP_RESPONSE_EMPTY_PARTITIONS})));

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer");
  auto future = catalogClient->GetPartitions(request);
  auto partitionsResponse = future.GetFuture().get();

  ASSERT_TRUE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(0u, partitionsResponse.GetResult().GetPartitions().size());
}

TEST_P(CatalogClientMockTest, GetVolatileDataHandle) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*handler_,
              op(IsGetRequest(
                     "https://volatile-blob-ireland.data.api.platform.here.com/"
                     "blobstore/v1/catalogs/hereos-internal-test-v2/layers/"
                     "testlayer_volatile/data/volatileHandle"),
                 testing::_, testing::_))
      .Times(1)
      .WillRepeatedly(testing::Invoke(returnsResponse({200, "someData"})));

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer_volatile").WithDataHandle("volatileHandle");

  auto future = catalogClient->GetData(request);

  auto dataResponse = future.GetFuture().get();

  ASSERT_TRUE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_LT(0, dataResponse.GetResult()->size());
  std::string dataStr(dataResponse.GetResult()->begin(),
                      dataResponse.GetResult()->end());
  ASSERT_EQ("someData", dataStr);
}

TEST_P(CatalogClientMockTest, GetVolatilePartitions) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_LATEST_CATALOG_VERSION),
                            testing::_, testing::_))
      .Times(0);

  EXPECT_CALL(
      *handler_,
      op(IsGetRequest(
             "https://metadata.data.api.platform.here.com/metadata/v1/catalogs/"
             "hereos-internal-test-v2/layers/testlayer_volatile/partitions"),
         testing::_, testing::_))
      .Times(1)
      .WillRepeatedly(
          testing::Invoke(returnsResponse({200, HTTP_RESPONSE_PARTITIONS_V2})));

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer_volatile");

  auto future = catalogClient->GetPartitions(request);

  auto partitionsResponse = future.GetFuture().get();

  ASSERT_TRUE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(1u, partitionsResponse.GetResult().GetPartitions().size());

  request.WithVersion(18);
  future = catalogClient->GetPartitions(request);
  partitionsResponse = future.GetFuture().get();

  ASSERT_TRUE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(1u, partitionsResponse.GetResult().GetPartitions().size());
}

TEST_P(CatalogClientMockTest, GetVolatileDataByPartitionId) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_LATEST_CATALOG_VERSION),
                            testing::_, testing::_))
      .Times(0);

  EXPECT_CALL(*handler_,
              op(IsGetRequest("https://query.data.api.platform.here.com/query/"
                              "v1/catalogs/hereos-internal-test-v2/layers/"
                              "testlayer_volatile/partitions?partition=269"),
                 testing::_, testing::_))
      .Times(1)
      .WillRepeatedly(
          testing::Invoke(returnsResponse({200, HTTP_RESPONSE_PARTITIONS_V2})));

  EXPECT_CALL(
      *handler_,
      op(IsGetRequest(
             "https://volatile-blob-ireland.data.api.platform.here.com/"
             "blobstore/v1/catalogs/hereos-internal-test-v2/layers/"
             "testlayer_volatile/data/4eed6ed1-0d32-43b9-ae79-043cb4256410"),
         testing::_, testing::_))
      .Times(1)
      .WillRepeatedly(testing::Invoke(returnsResponse({200, "someData"})));

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer_volatile").WithPartitionId("269");

  auto future = catalogClient->GetData(request);

  auto dataResponse = future.GetFuture().get();

  ASSERT_TRUE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_LT(0, dataResponse.GetResult()->size());
  std::string dataStr(dataResponse.GetResult()->begin(),
                      dataResponse.GetResult()->end());
  ASSERT_EQ("someData", dataStr);
}

TEST_P(CatalogClientMockTest, GetStreamDataHandle) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer_stream").WithDataHandle("streamHandle");

  auto future = catalogClient->GetData(request);

  auto dataResponse = future.GetFuture().get();

  ASSERT_FALSE(dataResponse.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::ServiceUnavailable,
            dataResponse.GetError().GetErrorCode());
}

TEST_P(CatalogClientMockTest, GetData429Error) {
  olp::client::HRN hrn(GetTestCatalog());

  {
    testing::InSequence s;
    EXPECT_CALL(*handler_,
                op(IsGetRequest(URL_BLOB_DATA_269), testing::_, testing::_))
        .Times(2)
        .WillRepeatedly(testing::Invoke(
            returnsResponse({429, "Server busy at the moment."})));
    EXPECT_CALL(*handler_,
                op(IsGetRequest(URL_BLOB_DATA_269), testing::_, testing::_))
        .Times(1);
  }

  olp::client::RetrySettings retrySettings;
  retrySettings.retry_condition =
      [](const olp::network::HttpResponse& response) {
        return 429 == response.status;
      };
  settings_->retry_settings = retrySettings;
  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer")
      .WithDataHandle("4eed6ed1-0d32-43b9-ae79-043cb4256432");

  auto future = catalogClient->GetData(request);

  auto dataResponse = future.GetFuture().get();

  ASSERT_TRUE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_LT(0, dataResponse.GetResult()->size());
  std::string dataStr(dataResponse.GetResult()->begin(),
                      dataResponse.GetResult()->end());
  ASSERT_EQ("DT_2_0031", dataStr);
}

TEST_P(CatalogClientMockTest, GetPartitions429Error) {
  olp::client::HRN hrn(GetTestCatalog());

  {
    testing::InSequence s;
    EXPECT_CALL(*handler_,
                op(IsGetRequest(URL_PARTITIONS), testing::_, testing::_))
        .Times(2)
        .WillRepeatedly(testing::Invoke(
            returnsResponse({429, "Server busy at the moment."})));
    EXPECT_CALL(*handler_,
                op(IsGetRequest(URL_PARTITIONS), testing::_, testing::_))
        .Times(1);
  }

  olp::client::RetrySettings retrySettings;
  retrySettings.retry_condition =
      [](const olp::network::HttpResponse& response) {
        return 429 == response.status;
      };
  settings_->retry_settings = retrySettings;
  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer");
  auto future = catalogClient->GetPartitions(request);
  auto partitionsResponse = future.GetFuture().get();

  ASSERT_TRUE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(4u, partitionsResponse.GetResult().GetPartitions().size());
}

TEST_P(CatalogClientMockTest, ApiLookup429) {
  olp::client::HRN hrn(GetTestCatalog());

  {
    testing::InSequence s;
    EXPECT_CALL(*handler_,
                op(IsGetRequest(URL_LOOKUP_METADATA), testing::_, testing::_))
        .Times(2)
        .WillRepeatedly(testing::Invoke(
            returnsResponse({429, "Server busy at the moment."})));
    EXPECT_CALL(*handler_,
                op(IsGetRequest(URL_LOOKUP_METADATA), testing::_, testing::_))
        .Times(1);
  }

  olp::client::RetrySettings retrySettings;
  retrySettings.retry_condition =
      [](const olp::network::HttpResponse& response) {
        return 429 == response.status;
      };
  settings_->retry_settings = retrySettings;
  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer");
  auto future = catalogClient->GetPartitions(request);
  auto partitionsResponse = future.GetFuture().get();

  ASSERT_TRUE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(4u, partitionsResponse.GetResult().GetPartitions().size());
}

TEST_P(CatalogClientMockTest, GetPartitionsForInvalidLayer) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("invalidLayer");
  auto future = catalogClient->GetPartitions(request);
  auto partitionsResponse = future.GetFuture().get();

  ASSERT_FALSE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(olp::client::ErrorCode::InvalidArgument,
            partitionsResponse.GetError().GetErrorCode());
}

TEST_P(CatalogClientMockTest, GetData404Error) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*handler_,
              op(IsGetRequest("https://blob-ireland.data.api.platform.here.com/"
                              "blobstore/v1/catalogs/hereos-internal-test-v2/"
                              "layers/testlayer/data/invalidDataHandle"),
                 testing::_, testing::_))
      .Times(1)
      .WillRepeatedly(
          testing::Invoke(returnsResponse({404, "Resource not found."})));

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithDataHandle("invalidDataHandle");
  auto future = catalogClient->GetData(request);

  auto dataResponse = future.GetFuture().get();

  ASSERT_FALSE(dataResponse.IsSuccessful());
  ASSERT_EQ(404, dataResponse.GetError().GetHttpStatusCode());
}

TEST_P(CatalogClientMockTest, GetPartitionsGarbageResponse) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_METADATA), testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(
          returnsResponse({200, R"jsonString(kd3sdf\)jsonString"})));

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer");
  auto future = catalogClient->GetPartitions(request);
  auto partitionsResponse = future.GetFuture().get();

  ASSERT_FALSE(partitionsResponse.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::ServiceUnavailable,
            partitionsResponse.GetError().GetErrorCode());
}

TEST_P(CatalogClientMockTest, GetCatalogCancelApiLookup) {
  olp::client::HRN hrn(GetTestCatalog());

  auto waitForCancel = std::make_shared<std::promise<void>>();
  auto pauseForCancel = std::make_shared<std::promise<void>>();

  // Setup the expected calls :
  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_CONFIG), testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(setsPromiseWaitsAndReturns(
          waitForCancel, pauseForCancel, {200, HTTP_RESPONSE_LOOKUP_CONFIG})));

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_CONFIG), testing::_, testing::_))
      .Times(0);

  // Run it!
  auto catalogClient = std::make_unique<CatalogClient>(hrn, settings_);

  auto request = CatalogRequest();

  std::promise<CatalogResponse> promise;
  CatalogResponseCallback callback = [&promise](CatalogResponse response) {
    promise.set_value(response);
  };
  olp::client::CancellationToken cancelToken =
      catalogClient->GetCatalog(request, callback);

  waitForCancel->get_future().get();
  cancelToken.cancel();
  pauseForCancel->set_value();
  CatalogResponse catalogResponse = promise.get_future().get();

  ASSERT_FALSE(catalogResponse.IsSuccessful())
      << PrintError(catalogResponse.GetError());

  ASSERT_EQ(olp::network::Network::ErrorCode::Cancelled,
            static_cast<int>(catalogResponse.GetError().GetHttpStatusCode()));
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            catalogResponse.GetError().GetErrorCode());
}

TEST_P(CatalogClientMockTest, GetCatalogCancelConfig) {
  olp::client::HRN hrn(GetTestCatalog());

  auto waitForCancel = std::make_shared<std::promise<void>>();
  auto pauseForCancel = std::make_shared<std::promise<void>>();

  // Setup the expected calls :
  EXPECT_CALL(*handler_, op(IsGetRequest(URL_CONFIG), testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(setsPromiseWaitsAndReturns(
          waitForCancel, pauseForCancel, {200, HTTP_RESPONSE_CONFIG})));

  // Run it!
  auto catalogClient = std::make_unique<CatalogClient>(hrn, settings_);

  auto request = CatalogRequest();

  std::promise<CatalogResponse> promise;
  CatalogResponseCallback callback = [&promise](CatalogResponse response) {
    promise.set_value(response);
  };
  olp::client::CancellationToken cancelToken =
      catalogClient->GetCatalog(request, callback);

  waitForCancel->get_future().get();
  std::cout << "Cancelling" << std::endl;
  cancelToken.cancel();  // crashing?
  std::cout << "Cancelled, unblocking response" << std::endl;
  pauseForCancel->set_value();
  std::cout << "Post Cancel, get response" << std::endl;
  CatalogResponse catalogResponse = promise.get_future().get();

  ASSERT_FALSE(catalogResponse.IsSuccessful())
      << PrintError(catalogResponse.GetError());

  ASSERT_EQ(olp::network::Network::ErrorCode::Cancelled,
            static_cast<int>(catalogResponse.GetError().GetHttpStatusCode()));
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            catalogResponse.GetError().GetErrorCode());
  std::cout << "Post Test" << std::endl;
}

TEST_P(CatalogClientMockTest, GetCatalogCancelAfterCompletion) {
  olp::client::HRN hrn(GetTestCatalog());

  // Run it!
  auto catalogClient = std::make_unique<CatalogClient>(hrn, settings_);

  auto request = CatalogRequest();

  std::promise<CatalogResponse> promise;
  CatalogResponseCallback callback = [&promise](CatalogResponse response) {
    promise.set_value(response);
  };
  olp::client::CancellationToken cancelToken =
      catalogClient->GetCatalog(request, callback);

  CatalogResponse catalogResponse = promise.get_future().get();

  ASSERT_TRUE(catalogResponse.IsSuccessful())
      << PrintError(catalogResponse.GetError());

  cancelToken.cancel();
}

TEST_P(CatalogClientMockTest, GetPartitionsCancelLookupMetadata) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto waitForCancel = std::make_shared<std::promise<void>>();
  auto pauseForCancel = std::make_shared<std::promise<void>>();

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_METADATA), testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(
          setsPromiseWaitsAndReturns(waitForCancel, pauseForCancel,
                                     {200, HTTP_RESPONSE_LOOKUP_METADATA})));

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_LATEST_CATALOG_VERSION),
                            testing::_, testing::_))
      .Times(0);

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request =
      olp::dataservice::read::PartitionsRequest().WithLayerId("testlayer");

  std::promise<PartitionsResponse> promise;
  PartitionsResponseCallback callback =
      [&promise](PartitionsResponse response) { promise.set_value(response); };

  olp::client::CancellationToken cancelToken =
      catalogClient->GetPartitions(request, callback);

  waitForCancel->get_future().get();  // wait for handler to get the request
  cancelToken.cancel();
  pauseForCancel->set_value();  // unblock the handler
  PartitionsResponse partitionsResponse = promise.get_future().get();

  ASSERT_FALSE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(
      olp::network::Network::ErrorCode::Cancelled,
      static_cast<int>(partitionsResponse.GetError().GetHttpStatusCode()));
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            partitionsResponse.GetError().GetErrorCode());
}

TEST_P(CatalogClientMockTest, GetPartitionsCancelLatestCatalogVersion) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto waitForCancel = std::make_shared<std::promise<void>>();
  auto pauseForCancel = std::make_shared<std::promise<void>>();

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_LATEST_CATALOG_VERSION),
                            testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(setsPromiseWaitsAndReturns(
          waitForCancel, pauseForCancel,
          {200, HTTP_RESPONSE_LATEST_CATALOG_VERSION})));

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LAYER_VERSIONS), testing::_, testing::_))
      .Times(0);

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request =
      olp::dataservice::read::PartitionsRequest().WithLayerId("testlayer");

  std::promise<PartitionsResponse> promise;
  PartitionsResponseCallback callback =
      [&promise](PartitionsResponse response) { promise.set_value(response); };

  olp::client::CancellationToken cancelToken =
      catalogClient->GetPartitions(request, callback);

  waitForCancel->get_future().get();  // wait for handler to get the request
  cancelToken.cancel();
  pauseForCancel->set_value();  // unblock the handler
  PartitionsResponse partitionsResponse = promise.get_future().get();

  ASSERT_FALSE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(olp::network::Network::ErrorCode::Cancelled,
            static_cast<int>(partitionsResponse.GetError().GetHttpStatusCode()))
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            partitionsResponse.GetError().GetErrorCode())
      << PrintError(partitionsResponse.GetError());
}

TEST_P(CatalogClientMockTest, GetPartitionsCancelLayerVersions) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto waitForCancel = std::make_shared<std::promise<void>>();
  auto pauseForCancel = std::make_shared<std::promise<void>>();

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LAYER_VERSIONS), testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(setsPromiseWaitsAndReturns(
          waitForCancel, pauseForCancel, {200, HTTP_RESPONSE_LAYER_VERSIONS})));

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_PARTITIONS), testing::_, testing::_))
      .Times(0);

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request =
      olp::dataservice::read::PartitionsRequest().WithLayerId("testlayer");

  std::promise<PartitionsResponse> promise;
  PartitionsResponseCallback callback =
      [&promise](PartitionsResponse response) { promise.set_value(response); };

  olp::client::CancellationToken cancelToken =
      catalogClient->GetPartitions(request, callback);
  waitForCancel->get_future().get();  // wait for handler to get the request
  cancelToken.cancel();
  pauseForCancel->set_value();  // unblock the handler

  PartitionsResponse partitionsResponse = promise.get_future().get();

  ASSERT_FALSE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(olp::network::Network::ErrorCode::Cancelled,
            static_cast<int>(partitionsResponse.GetError().GetHttpStatusCode()))
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            partitionsResponse.GetError().GetErrorCode())
      << PrintError(partitionsResponse.GetError());
}

TEST_P(CatalogClientMockTest, GetDataWithPartitionIdCancelLookupConfig) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto waitForCancel = std::make_shared<std::promise<void>>();
  auto pauseForCancel = std::make_shared<std::promise<void>>();

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_CONFIG), testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(setsPromiseWaitsAndReturns(
          waitForCancel, pauseForCancel, {200, HTTP_RESPONSE_LOOKUP_CONFIG})));

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_CONFIG), testing::_, testing::_))
      .Times(0);

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancelToken =
      catalogClient->GetData(request, callback);

  waitForCancel->get_future().get();  // wait for handler to get the request
  cancelToken.cancel();
  pauseForCancel->set_value();  // unblock the handler

  DataResponse dataResponse = promise.get_future().get();

  ASSERT_FALSE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_EQ(olp::network::Network::ErrorCode::Cancelled,
            static_cast<int>(dataResponse.GetError().GetHttpStatusCode()))
      << PrintError(dataResponse.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            dataResponse.GetError().GetErrorCode())
      << PrintError(dataResponse.GetError());
}

TEST_P(CatalogClientMockTest, GetDataWithPartitionIdCancelConfig) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto waitForCancel = std::make_shared<std::promise<void>>();
  auto pauseForCancel = std::make_shared<std::promise<void>>();

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_CONFIG), testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(setsPromiseWaitsAndReturns(
          waitForCancel, pauseForCancel, {200, HTTP_RESPONSE_CONFIG})));

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_METADATA), testing::_, testing::_))
      .Times(0);

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancelToken =
      catalogClient->GetData(request, callback);

  waitForCancel->get_future().get();  // wait for handler to get the request
  cancelToken.cancel();
  pauseForCancel->set_value();  // unblock the handler

  DataResponse dataResponse = promise.get_future().get();

  ASSERT_FALSE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_EQ(olp::network::Network::ErrorCode::Cancelled,
            static_cast<int>(dataResponse.GetError().GetHttpStatusCode()))
      << PrintError(dataResponse.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            dataResponse.GetError().GetErrorCode())
      << PrintError(dataResponse.GetError());
}

TEST_P(CatalogClientMockTest, GetDataWithPartitionIdCancelLookupMetadata) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto waitForCancel = std::make_shared<std::promise<void>>();
  auto pauseForCancel = std::make_shared<std::promise<void>>();

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_METADATA), testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(
          setsPromiseWaitsAndReturns(waitForCancel, pauseForCancel,
                                     {200, HTTP_RESPONSE_LOOKUP_METADATA})));

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_LATEST_CATALOG_VERSION),
                            testing::_, testing::_))
      .Times(0);

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancelToken =
      catalogClient->GetData(request, callback);

  waitForCancel->get_future().get();  // wait for handler to get the request
  cancelToken.cancel();
  pauseForCancel->set_value();  // unblock the handler

  DataResponse dataResponse = promise.get_future().get();

  ASSERT_FALSE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_EQ(olp::network::Network::ErrorCode::Cancelled,
            static_cast<int>(dataResponse.GetError().GetHttpStatusCode()))
      << PrintError(dataResponse.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            dataResponse.GetError().GetErrorCode())
      << PrintError(dataResponse.GetError());
}

TEST_P(CatalogClientMockTest,
       GetDataWithPartitionIdCancelLatestCatalogVersion) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto waitForCancel = std::make_shared<std::promise<void>>();
  auto pauseForCancel = std::make_shared<std::promise<void>>();

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_LATEST_CATALOG_VERSION),
                            testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(setsPromiseWaitsAndReturns(
          waitForCancel, pauseForCancel,
          {200, HTTP_RESPONSE_LATEST_CATALOG_VERSION})));

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_QUERY), testing::_, testing::_))
      .Times(0);

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancelToken =
      catalogClient->GetData(request, callback);

  waitForCancel->get_future().get();  // wait for handler to get the request
  cancelToken.cancel();
  pauseForCancel->set_value();  // unblock the handler

  DataResponse dataResponse = promise.get_future().get();

  ASSERT_FALSE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_EQ(olp::network::Network::ErrorCode::Cancelled,
            static_cast<int>(dataResponse.GetError().GetHttpStatusCode()))
      << PrintError(dataResponse.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            dataResponse.GetError().GetErrorCode())
      << PrintError(dataResponse.GetError());
}

TEST_P(CatalogClientMockTest, GetDataWithPartitionIdCancelInnerConfig) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto waitForCancel = std::make_shared<std::promise<void>>();
  auto pauseForCancel = std::make_shared<std::promise<void>>();

  {
    testing::InSequence s;
    EXPECT_CALL(*handler_, op(IsGetRequest(URL_CONFIG), testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(*handler_, op(IsGetRequest(URL_CONFIG), testing::_, testing::_))
        .Times(1)
        .WillOnce(testing::Invoke(setsPromiseWaitsAndReturns(
            waitForCancel, pauseForCancel, {200, HTTP_RESPONSE_CONFIG})));
  }

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_LATEST_CATALOG_VERSION),
                            testing::_, testing::_))
      .Times(0);

  olp::cache::CacheSettings cacheSettings;
  cacheSettings.max_memory_cache_size = 0;
  auto catalogClient = std::make_unique<olp::dataservice::read::CatalogClient>(
      hrn, settings_,
      olp::dataservice::read::CreateDefaultCache(cacheSettings));

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancelToken =
      catalogClient->GetData(request, callback);

  waitForCancel->get_future().get();  // wait for handler to get the request
  cancelToken.cancel();
  pauseForCancel->set_value();  // unblock the handler

  DataResponse dataResponse = promise.get_future().get();

  ASSERT_FALSE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_EQ(olp::network::Network::ErrorCode::Cancelled,
            static_cast<int>(dataResponse.GetError().GetHttpStatusCode()))
      << PrintError(dataResponse.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            dataResponse.GetError().GetErrorCode())
      << PrintError(dataResponse.GetError());
}

TEST_P(CatalogClientMockTest, GetDataWithPartitionIdCancelLookupQuery) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto waitForCancel = std::make_shared<std::promise<void>>();
  auto pauseForCancel = std::make_shared<std::promise<void>>();

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_QUERY), testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(setsPromiseWaitsAndReturns(
          waitForCancel, pauseForCancel, {200, HTTP_RESPONSE_LOOKUP_QUERY})));

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_QUERY_PARTITION_269), testing::_, testing::_))
      .Times(0);

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancelToken =
      catalogClient->GetData(request, callback);

  waitForCancel->get_future().get();  // wait for handler to get the request
  cancelToken.cancel();
  pauseForCancel->set_value();  // unblock the handler

  DataResponse dataResponse = promise.get_future().get();

  ASSERT_FALSE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_EQ(olp::network::Network::ErrorCode::Cancelled,
            static_cast<int>(dataResponse.GetError().GetHttpStatusCode()))
      << PrintError(dataResponse.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            dataResponse.GetError().GetErrorCode())
      << PrintError(dataResponse.GetError());
}

TEST_P(CatalogClientMockTest, GetDataWithPartitionIdCancelQuery) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto waitForCancel = std::make_shared<std::promise<void>>();
  auto pauseForCancel = std::make_shared<std::promise<void>>();

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_QUERY_PARTITION_269), testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(setsPromiseWaitsAndReturns(
          waitForCancel, pauseForCancel, {200, HTTP_RESPONSE_PARTITION_269})));

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_BLOB), testing::_, testing::_))
      .Times(0);

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancelToken =
      catalogClient->GetData(request, callback);

  waitForCancel->get_future().get();  // wait for handler to get the request
  cancelToken.cancel();
  pauseForCancel->set_value();  // unblock the handler

  DataResponse dataResponse = promise.get_future().get();

  ASSERT_FALSE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_EQ(olp::network::Network::ErrorCode::Cancelled,
            static_cast<int>(dataResponse.GetError().GetHttpStatusCode()))
      << PrintError(dataResponse.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            dataResponse.GetError().GetErrorCode())
      << PrintError(dataResponse.GetError());
}

TEST_P(CatalogClientMockTest, GetDataWithPartitionIdCancelLookupBlob) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto waitForCancel = std::make_shared<std::promise<void>>();
  auto pauseForCancel = std::make_shared<std::promise<void>>();

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_BLOB), testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(setsPromiseWaitsAndReturns(
          waitForCancel, pauseForCancel, {200, HTTP_RESPONSE_LOOKUP_BLOB})));

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_BLOB_DATA_269), testing::_, testing::_))
      .Times(0);

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancelToken =
      catalogClient->GetData(request, callback);

  waitForCancel->get_future().get();  // wait for handler to get the request
  cancelToken.cancel();
  pauseForCancel->set_value();  // unblock the handler

  DataResponse dataResponse = promise.get_future().get();

  ASSERT_FALSE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_EQ(olp::network::Network::ErrorCode::Cancelled,
            static_cast<int>(dataResponse.GetError().GetHttpStatusCode()))
      << PrintError(dataResponse.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            dataResponse.GetError().GetErrorCode())
      << PrintError(dataResponse.GetError());
}

TEST_P(CatalogClientMockTest, GetDataWithPartitionIdCancelBlob) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto waitForCancel = std::make_shared<std::promise<void>>();
  auto pauseForCancel = std::make_shared<std::promise<void>>();

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_BLOB_DATA_269), testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(setsPromiseWaitsAndReturns(
          waitForCancel, pauseForCancel, {200, HTTP_RESPONSE_BLOB_DATA_269})));

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancelToken =
      catalogClient->GetData(request, callback);

  waitForCancel->get_future().get();  // wait for handler to get the request
  cancelToken.cancel();
  pauseForCancel->set_value();  // unblock the handler

  DataResponse dataResponse = promise.get_future().get();

  ASSERT_FALSE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_EQ(olp::network::Network::ErrorCode::Cancelled,
            static_cast<int>(dataResponse.GetError().GetHttpStatusCode()))
      << PrintError(dataResponse.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            dataResponse.GetError().GetErrorCode())
      << PrintError(dataResponse.GetError());
}

TEST_P(CatalogClientMockTest, GetCatalogVersion) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_METADATA), testing::_, testing::_))
      .Times(1);

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_LATEST_CATALOG_VERSION),
                            testing::_, testing::_))
      .Times(1);

  auto catalogClient = std::make_unique<CatalogClient>(hrn, settings_);

  auto request = CatalogVersionRequest().WithStartVersion(-1);

  auto future = catalogClient->GetCatalogMetadataVersion(request);
  auto catalogVersionResponse = future.GetFuture().get();

  ASSERT_TRUE(catalogVersionResponse.IsSuccessful())
      << PrintError(catalogVersionResponse.GetError());
}

TEST_P(CatalogClientMockTest, GetDataWithPartitionIdVersion2) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_LATEST_CATALOG_VERSION),
                            testing::_, testing::_))
      .Times(0);

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LAYER_VERSIONS_V2), testing::_, testing::_))
      .Times(0);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269").WithVersion(2);
  auto dataResponse =
      GetExecutionTime<olp::dataservice::read::DataResponse>([&] {
        auto future = catalogClient->GetData(request);
        return future.GetFuture().get();
      });

  ASSERT_TRUE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_LT(0, dataResponse.GetResult()->size());
  std::string dataStr(dataResponse.GetResult()->begin(),
                      dataResponse.GetResult()->end());
  ASSERT_EQ("DT_2_0031_V2", dataStr);
}

TEST_P(CatalogClientMockTest, GetDataWithPartitionIdInvalidVersion) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269").WithVersion(10);
  auto dataResponse =
      GetExecutionTime<olp::dataservice::read::DataResponse>([&] {
        auto future = catalogClient->GetData(request);
        return future.GetFuture().get();
      });

  ASSERT_FALSE(dataResponse.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::BadRequest,
            dataResponse.GetError().GetErrorCode());
  ASSERT_EQ(400, dataResponse.GetError().GetHttpStatusCode());

  request.WithVersion(-1);
  dataResponse = GetExecutionTime<olp::dataservice::read::DataResponse>([&] {
    auto future = catalogClient->GetData(request);
    return future.GetFuture().get();
  });

  ASSERT_FALSE(dataResponse.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::BadRequest,
            dataResponse.GetError().GetErrorCode());
  ASSERT_EQ(400, dataResponse.GetError().GetHttpStatusCode());
}

TEST_P(CatalogClientMockTest, GetPartitionsVersion2) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_LATEST_CATALOG_VERSION),
                            testing::_, testing::_))
      .Times(0);

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LAYER_VERSIONS_V2), testing::_, testing::_))
      .Times(1);

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer").WithVersion(2);
  auto partitionsResponse =
      GetExecutionTime<olp::dataservice::read::PartitionsResponse>([&] {
        auto future = catalogClient->GetPartitions(request);
        return future.GetFuture().get();
      });

  ASSERT_TRUE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(1u, partitionsResponse.GetResult().GetPartitions().size());
}

TEST_P(CatalogClientMockTest, GetPartitionsInvalidVersion) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer").WithVersion(10);
  auto partitionsResponse =
      GetExecutionTime<olp::dataservice::read::PartitionsResponse>([&] {
        auto future = catalogClient->GetPartitions(request);
        return future.GetFuture().get();
      });

  ASSERT_FALSE(partitionsResponse.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::BadRequest,
            partitionsResponse.GetError().GetErrorCode());
  ASSERT_EQ(400, partitionsResponse.GetError().GetHttpStatusCode());

  request.WithVersion(-1);
  partitionsResponse =
      GetExecutionTime<olp::dataservice::read::PartitionsResponse>([&] {
        auto future = catalogClient->GetPartitions(request);
        return future.GetFuture().get();
      });

  ASSERT_FALSE(partitionsResponse.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::BadRequest,
            partitionsResponse.GetError().GetErrorCode());
  ASSERT_EQ(400, partitionsResponse.GetError().GetHttpStatusCode());
}

TEST_P(CatalogClientMockTest, GetCatalogVersionCancel) {
  olp::client::HRN hrn(GetTestCatalog());

  auto waitForCancel = std::make_shared<std::promise<void>>();
  auto pauseForCancel = std::make_shared<std::promise<void>>();

  // Setup the expected calls :
  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_METADATA), testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(
          setsPromiseWaitsAndReturns(waitForCancel, pauseForCancel,
                                     {200, HTTP_RESPONSE_LOOKUP_METADATA})));

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_LATEST_CATALOG_VERSION),
                            testing::_, testing::_))
      .Times(0);

  // Run it!
  auto catalogClient = std::make_unique<CatalogClient>(hrn, settings_);

  auto request = CatalogVersionRequest().WithStartVersion(-1);

  std::promise<CatalogVersionResponse> promise;
  CatalogVersionCallback callback =
      [&promise](CatalogVersionResponse response) {
        promise.set_value(response);
      };
  olp::client::CancellationToken cancelToken =
      catalogClient->GetCatalogMetadataVersion(request, callback);

  waitForCancel->get_future().get();
  cancelToken.cancel();
  pauseForCancel->set_value();
  CatalogVersionResponse versionResponse = promise.get_future().get();

  ASSERT_FALSE(versionResponse.IsSuccessful())
      << PrintError(versionResponse.GetError());

  ASSERT_EQ(olp::network::Network::ErrorCode::Cancelled,
            static_cast<int>(versionResponse.GetError().GetHttpStatusCode()));
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            versionResponse.GetError().GetErrorCode());
}

TEST_P(CatalogClientMockTest, GetCatalogCacheOnly) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_CONFIG), testing::_, testing::_))
      .Times(0);

  auto catalogClient = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = CatalogRequest();
  request.WithFetchOption(CacheOnly);
  auto future = catalogClient->GetCatalog(request);
  CatalogResponse catalogResponse = future.GetFuture().get();
  ASSERT_FALSE(catalogResponse.IsSuccessful())
      << PrintError(catalogResponse.GetError());
}

TEST_P(CatalogClientMockTest, GetCatalogOnlineOnly) {
  olp::client::HRN hrn(GetTestCatalog());

  {
    testing::InSequence s;
    EXPECT_CALL(*handler_, op(IsGetRequest(URL_CONFIG), testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(*handler_, op(IsGetRequest(URL_CONFIG), testing::_, testing::_))
        .Times(1)
        .WillOnce(testing::Invoke(
            returnsResponse({429, "Server busy at the moment."})));
  }

  auto catalogClient = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = CatalogRequest();
  request.WithFetchOption(OnlineOnly);
  auto future = catalogClient->GetCatalog(request);
  CatalogResponse catalogResponse = future.GetFuture().get();
  ASSERT_TRUE(catalogResponse.IsSuccessful())
      << PrintError(catalogResponse.GetError());
  future = catalogClient->GetCatalog(request);
  // Should fail despite valid cache entry.
  catalogResponse = future.GetFuture().get();
  ASSERT_FALSE(catalogResponse.IsSuccessful())
      << PrintError(catalogResponse.GetError());
}

TEST_P(CatalogClientMockTest, GetCatalogCacheWithUpdate) {
  olp::logging::Log::setLevel(olp::logging::Level::Trace);

  olp::client::HRN hrn(GetTestCatalog());
  auto waitToStartSignal = std::make_shared<std::promise<void>>();
  auto preCallbackWait = std::make_shared<std::promise<void>>();
  preCallbackWait->set_value();
  auto waitForEnd = std::make_shared<std::promise<void>>();

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_CONFIG), testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(
          setsPromiseWaitsAndReturns(waitToStartSignal, preCallbackWait,
                                     {200, HTTP_RESPONSE_CONFIG}, waitForEnd)));

  auto catalogClient = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = CatalogRequest();
  request.WithFetchOption(CacheWithUpdate);
  // Request 1
  EDGE_SDK_LOG_TRACE_F("CatalogClientMockTest",
                       "Request Catalog, CacheWithUpdate");
  auto future = catalogClient->GetCatalog(request);

  EDGE_SDK_LOG_TRACE_F("CatalogClientMockTest", "get CatalogResponse1");
  CatalogResponse catalogResponse = future.GetFuture().get();

  // Request 1 return. Cached value (nothing)
  ASSERT_FALSE(catalogResponse.IsSuccessful())
      << PrintError(catalogResponse.GetError());
  // Wait for background cache update to finish
  EDGE_SDK_LOG_TRACE_F("CatalogClientMockTest",
                       "wait some time for update to conclude");
  waitForEnd->get_future().get();

  // Request 2 to check there is a cached value.
  EDGE_SDK_LOG_TRACE_F("CatalogClientMockTest", "Request Catalog, CacheOnly");
  request.WithFetchOption(CacheOnly);
  future = catalogClient->GetCatalog(request);
  EDGE_SDK_LOG_TRACE_F("CatalogClientMockTest", "get CatalogResponse2");
  catalogResponse = future.GetFuture().get();
  // Cache should be available here.
  ASSERT_TRUE(catalogResponse.IsSuccessful())
      << PrintError(catalogResponse.GetError());
}

TEST_P(CatalogClientMockTest, GetDataCacheOnly) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_BLOB_DATA_269), testing::_, testing::_))
      .Times(0);

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer")
      .WithPartitionId("269")
      .WithFetchOption(CacheOnly);
  auto future = catalogClient->GetData(request);
  auto dataResponse = future.GetFuture().get();
  ASSERT_FALSE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
}

TEST_P(CatalogClientMockTest, GetDataOnlineOnly) {
  olp::client::HRN hrn(GetTestCatalog());

  {
    testing::InSequence s;
    EXPECT_CALL(*handler_,
                op(IsGetRequest(URL_BLOB_DATA_269), testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(*handler_,
                op(IsGetRequest(URL_BLOB_DATA_269), testing::_, testing::_))
        .Times(1)
        .WillOnce(testing::Invoke(
            returnsResponse({429, "Server busy at the moment."})));
  }

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer")
      .WithPartitionId("269")
      .WithFetchOption(OnlineOnly);
  auto future = catalogClient->GetData(request);

  auto dataResponse = future.GetFuture().get();

  ASSERT_TRUE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_LT(0, dataResponse.GetResult()->size());
  std::string dataStr(dataResponse.GetResult()->begin(),
                      dataResponse.GetResult()->end());
  ASSERT_EQ("DT_2_0031", dataStr);
  // Should fail despite cached response
  future = catalogClient->GetData(request);
  dataResponse = future.GetFuture().get();
  ASSERT_FALSE(dataResponse.IsSuccessful());
}

TEST_P(CatalogClientMockTest, GetDataCacheWithUpdate) {
  olp::logging::Log::setLevel(olp::logging::Level::Trace);

  olp::client::HRN hrn(GetTestCatalog());
  // Setup the expected calls :
  auto waitToStartSignal = std::make_shared<std::promise<void>>();
  auto preCallbackWait = std::make_shared<std::promise<void>>();
  preCallbackWait->set_value();
  auto waitForEndSignal = std::make_shared<std::promise<void>>();

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_BLOB_DATA_269), testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(setsPromiseWaitsAndReturns(
          waitToStartSignal, preCallbackWait,
          {200, HTTP_RESPONSE_BLOB_DATA_269}, waitForEndSignal)));

  auto catalogClient = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = DataRequest();
  request.WithLayerId("testlayer")
      .WithPartitionId("269")
      .WithFetchOption(CacheWithUpdate);
  // Request 1
  auto future = catalogClient->GetData(request);
  DataResponse dataResponse = future.GetFuture().get();
  // Request 1 return. Cached value (nothing)
  ASSERT_FALSE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  // Request 2 to check there is a cached value.
  // waiting for cache to fill-in
  waitForEndSignal->get_future().get();
  request.WithFetchOption(CacheOnly);
  future = catalogClient->GetData(request);
  dataResponse = future.GetFuture().get();
  // Cache should be available here.
  ASSERT_TRUE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
}

TEST_P(CatalogClientMockTest, GetPartitionsCacheOnly) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_PARTITIONS), testing::_, testing::_))
      .Times(0);

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);
  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer").WithFetchOption(CacheOnly);
  auto future = catalogClient->GetPartitions(request);
  auto partitionsResponse = future.GetFuture().get();
  ASSERT_FALSE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
}

TEST_P(CatalogClientMockTest, GetPartitionsOnlineOnly) {
  olp::client::HRN hrn(GetTestCatalog());

  {
    testing::InSequence s;
    EXPECT_CALL(*handler_,
                op(IsGetRequest(URL_PARTITIONS), testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(*handler_,
                op(IsGetRequest(URL_PARTITIONS), testing::_, testing::_))
        .Times(1)
        .WillOnce(testing::Invoke(
            returnsResponse({429, "Server busy at the moment."})));
  }

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer").WithFetchOption(OnlineOnly);
  auto future = catalogClient->GetPartitions(request);
  auto partitionsResponse = future.GetFuture().get();

  ASSERT_TRUE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(4u, partitionsResponse.GetResult().GetPartitions().size());

  future = catalogClient->GetPartitions(request);
  partitionsResponse = future.GetFuture().get();
  // Should fail despite valid cache entry
  ASSERT_FALSE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
}

TEST_P(CatalogClientMockTest, GetPartitionsCacheWithUpdate) {
  olp::logging::Log::setLevel(olp::logging::Level::Trace);

  olp::client::HRN hrn(GetTestCatalog());

  auto waitToStartSignal = std::make_shared<std::promise<void>>();
  auto preCallbackWait = std::make_shared<std::promise<void>>();
  preCallbackWait->set_value();
  auto waitForEndSignal = std::make_shared<std::promise<void>>();

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_PARTITIONS), testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(setsPromiseWaitsAndReturns(
          waitToStartSignal, preCallbackWait, {200, HTTP_RESPONSE_PARTITIONS},
          waitForEndSignal)));

  auto catalogClient = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = PartitionsRequest();
  request.WithLayerId("testlayer").WithFetchOption(CacheWithUpdate);
  // Request 1
  auto future = catalogClient->GetPartitions(request);
  PartitionsResponse partitionsResponse = future.GetFuture().get();
  // Request 1 return. Cached value (nothing)
  ASSERT_FALSE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  // Request 2 to check there is a cached value.
  waitForEndSignal->get_future().get();
  request.WithFetchOption(CacheOnly);
  future = catalogClient->GetPartitions(request);
  partitionsResponse = future.GetFuture().get();
  // Cache should be available here.
  ASSERT_TRUE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
}

TEST_P(CatalogClientMockTest, GetCatalog403CacheClear) {
  olp::client::HRN hrn(GetTestCatalog());
  {
    testing::InSequence s;
    EXPECT_CALL(*handler_, op(IsGetRequest(URL_CONFIG), testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(*handler_, op(IsGetRequest(URL_CONFIG), testing::_, testing::_))
        .Times(1)
        .WillOnce(testing::Invoke(returnsResponse({403, HTTP_RESPONSE_403})));
  }

  auto catalogClient = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = CatalogRequest();
  // Populate cache
  auto future = catalogClient->GetCatalog(request);
  CatalogResponse catalogResponse = future.GetFuture().get();
  ASSERT_TRUE(catalogResponse.IsSuccessful());
  auto datarequest = DataRequest();
  datarequest.WithLayerId("testlayer").WithPartitionId("269");
  auto datafuture = catalogClient->GetData(datarequest);
  auto dataresponse = datafuture.GetFuture().get();
  // Receive 403
  request.WithFetchOption(OnlineOnly);
  future = catalogClient->GetCatalog(request);
  catalogResponse = future.GetFuture().get();
  ASSERT_FALSE(catalogResponse.IsSuccessful());
  ASSERT_EQ(403, catalogResponse.GetError().GetHttpStatusCode());
  // Check for cached response
  request.WithFetchOption(CacheOnly);
  future = catalogClient->GetCatalog(request);
  catalogResponse = future.GetFuture().get();
  ASSERT_FALSE(catalogResponse.IsSuccessful());
  // Check the associated data has also been cleared
  datarequest.WithFetchOption(CacheOnly);
  datafuture = catalogClient->GetData(datarequest);
  dataresponse = datafuture.GetFuture().get();
  ASSERT_FALSE(dataresponse.IsSuccessful());
}

TEST_P(CatalogClientMockTest, GetData403CacheClear) {
  olp::client::HRN hrn(GetTestCatalog());
  {
    testing::InSequence s;
    EXPECT_CALL(*handler_,
                op(IsGetRequest(URL_BLOB_DATA_269), testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(*handler_,
                op(IsGetRequest(URL_BLOB_DATA_269), testing::_, testing::_))
        .Times(1)
        .WillOnce(testing::Invoke(returnsResponse({403, HTTP_RESPONSE_403})));
  }

  auto catalogClient = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");
  // Populate cache
  auto future = catalogClient->GetData(request);
  DataResponse dataResponse = future.GetFuture().get();
  ASSERT_TRUE(dataResponse.IsSuccessful());
  // Receive 403
  request.WithFetchOption(OnlineOnly);
  future = catalogClient->GetData(request);
  dataResponse = future.GetFuture().get();
  ASSERT_FALSE(dataResponse.IsSuccessful());
  ASSERT_EQ(403, dataResponse.GetError().GetHttpStatusCode());
  // Check for cached response
  request.WithFetchOption(CacheOnly);
  future = catalogClient->GetData(request);
  dataResponse = future.GetFuture().get();
  ASSERT_FALSE(dataResponse.IsSuccessful());
}

TEST_P(CatalogClientMockTest, GetPartitions403CacheClear) {
  olp::client::HRN hrn(GetTestCatalog());
  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);

  {
    InSequence s;
    EXPECT_CALL(*handler_, op(IsGetRequest(URL_PARTITIONS), _, _)).Times(1);
    EXPECT_CALL(*handler_, op(IsGetRequest(URL_PARTITIONS), _, _))
        .WillOnce(Invoke(returnsResponse({403, HTTP_RESPONSE_403})));
  }

  // Populate cache
  auto request = PartitionsRequest().WithLayerId("testlayer");
  auto future = catalog_client->GetPartitions(request);
  auto partitions_response = future.GetFuture().get();
  ASSERT_TRUE(partitions_response.IsSuccessful());

  // Receive 403
  request.WithFetchOption(OnlineOnly);
  future = catalog_client->GetPartitions(request);
  partitions_response = future.GetFuture().get();
  ASSERT_FALSE(partitions_response.IsSuccessful());
  ASSERT_EQ(403, partitions_response.GetError().GetHttpStatusCode());

  // Check for cached response
  request.WithFetchOption(CacheOnly);
  future = catalog_client->GetPartitions(request);
  partitions_response = future.GetFuture().get();
  ASSERT_FALSE(partitions_response.IsSuccessful());
}

TEST_P(CatalogClientMockTest, CancelPendingRequestsCatalog) {
  olp::client::HRN hrn(GetTestCatalog());
  testing::InSequence s;
  std::vector<std::shared_ptr<std::promise<void>>> waits;
  std::vector<std::shared_ptr<std::promise<void>>> pauses;

  auto catalogClient = std::make_unique<CatalogClient>(hrn, settings_);
  auto catalogRequest = CatalogRequest().WithFetchOption(OnlineOnly);
  auto versionRequest = CatalogVersionRequest().WithFetchOption(OnlineOnly);

  // Make a few requests
  auto waitForCancel1 = std::make_shared<std::promise<void>>();
  auto pauseForCancel1 = std::make_shared<std::promise<void>>();
  auto waitForCancel2 = std::make_shared<std::promise<void>>();
  auto pauseForCancel2 = std::make_shared<std::promise<void>>();

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_CONFIG), testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(
          setsPromiseWaitsAndReturns(waitForCancel1, pauseForCancel1,
                                     {200, HTTP_RESPONSE_LOOKUP_CONFIG})));

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_METADATA), testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(
          setsPromiseWaitsAndReturns(waitForCancel2, pauseForCancel2,
                                     {200, HTTP_RESPONSE_LOOKUP_METADATA})));

  waits.push_back(waitForCancel1);
  pauses.push_back(pauseForCancel1);
  auto catalogFuture = catalogClient->GetCatalog(catalogRequest);

  waits.push_back(waitForCancel2);
  pauses.push_back(pauseForCancel2);
  auto versionFuture = catalogClient->GetCatalogMetadataVersion(versionRequest);

  for (auto wait : waits) {
    wait->get_future().get();
  }
  // Cancel them all
  catalogClient->cancelPendingRequests();
  for (auto pause : pauses) {
    pause->set_value();
  }

  // Verify they are all cancelled
  CatalogResponse catalogResponse = catalogFuture.GetFuture().get();

  ASSERT_FALSE(catalogResponse.IsSuccessful())
      << PrintError(catalogResponse.GetError());

  ASSERT_EQ(olp::network::Network::ErrorCode::Cancelled,
            static_cast<int>(catalogResponse.GetError().GetHttpStatusCode()));
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            catalogResponse.GetError().GetErrorCode());

  CatalogVersionResponse versionResponse = versionFuture.GetFuture().get();

  ASSERT_FALSE(versionResponse.IsSuccessful())
      << PrintError(versionResponse.GetError());

  ASSERT_EQ(olp::network::Network::ErrorCode::Cancelled,
            static_cast<int>(versionResponse.GetError().GetHttpStatusCode()));
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            versionResponse.GetError().GetErrorCode());
}

TEST_P(CatalogClientMockTest, CancelPendingRequestsPartitions) {
  olp::client::HRN hrn(GetTestCatalog());
  testing::InSequence s;
  std::vector<std::shared_ptr<std::promise<void>>> waits;
  std::vector<std::shared_ptr<std::promise<void>>> pauses;

  auto catalogClient = std::make_unique<CatalogClient>(hrn, settings_);
  auto partitionsRequest =
      PartitionsRequest().WithLayerId("testlayer").WithFetchOption(OnlineOnly);
  auto dataRequest = DataRequest()
                         .WithLayerId("testlayer")
                         .WithPartitionId("269")
                         .WithFetchOption(OnlineOnly);

  // Make a few requests
  auto waitForCancel1 = std::make_shared<std::promise<void>>();
  auto pauseForCancel1 = std::make_shared<std::promise<void>>();
  auto waitForCancel2 = std::make_shared<std::promise<void>>();
  auto pauseForCancel2 = std::make_shared<std::promise<void>>();

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LAYER_VERSIONS), testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(
          setsPromiseWaitsAndReturns(waitForCancel1, pauseForCancel1,
                                     {200, HTTP_RESPONSE_LAYER_VERSIONS})));

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_BLOB_DATA_269), testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(
          setsPromiseWaitsAndReturns(waitForCancel2, pauseForCancel2,
                                     {200, HTTP_RESPONSE_BLOB_DATA_269})));

  waits.push_back(waitForCancel1);
  pauses.push_back(pauseForCancel1);
  auto partitionsFuture = catalogClient->GetPartitions(partitionsRequest);

  waits.push_back(waitForCancel2);
  pauses.push_back(pauseForCancel2);
  auto dataFuture = catalogClient->GetData(dataRequest);
  std::cout << "waiting" << std::endl;
  for (auto wait : waits) {
    wait->get_future().get();
  }
  std::cout << "done waitingg" << std::endl;
  // Cancel them all
  catalogClient->cancelPendingRequests();
  std::cout << "done cancelling" << std::endl;
  for (auto pause : pauses) {
    pause->set_value();
  }

  // Verify they are all cancelled
  PartitionsResponse partitionsResponse = partitionsFuture.GetFuture().get();

  ASSERT_FALSE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());

  ASSERT_EQ(
      olp::network::Network::ErrorCode::Cancelled,
      static_cast<int>(partitionsResponse.GetError().GetHttpStatusCode()));
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            partitionsResponse.GetError().GetErrorCode());

  DataResponse dataResponse = dataFuture.GetFuture().get();

  ASSERT_FALSE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());

  ASSERT_EQ(olp::network::Network::ErrorCode::Cancelled,
            static_cast<int>(dataResponse.GetError().GetHttpStatusCode()));
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            dataResponse.GetError().GetErrorCode());
}

TEST_P(CatalogClientMockTest, Prefetch) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  std::vector<olp::geo::TileKey> tileKeys;
  tileKeys.emplace_back(olp::geo::TileKey::FromHereTile("5904591"));

  auto request = olp::dataservice::read::PrefetchTilesRequest()
                     .WithLayerId("hype-test-prefetch")
                     .WithTileKeys(tileKeys)
                     .WithMinLevel(10)
                     .WithMaxLevel(12);

  auto future = catalogClient->PrefetchTiles(request);

  auto response = future.GetFuture().get();
  ASSERT_TRUE(response.IsSuccessful());

  auto& result = response.GetResult();

  for (auto tileResult : result) {
    ASSERT_TRUE(tileResult->IsSuccessful());
    ASSERT_TRUE(tileResult->tile_key_.IsValid());

    dumpTileKey(tileResult->tile_key_);
  }
  ASSERT_EQ(6u, result.size());

  // Second part, use the cache, fetch a partition that's the child of 5904591
  {
    auto request = olp::dataservice::read::DataRequest();
    request.WithLayerId("hype-test-prefetch")
        .WithPartitionId("23618365")
        .WithFetchOption(FetchOptions::CacheOnly);
    auto future = catalogClient->GetData(request);

    auto dataResponse = future.GetFuture().get();

    ASSERT_TRUE(dataResponse.IsSuccessful())
        << PrintError(dataResponse.GetError());
    ASSERT_LT(0, dataResponse.GetResult()->size());
  }
  // The parent of 5904591 should be fetched too
  {
    auto request = olp::dataservice::read::DataRequest();
    request.WithLayerId("hype-test-prefetch")
        .WithPartitionId("1476147")
        .WithFetchOption(FetchOptions::CacheOnly);
    auto future = catalogClient->GetData(request);

    auto dataResponse = future.GetFuture().get();

    ASSERT_TRUE(dataResponse.IsSuccessful())
        << PrintError(dataResponse.GetError());
    ASSERT_LT(0, dataResponse.GetResult()->size());
  }
}

TEST_P(CatalogClientMockTest, PrefetchEmbedded) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  std::vector<olp::geo::TileKey> tileKeys;
  tileKeys.emplace_back(olp::geo::TileKey::FromHereTile("369036"));

  auto request = olp::dataservice::read::PrefetchTilesRequest()
                     .WithLayerId("hype-test-prefetch")
                     .WithTileKeys(tileKeys)
                     .WithMinLevel(9)
                     .WithMaxLevel(9);

  auto future = catalogClient->PrefetchTiles(request);

  auto response = future.GetFuture().get();
  ASSERT_TRUE(response.IsSuccessful());

  auto& result = response.GetResult();

  for (auto tileResult : result) {
    ASSERT_TRUE(tileResult->IsSuccessful());
    ASSERT_TRUE(tileResult->tile_key_.IsValid());

    dumpTileKey(tileResult->tile_key_);
  }
  ASSERT_EQ(1u, result.size());

  // Second part, use the cache to fetch the partition
  {
    auto request = olp::dataservice::read::DataRequest();
    request.WithLayerId("hype-test-prefetch")
        .WithPartitionId("369036")
        .WithFetchOption(FetchOptions::CacheOnly);
    auto future = catalogClient->GetData(request);

    auto dataResponse = future.GetFuture().get();

    ASSERT_TRUE(dataResponse.IsSuccessful())
        << PrintError(dataResponse.GetError());
    ASSERT_LT(0, dataResponse.GetResult()->size());

    // expected data = "data:Embedded Data for 369036"
    std::string dataStrDup(dataResponse.GetResult()->begin(),
                           dataResponse.GetResult()->end());
    ASSERT_EQ("data:Embedded Data for 369036", dataStrDup);
  }
}

TEST_P(CatalogClientMockTest, PrefetchBusy) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  // Prepare the first request
  std::vector<olp::geo::TileKey> tileKeys1;
  tileKeys1.emplace_back(olp::geo::TileKey::FromHereTile("5904591"));

  auto request1 = olp::dataservice::read::PrefetchTilesRequest()
                      .WithLayerId("hype-test-prefetch")
                      .WithTileKeys(tileKeys1)
                      .WithMinLevel(10)
                      .WithMaxLevel(12);

  // Prepare to delay the response of URL_QUADKEYS_5904591 until we've issued
  // the second request
  auto waitForQuadKeyRequest = std::make_shared<std::promise<void>>();
  auto pauseForSecondRequest = std::make_shared<std::promise<void>>();

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_QUADKEYS_5904591), testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(setsPromiseWaitsAndReturns(
          waitForQuadKeyRequest, pauseForSecondRequest,
          {200, HTTP_RESPONSE_QUADKEYS_5904591})));

  // Issue the first request
  auto future1 = catalogClient->PrefetchTiles(request1);

  // Wait for QuadKey request
  waitForQuadKeyRequest->get_future().get();

  // Prepare the second request
  std::vector<olp::geo::TileKey> tileKeys2;
  tileKeys2.emplace_back(olp::geo::TileKey::FromHereTile("369036"));

  auto request2 = olp::dataservice::read::PrefetchTilesRequest()
                      .WithLayerId("hype-test-prefetch")
                      .WithTileKeys(tileKeys2)
                      .WithMinLevel(9)
                      .WithMaxLevel(9);

  // Issue the second request
  auto future2 = catalogClient->PrefetchTiles(request2);

  // Unblock the QuadKey request
  pauseForSecondRequest->set_value();

  // Validate that the second request failed
  auto response2 = future2.GetFuture().get();
  ASSERT_FALSE(response2.IsSuccessful());

  auto& error = response2.GetError();
  ASSERT_EQ(olp::client::ErrorCode::SlowDown, error.GetErrorCode());

  // Get and validate the first request
  auto response1 = future1.GetFuture().get();
  ASSERT_TRUE(response1.IsSuccessful());

  auto& result1 = response1.GetResult();

  for (auto tileResult : result1) {
    ASSERT_TRUE(tileResult->IsSuccessful());
    ASSERT_TRUE(tileResult->tile_key_.IsValid());

    dumpTileKey(tileResult->tile_key_);
  }
  ASSERT_EQ(6u, result1.size());
}

class CatalogClientCacheTest : public CatalogClientMockTest {
  void SetUp() {
    CatalogClientMockTest::SetUp();
    olp::cache::CacheSettings settings;
    switch (std::get<1>(GetParam())) {
      case InMemory: {
        // use the default value
        break;
      }
      case Disk: {
        settings.max_memory_cache_size = 0;
        settings.disk_path =
            olp::utils::Dir::TempDirectory() + k_client_test_cache_dir;
        ClearCache(settings.disk_path.get());
        break;
      }
      case Both: {
        settings.disk_path =
            olp::utils::Dir::TempDirectory() + k_client_test_cache_dir;
        ClearCache(settings.disk_path.get());
        break;
      }
      default:
        // shouldn't get here
        break;
    }

    cache_ = std::make_shared<olp::cache::DefaultCache>(settings);
    ASSERT_EQ(olp::cache::DefaultCache::StorageOpenResult::Success,
              cache_->Open());
  }

  void ClearCache(const std::string& path) { olp::utils::Dir::remove(path); }

  void TearDown() {
    if (cache_) {
      cache_->Close();
    }
    ClearCache(olp::utils::Dir::TempDirectory() + k_client_test_dir);
    handler_.reset();
  }

 protected:
  std::shared_ptr<olp::cache::DefaultCache> cache_;
};

INSTANTIATE_TEST_SUITE_P(TestInMemoryCache, CatalogClientCacheTest,
                         ::testing::Values(std::make_pair(false, InMemory)));

INSTANTIATE_TEST_SUITE_P(TestDiskCache, CatalogClientCacheTest,
                         ::testing::Values(std::make_pair(false, Disk)));

INSTANTIATE_TEST_SUITE_P(TestBothCache, CatalogClientCacheTest,
                         ::testing::Values(std::make_pair(false, Both)));

TEST_P(CatalogClientCacheTest, GetApi) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_METADATA), testing::_, testing::_))
      .Times(1);

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_LATEST_CATALOG_VERSION),
                            testing::_, testing::_))
      .Times(2);

  auto catalogClient = std::make_unique<CatalogClient>(hrn, settings_, cache_);

  auto request = CatalogVersionRequest().WithStartVersion(-1);

  auto future = catalogClient->GetCatalogMetadataVersion(request);
  auto catalogVersionResponse = future.GetFuture().get();

  ASSERT_TRUE(catalogVersionResponse.IsSuccessful())
      << PrintError(catalogVersionResponse.GetError());

  auto partitionsRequest = olp::dataservice::read::PartitionsRequest();
  partitionsRequest.WithLayerId("testlayer");
  auto partitionsFuture = catalogClient->GetPartitions(partitionsRequest);
  auto partitionsResponse = partitionsFuture.GetFuture().get();

  ASSERT_TRUE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(4u, partitionsResponse.GetResult().GetPartitions().size());
}

TEST_P(CatalogClientCacheTest, GetCatalog) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_CONFIG), testing::_, testing::_))
      .Times(1);

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_CONFIG), testing::_, testing::_))
      .Times(1);

  auto catalogClient = std::make_unique<CatalogClient>(hrn, settings_, cache_);
  auto request = CatalogRequest();
  auto future = catalogClient->GetCatalog(request);
  CatalogResponse catalogResponse = future.GetFuture().get();

  ASSERT_TRUE(catalogResponse.IsSuccessful())
      << PrintError(catalogResponse.GetError());

  future = catalogClient->GetCatalog(request);
  CatalogResponse catalogResponse2 = future.GetFuture().get();

  ASSERT_TRUE(catalogResponse2.IsSuccessful())
      << PrintError(catalogResponse2.GetError());
  ASSERT_EQ(catalogResponse2.GetResult().GetName(),
            catalogResponse.GetResult().GetName());
}

TEST_P(CatalogClientCacheTest, GetDataWithPartitionId) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_METADATA), testing::_, testing::_))
      .Times(1);

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_LATEST_CATALOG_VERSION),
                            testing::_, testing::_))
      .Times(2);

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_CONFIG), testing::_, testing::_))
      .Times(1);

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_CONFIG), testing::_, testing::_))
      .Times(1);

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_BLOB), testing::_, testing::_))
      .Times(1);

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_QUERY), testing::_, testing::_))
      .Times(1);

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_QUERY_PARTITION_269), testing::_, testing::_))
      .Times(1);

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_BLOB_DATA_269), testing::_, testing::_))
      .Times(1);

  auto catalogClient = std::make_unique<olp::dataservice::read::CatalogClient>(
      hrn, settings_, cache_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");
  auto future = catalogClient->GetData(request);

  auto dataResponse = future.GetFuture().get();

  ASSERT_TRUE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_LT(0, dataResponse.GetResult()->size());
  std::string dataStr(dataResponse.GetResult()->begin(),
                      dataResponse.GetResult()->end());
  ASSERT_EQ("DT_2_0031", dataStr);

  future = catalogClient->GetData(request);

  dataResponse = future.GetFuture().get();

  ASSERT_TRUE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_LT(0, dataResponse.GetResult()->size());
  std::string dataStrDup(dataResponse.GetResult()->begin(),
                         dataResponse.GetResult()->end());
  ASSERT_EQ("DT_2_0031", dataStrDup);
}

TEST_P(CatalogClientCacheTest, GetPartitionsLayerVersions) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_METADATA), testing::_, testing::_))
      .Times(1);

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_LATEST_CATALOG_VERSION),
                            testing::_, testing::_))
      .Times(2);

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LAYER_VERSIONS), testing::_, testing::_))
      .Times(1);

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_PARTITIONS), testing::_, testing::_))
      .Times(1);

  std::string url_testlayer_res = std::regex_replace(
      URL_PARTITIONS, std::regex("testlayer"), "testlayer_res");
  std::string http_response_testlayer_res = std::regex_replace(
      HTTP_RESPONSE_PARTITIONS, std::regex("testlayer"), "testlayer_res");
  EXPECT_CALL(*handler_,
              op(IsGetRequest(url_testlayer_res), testing::_, testing::_))
      .Times(1)
      .WillOnce(
          testing::Invoke(returnsResponse({200, http_response_testlayer_res})));

  auto catalogClient = std::make_unique<olp::dataservice::read::CatalogClient>(
      hrn, settings_, cache_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer");
  auto future = catalogClient->GetPartitions(request);
  auto partitionsResponse = future.GetFuture().get();

  ASSERT_TRUE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(4u, partitionsResponse.GetResult().GetPartitions().size());
  request.WithLayerId("testlayer_res");

  future = catalogClient->GetPartitions(request);

  partitionsResponse = future.GetFuture().get();

  ASSERT_TRUE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(4u, partitionsResponse.GetResult().GetPartitions().size());
}

TEST_P(CatalogClientCacheTest, GetPartitions) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_METADATA), testing::_, testing::_))
      .Times(1);

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_LATEST_CATALOG_VERSION),
                            testing::_, testing::_))
      .Times(2);

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LAYER_VERSIONS), testing::_, testing::_))
      .Times(1);

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_PARTITIONS), testing::_, testing::_))
      .Times(1);

  auto catalogClient = std::make_unique<olp::dataservice::read::CatalogClient>(
      hrn, settings_, cache_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer");
  auto future = catalogClient->GetPartitions(request);
  auto partitionsResponse = future.GetFuture().get();

  ASSERT_TRUE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(4u, partitionsResponse.GetResult().GetPartitions().size());

  future = catalogClient->GetPartitions(request);
  partitionsResponse = future.GetFuture().get();

  ASSERT_TRUE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(4u, partitionsResponse.GetResult().GetPartitions().size());
}

TEST_P(CatalogClientCacheTest, GetDataWithPartitionIdDifferentVersions) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_METADATA), testing::_, testing::_))
      .Times(1);

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_LATEST_CATALOG_VERSION),
                            testing::_, testing::_))
      .Times(2);

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_CONFIG), testing::_, testing::_))
      .Times(1);

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_CONFIG), testing::_, testing::_))
      .Times(1);

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_BLOB), testing::_, testing::_))
      .Times(1);

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_LOOKUP_QUERY), testing::_, testing::_))
      .Times(1);

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_QUERY_PARTITION_269), testing::_, testing::_))
      .Times(1);

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_BLOB_DATA_269), testing::_, testing::_))
      .Times(1);

  EXPECT_CALL(*handler_, op(IsGetRequest(URL_QUERY_PARTITION_269_V2),
                            testing::_, testing::_))
      .Times(1);

  EXPECT_CALL(*handler_,
              op(IsGetRequest(URL_BLOB_DATA_269_V2), testing::_, testing::_))
      .Times(1);

  auto catalogClient =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  {
    request.WithLayerId("testlayer").WithPartitionId("269");
    auto dataResponse =
        GetExecutionTime<olp::dataservice::read::DataResponse>([&] {
          auto future = catalogClient->GetData(request);
          return future.GetFuture().get();
        });

    ASSERT_TRUE(dataResponse.IsSuccessful())
        << PrintError(dataResponse.GetError());
    ASSERT_LT(0, dataResponse.GetResult()->size());
    std::string dataStr(dataResponse.GetResult()->begin(),
                        dataResponse.GetResult()->end());
    ASSERT_EQ("DT_2_0031", dataStr);
  }

  {
    request.WithVersion(2);
    auto dataResponse =
        GetExecutionTime<olp::dataservice::read::DataResponse>([&] {
          auto future = catalogClient->GetData(request);
          return future.GetFuture().get();
        });

    ASSERT_TRUE(dataResponse.IsSuccessful())
        << PrintError(dataResponse.GetError());
    ASSERT_LT(0, dataResponse.GetResult()->size());
    std::string dataStr(dataResponse.GetResult()->begin(),
                        dataResponse.GetResult()->end());
    ASSERT_EQ("DT_2_0031_V2", dataStr);
  }

  {
    request.WithVersion(boost::none);
    auto dataResponse =
        GetExecutionTime<olp::dataservice::read::DataResponse>([&] {
          auto future = catalogClient->GetData(request);
          return future.GetFuture().get();
        });

    ASSERT_TRUE(dataResponse.IsSuccessful())
        << PrintError(dataResponse.GetError());
    ASSERT_LT(0, dataResponse.GetResult()->size());
    std::string dataStr(dataResponse.GetResult()->begin(),
                        dataResponse.GetResult()->end());
    ASSERT_EQ("DT_2_0031", dataStr);
  }

  {
    request.WithVersion(2);
    auto dataResponse =
        GetExecutionTime<olp::dataservice::read::DataResponse>([&] {
          auto future = catalogClient->GetData(request);
          return future.GetFuture().get();
        });

    ASSERT_TRUE(dataResponse.IsSuccessful())
        << PrintError(dataResponse.GetError());
    ASSERT_LT(0, dataResponse.GetResult()->size());
    std::string dataStr(dataResponse.GetResult()->begin(),
                        dataResponse.GetResult()->end());
    ASSERT_EQ("DT_2_0031_V2", dataStr);
  }
}

TEST_P(CatalogClientCacheTest, GetVolatilePartitionsExpiry) {
  olp::client::HRN hrn(GetTestCatalog());

  {
    testing::InSequence s;
    EXPECT_CALL(
        *handler_,
        op(IsGetRequest(
               "https://metadata.data.api.platform.here.com/metadata/v1/"
               "catalogs/"
               "hereos-internal-test-v2/layers/testlayer_volatile/partitions"),
           testing::_, testing::_))
        .Times(1)
        .WillRepeatedly(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_PARTITIONS_V2})));
    EXPECT_CALL(
        *handler_,
        op(IsGetRequest(
               "https://metadata.data.api.platform.here.com/metadata/v1/"
               "catalogs/"
               "hereos-internal-test-v2/layers/testlayer_volatile/partitions"),
           testing::_, testing::_))
        .Times(1)
        .WillRepeatedly(testing::Invoke(
            returnsResponse({200, HTTP_RESPONSE_EMPTY_PARTITIONS})));
  }

  auto catalogClient = std::make_unique<olp::dataservice::read::CatalogClient>(
      hrn, settings_, cache_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer_volatile");

  auto future = catalogClient->GetPartitions(request);

  auto partitionsResponse = future.GetFuture().get();

  ASSERT_TRUE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(1u, partitionsResponse.GetResult().GetPartitions().size());

  // hit the cache only, should be still be there
  request.WithFetchOption(olp::dataservice::read::CacheOnly);
  future = catalogClient->GetPartitions(request);
  partitionsResponse = future.GetFuture().get();
  ASSERT_TRUE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(1u, partitionsResponse.GetResult().GetPartitions().size());

  // wait for the layer to expire in cache
  std::this_thread::sleep_for(std::chrono::seconds(2));
  request.WithFetchOption(olp::dataservice::read::OnlineIfNotFound);
  future = catalogClient->GetPartitions(request);
  partitionsResponse = future.GetFuture().get();

  ASSERT_TRUE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(0u, partitionsResponse.GetResult().GetPartitions().size());
}
