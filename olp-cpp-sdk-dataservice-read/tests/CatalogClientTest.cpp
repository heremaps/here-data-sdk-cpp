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

#include <gmock/gmock.h>
#include <matchers/NetworkUrlMatchers.h>
#include <mocks/NetworkMock.h>
#include <olp/core/client/HRN.h>
#include <olp/core/http/Network.h>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include <olp/dataservice/read/CatalogClient.h>
#include <olp/dataservice/read/CatalogRequest.h>
#include <olp/dataservice/read/CatalogVersionRequest.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/PartitionsRequest.h>
#include <olp/dataservice/read/PrefetchTilesRequest.h>
#include <olp/dataservice/read/model/Catalog.h>
#include <testutils/CustomParameters.hpp>
#include "CatalogClientTestBase.h"
#include "HttpResponses.h"

namespace {

using namespace olp::dataservice::read;
using namespace testing;

void DumpTileKey(const olp::geo::TileKey& tile_key) {
  std::cout << "Tile: " << tile_key.ToHereTile()
            << ", level: " << tile_key.Level()
            << ", parent: " << tile_key.Parent().ToHereTile() << std::endl;
}

class CatalogClientTest : public CatalogClientTestBase {};

TEST_P(CatalogClientTest, GetCatalog) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1);
  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = CatalogRequest();
  auto future = catalog_client->GetCatalog(request);
  CatalogResponse catalog_response = future.GetFuture().get();

  ASSERT_TRUE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());
}

TEST_P(CatalogClientTest, GetCatalogCallback) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1);

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);

  auto request = CatalogRequest();

  std::promise<CatalogResponse> promise;
  CatalogResponseCallback callback = [&promise](CatalogResponse response) {
    promise.set_value(response);
  };
  catalog_client->GetCatalog(request, callback);
  CatalogResponse catalog_response = promise.get_future().get();
  ASSERT_TRUE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());
}

TEST_P(CatalogClientTest, GetCatalog403) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .WillOnce(NetworkMock::ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(403), HTTP_RESPONSE_403));

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = CatalogRequest();
  auto future = catalog_client->GetCatalog(request);
  CatalogResponse catalog_response = future.GetFuture().get();

  ASSERT_FALSE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());
  ASSERT_EQ(403, catalog_response.GetError().GetHttpStatusCode());
}

TEST_P(CatalogClientTest, GetPartitions) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1);

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer");
  auto future = catalog_client->GetPartitions(request);
  auto partitions_response = future.GetFuture().get();

  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(4u, partitions_response.GetResult().GetPartitions().size());
}

TEST_P(CatalogClientTest, GetDataWithPartitionId) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .Times(1);

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");
  auto future = catalog_client->GetData(request);

  auto data_response = future.GetFuture().get();

  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_LT(0, data_response.GetResult()->size());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ("DT_2_0031", data_string);
}

TEST_P(CatalogClientTest, GetDataWithInlineField) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITION_3), _, _, _, _))
      .Times(1);

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("3");
  auto future = catalog_client->GetData(request);

  auto data_response = future.GetFuture().get();

  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_LT(0, data_response.GetResult()->size());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ(0u, data_string.find("data:"));
}

TEST_P(CatalogClientTest, GetEmptyPartitions) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .WillOnce(NetworkMock::ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(200),
          HTTP_RESPONSE_EMPTY_PARTITIONS));

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer");
  auto future = catalog_client->GetPartitions(request);
  auto partitions_response = future.GetFuture().get();

  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(0u, partitions_response.GetResult().GetPartitions().size());
}

TEST_P(CatalogClientTest, GetVolatileDataHandle) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(
      *network_mock_,
      Send(IsGetRequest(
               "https://volatile-blob-ireland.data.api.platform.here.com/"
               "blobstore/v1/catalogs/hereos-internal-test-v2/layers/"
               "testlayer_volatile/data/volatileHandle"),
           _, _, _, _))
      .WillOnce(NetworkMock::ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(200), "someData"));

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer_volatile").WithDataHandle("volatileHandle");

  auto future = catalog_client->GetData(request);

  auto data_response = future.GetFuture().get();

  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_LT(0, data_response.GetResult()->size());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ("someData", data_string);
}

TEST_P(CatalogClientTest, GetVolatilePartitions) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest("https://metadata.data.api.platform.here.com/"
                                "metadata/v1/catalogs/hereos-internal-test-v2/"
                                "layers/testlayer_volatile/partitions"),
                   _, _, _, _))
      .WillOnce(NetworkMock::ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(200),
          HTTP_RESPONSE_PARTITIONS_V2));

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer_volatile");

  auto future = catalog_client->GetPartitions(request);

  auto partitions_response = future.GetFuture().get();

  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(1u, partitions_response.GetResult().GetPartitions().size());

  request.WithVersion(18);
  future = catalog_client->GetPartitions(request);
  partitions_response = future.GetFuture().get();

  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(1u, partitions_response.GetResult().GetPartitions().size());
}

TEST_P(CatalogClientTest, GetVolatileDataByPartitionId) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);

  EXPECT_CALL(
      *network_mock_,
      Send(IsGetRequest("https://query.data.api.platform.here.com/query/v1/"
                        "catalogs/hereos-internal-test-v2/layers/"
                        "testlayer_volatile/partitions?partition=269"),
           _, _, _, _))
      .WillOnce(NetworkMock::ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(200),
          HTTP_RESPONSE_PARTITIONS_V2));

  EXPECT_CALL(
      *network_mock_,
      Send(IsGetRequest(
               "https://volatile-blob-ireland.data.api.platform.here.com/"
               "blobstore/v1/catalogs/hereos-internal-test-v2/layers/"
               "testlayer_volatile/data/4eed6ed1-0d32-43b9-ae79-043cb4256410"),
           _, _, _, _))
      .WillOnce(NetworkMock::ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(200), "someData"));

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer_volatile").WithPartitionId("269");

  auto future = catalog_client->GetData(request);

  auto data_response = future.GetFuture().get();

  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_LT(0, data_response.GetResult()->size());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ("someData", data_string);
}

TEST_P(CatalogClientTest, GetStreamDataHandle) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer_stream").WithDataHandle("streamHandle");

  auto future = catalog_client->GetData(request);

  auto data_response = future.GetFuture().get();

  ASSERT_FALSE(data_response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::ServiceUnavailable,
            data_response.GetError().GetErrorCode());
}

TEST_P(CatalogClientTest, GetData429Error) {
  olp::client::HRN hrn(GetTestCatalog());

  {
    testing::InSequence s;

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .Times(2)
        .WillRepeatedly(NetworkMock::ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(429),
            "Server busy at the moment."));

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .Times(1);
  }

  olp::client::RetrySettings retry_settings;
  retry_settings.retry_condition =
      [](const olp::client::HttpResponse& response) {
        return 429 == response.status;
      };
  settings_->retry_settings = retry_settings;
  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer")
      .WithDataHandle("4eed6ed1-0d32-43b9-ae79-043cb4256432");

  auto future = catalog_client->GetData(request);

  auto data_response = future.GetFuture().get();

  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_LT(0, data_response.GetResult()->size());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ("DT_2_0031", data_string);
}

TEST_P(CatalogClientTest, GetPartitions429Error) {
  olp::client::HRN hrn(GetTestCatalog());

  {
    testing::InSequence s;

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
        .Times(2)
        .WillRepeatedly(NetworkMock::ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(429),
            "Server busy at the moment."));

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
        .Times(1);
  }

  olp::client::RetrySettings retry_settings;
  retry_settings.retry_condition =
      [](const olp::client::HttpResponse& response) {
        return 429 == response.status;
      };
  settings_->retry_settings = retry_settings;
  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer");
  auto future = catalog_client->GetPartitions(request);
  auto partitions_response = future.GetFuture().get();

  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(4u, partitions_response.GetResult().GetPartitions().size());
}

TEST_P(CatalogClientTest, ApiLookup429) {
  olp::client::HRN hrn(GetTestCatalog());

  {
    testing::InSequence s;

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
        .Times(2)
        .WillRepeatedly(NetworkMock::ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(429),
            "Server busy at the moment."));

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
        .Times(1);
  }

  olp::client::RetrySettings retry_settings;
  retry_settings.retry_condition =
      [](const olp::client::HttpResponse& response) {
        return 429 == response.status;
      };
  settings_->retry_settings = retry_settings;
  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer");
  auto future = catalog_client->GetPartitions(request);
  auto partitions_response = future.GetFuture().get();

  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(4u, partitions_response.GetResult().GetPartitions().size());
}

TEST_P(CatalogClientTest, GetPartitionsForInvalidLayer) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("invalidLayer");
  auto future = catalog_client->GetPartitions(request);
  auto partitions_response = future.GetFuture().get();

  ASSERT_FALSE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(olp::client::ErrorCode::InvalidArgument,
            partitions_response.GetError().GetErrorCode());
}

TEST_P(CatalogClientTest, GetData404Error) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(
      *network_mock_,
      Send(IsGetRequest("https://blob-ireland.data.api.platform.here.com/"
                        "blobstore/v1/catalogs/hereos-internal-test-v2/"
                        "layers/testlayer/data/invalidDataHandle"),
           _, _, _, _))
      .WillOnce(NetworkMock::ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(404), "Resource not found."));

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithDataHandle("invalidDataHandle");
  auto future = catalog_client->GetData(request);

  auto data_response = future.GetFuture().get();

  ASSERT_FALSE(data_response.IsSuccessful());
  ASSERT_EQ(404, data_response.GetError().GetHttpStatusCode());
}

TEST_P(CatalogClientTest, GetPartitionsGarbageResponse) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
      .WillOnce(NetworkMock::ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(200),
          R"jsonString(kd3sdf\)jsonString"));

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer");
  auto future = catalog_client->GetPartitions(request);
  auto partitions_response = future.GetFuture().get();

  ASSERT_FALSE(partitions_response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::ServiceUnavailable,
            partitions_response.GetError().GetErrorCode());
}

TEST_P(CatalogClientTest, GetCatalogCancelApiLookup) {
  olp::client::HRN hrn(GetTestCatalog());

  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_LOOKUP_CONFIG});

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(0);

  // Run it!
  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);

  auto request = CatalogRequest();

  std::promise<CatalogResponse> promise;
  CatalogResponseCallback callback = [&promise](CatalogResponse response) {
    promise.set_value(response);
  };
  olp::client::CancellationToken cancel_token =
      catalog_client->GetCatalog(request, callback);

  wait_for_cancel->get_future().get();
  cancel_token.cancel();
  pause_for_cancel->set_value();
  CatalogResponse catalog_response = promise.get_future().get();

  ASSERT_FALSE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());

  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            catalog_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            catalog_response.GetError().GetErrorCode());
}

TEST_P(CatalogClientTest, GetCatalogCancelConfig) {
  olp::client::HRN hrn(GetTestCatalog());

  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_CONFIG});

  // Setup the expected calls :
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  // Run it!
  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);

  auto request = CatalogRequest();

  std::promise<CatalogResponse> promise;
  CatalogResponseCallback callback = [&promise](CatalogResponse response) {
    promise.set_value(response);
  };
  olp::client::CancellationToken cancel_token =
      catalog_client->GetCatalog(request, callback);

  wait_for_cancel->get_future().get();
  std::cout << "Cancelling" << std::endl;
  cancel_token.cancel();  // crashing?
  std::cout << "Cancelled, unblocking response" << std::endl;
  pause_for_cancel->set_value();
  std::cout << "Post Cancel, get response" << std::endl;
  CatalogResponse catalog_response = promise.get_future().get();

  ASSERT_FALSE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());

  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            catalog_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            catalog_response.GetError().GetErrorCode());
  std::cout << "Post Test" << std::endl;
}

TEST_P(CatalogClientTest, GetCatalogCancelAfterCompletion) {
  olp::client::HRN hrn(GetTestCatalog());

  // Run it!
  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);

  auto request = CatalogRequest();

  std::promise<CatalogResponse> promise;
  CatalogResponseCallback callback = [&promise](CatalogResponse response) {
    promise.set_value(response);
  };
  olp::client::CancellationToken cancel_token =
      catalog_client->GetCatalog(request, callback);

  CatalogResponse catalog_response = promise.get_future().get();

  ASSERT_TRUE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());

  cancel_token.cancel();
}

TEST_P(CatalogClientTest, GetPartitionsCancelLookupMetadata) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_LOOKUP_METADATA});

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request =
      olp::dataservice::read::PartitionsRequest().WithLayerId("testlayer");

  std::promise<PartitionsResponse> promise;
  PartitionsResponseCallback callback =
      [&promise](PartitionsResponse response) { promise.set_value(response); };

  olp::client::CancellationToken cancel_token =
      catalog_client->GetPartitions(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.cancel();
  pause_for_cancel->set_value();  // unblock the handler
  PartitionsResponse partitions_response = promise.get_future().get();

  ASSERT_FALSE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            partitions_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            partitions_response.GetError().GetErrorCode());
}

TEST_P(CatalogClientTest, GetPartitionsCancelLatestCatalogVersion) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) =
      GenerateNetworkMockActions(wait_for_cancel, pause_for_cancel,
                                 {200, HTTP_RESPONSE_LATEST_CATALOG_VERSION});

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LAYER_VERSIONS), _, _, _, _))
      .Times(0);

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request =
      olp::dataservice::read::PartitionsRequest().WithLayerId("testlayer");

  std::promise<PartitionsResponse> promise;
  PartitionsResponseCallback callback =
      [&promise](PartitionsResponse response) { promise.set_value(response); };

  olp::client::CancellationToken cancel_token =
      catalog_client->GetPartitions(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.cancel();
  pause_for_cancel->set_value();  // unblock the handler
  PartitionsResponse partitions_response = promise.get_future().get();

  ASSERT_FALSE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            partitions_response.GetError().GetHttpStatusCode())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            partitions_response.GetError().GetErrorCode())
      << ApiErrorToString(partitions_response.GetError());
}

TEST_P(CatalogClientTest, DISABLED_GetPartitionsCancelLayerVersions) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_LAYER_VERSIONS});

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LAYER_VERSIONS), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .Times(0);

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request =
      olp::dataservice::read::PartitionsRequest().WithLayerId("testlayer");

  std::promise<PartitionsResponse> promise;
  PartitionsResponseCallback callback =
      [&promise](PartitionsResponse response) { promise.set_value(response); };

  olp::client::CancellationToken cancel_token =
      catalog_client->GetPartitions(request, callback);
  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.cancel();
  pause_for_cancel->set_value();  // unblock the handler

  PartitionsResponse partitions_response = promise.get_future().get();

  ASSERT_FALSE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            partitions_response.GetError().GetHttpStatusCode())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            partitions_response.GetError().GetErrorCode())
      << ApiErrorToString(partitions_response.GetError());
}

TEST_P(CatalogClientTest, GetDataWithPartitionIdCancelLookupConfig) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_LOOKUP_CONFIG});

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(0);

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancel_token =
      catalog_client->GetData(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.cancel();
  pause_for_cancel->set_value();  // unblock the handler

  DataResponse data_response = promise.get_future().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode())
      << ApiErrorToString(data_response.GetError());
}

TEST_P(CatalogClientTest, GetDataWithPartitionIdCancelConfig) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_CONFIG});

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
      .Times(0);

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancel_token =
      catalog_client->GetData(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.cancel();
  pause_for_cancel->set_value();  // unblock the handler

  DataResponse data_response = promise.get_future().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode())
      << ApiErrorToString(data_response.GetError());
}

TEST_P(CatalogClientTest, DISABLED_GetDataWithPartitionIdCancelLookupMetadata) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_LOOKUP_METADATA});

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancel_token =
      catalog_client->GetData(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.cancel();
  pause_for_cancel->set_value();  // unblock the handler

  DataResponse data_response = promise.get_future().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode())
      << ApiErrorToString(data_response.GetError());
}

TEST_P(CatalogClientTest,
       DISABLED_GetDataWithPartitionIdCancelLatestCatalogVersion) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) =
      GenerateNetworkMockActions(wait_for_cancel, pause_for_cancel,
                                 {200, HTTP_RESPONSE_LATEST_CATALOG_VERSION});

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_QUERY), _, _, _, _))
      .Times(0);

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancel_token =
      catalog_client->GetData(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.cancel();
  pause_for_cancel->set_value();  // unblock the handler

  DataResponse data_response = promise.get_future().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode())
      << ApiErrorToString(data_response.GetError());
}

TEST_P(CatalogClientTest, GetDataWithPartitionIdCancelInnerConfig) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  {
    testing::InSequence s;
    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
        .Times(1);

    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
        wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_CONFIG});

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
        .Times(1)
        .WillOnce(testing::Invoke(std::move(send_mock)));

    EXPECT_CALL(*network_mock_, Cancel(request_id))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));
  }

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);

  olp::cache::CacheSettings cache_settings;
  cache_settings.max_memory_cache_size = 0;
  auto catalog_client = std::make_unique<olp::dataservice::read::CatalogClient>(
      hrn, settings_,
      olp::dataservice::read::CreateDefaultCache(cache_settings));

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancel_token =
      catalog_client->GetData(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.cancel();
  pause_for_cancel->set_value();  // unblock the handler

  DataResponse data_response = promise.get_future().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode())
      << ApiErrorToString(data_response.GetError());
}

TEST_P(CatalogClientTest, GetDataWithPartitionIdCancelLookupQuery) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_LOOKUP_QUERY});

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_QUERY), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_QUERY_PARTITION_269), _, _, _, _))
      .Times(0);

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancel_token =
      catalog_client->GetData(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.cancel();
  pause_for_cancel->set_value();  // unblock the handler

  DataResponse data_response = promise.get_future().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode())
      << ApiErrorToString(data_response.GetError());
}

TEST_P(CatalogClientTest, GetDataWithPartitionIdCancelQuery) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_PARTITION_269});

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_QUERY_PARTITION_269), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
      .Times(0);

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancel_token =
      catalog_client->GetData(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.cancel();
  pause_for_cancel->set_value();  // unblock the handler

  DataResponse data_response = promise.get_future().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode())
      << ApiErrorToString(data_response.GetError());
}

TEST_P(CatalogClientTest, GetDataWithPartitionIdCancelLookupBlob) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_LOOKUP_BLOB});

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .Times(0);

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancel_token =
      catalog_client->GetData(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.cancel();
  pause_for_cancel->set_value();  // unblock the handler

  DataResponse data_response = promise.get_future().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode())
      << ApiErrorToString(data_response.GetError());
}

TEST_P(CatalogClientTest, GetDataWithPartitionIdCancelBlob) {
  olp::client::HRN hrn(GetTestCatalog());

  // Setup the expected calls :
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_BLOB_DATA_269});

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");

  std::promise<DataResponse> promise;
  DataResponseCallback callback = [&promise](DataResponse response) {
    promise.set_value(response);
  };

  olp::client::CancellationToken cancel_token =
      catalog_client->GetData(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.cancel();
  pause_for_cancel->set_value();  // unblock the handler

  DataResponse data_response = promise.get_future().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode())
      << ApiErrorToString(data_response.GetError());
}

TEST_P(CatalogClientTest, GetCatalogVersion) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(1);

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);

  auto request = CatalogVersionRequest().WithStartVersion(-1);

  auto future = catalog_client->GetCatalogMetadataVersion(request);
  auto catalog_version_response = future.GetFuture().get();

  ASSERT_TRUE(catalog_version_response.IsSuccessful())
      << ApiErrorToString(catalog_version_response.GetError());
}

TEST_P(CatalogClientTest, GetDataWithPartitionIdVersion2) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LAYER_VERSIONS_V2), _, _, _, _))
      .Times(0);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269").WithVersion(2);
  auto data_response = catalog_client->GetData(request).GetFuture().get();

  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_LT(0, data_response.GetResult()->size());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ("DT_2_0031_V2", data_string);
}

TEST_P(CatalogClientTest, GetDataWithPartitionIdInvalidVersion) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269").WithVersion(10);
  auto data_response = catalog_client->GetData(request).GetFuture().get();

  ASSERT_FALSE(data_response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::BadRequest,
            data_response.GetError().GetErrorCode());
  ASSERT_EQ(400, data_response.GetError().GetHttpStatusCode());

  request.WithVersion(-1);
  data_response = catalog_client->GetData(request).GetFuture().get();

  ASSERT_FALSE(data_response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::BadRequest,
            data_response.GetError().GetErrorCode());
  ASSERT_EQ(400, data_response.GetError().GetHttpStatusCode());
}

TEST_P(CatalogClientTest, GetPartitionsVersion2) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LAYER_VERSIONS_V2), _, _, _, _))
      .Times(1);

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer").WithVersion(2);
  auto partitions_response =
      catalog_client->GetPartitions(request).GetFuture().get();

  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(1u, partitions_response.GetResult().GetPartitions().size());
}

TEST_P(CatalogClientTest, GetPartitionsInvalidVersion) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer").WithVersion(10);
  auto partitions_response =
      catalog_client->GetPartitions(request).GetFuture().get();

  ASSERT_FALSE(partitions_response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::BadRequest,
            partitions_response.GetError().GetErrorCode());
  ASSERT_EQ(400, partitions_response.GetError().GetHttpStatusCode());

  request.WithVersion(-1);
  partitions_response =
      catalog_client->GetPartitions(request).GetFuture().get();

  ASSERT_FALSE(partitions_response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::BadRequest,
            partitions_response.GetError().GetErrorCode());
  ASSERT_EQ(400, partitions_response.GetError().GetHttpStatusCode());
}

TEST_P(CatalogClientTest, GetCatalogVersionCancel) {
  olp::client::HRN hrn(GetTestCatalog());

  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  // Setup the expected calls :
  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_LOOKUP_METADATA});

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);

  // Run it!
  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);

  auto request = CatalogVersionRequest().WithStartVersion(-1);

  std::promise<CatalogVersionResponse> promise;
  CatalogVersionCallback callback =
      [&promise](CatalogVersionResponse response) {
        promise.set_value(response);
      };
  olp::client::CancellationToken cancel_token =
      catalog_client->GetCatalogMetadataVersion(request, callback);

  wait_for_cancel->get_future().get();
  cancel_token.cancel();
  pause_for_cancel->set_value();
  CatalogVersionResponse version_response = promise.get_future().get();

  ASSERT_FALSE(version_response.IsSuccessful())
      << ApiErrorToString(version_response.GetError());

  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            version_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            version_response.GetError().GetErrorCode());
}

TEST_P(CatalogClientTest, GetCatalogCacheOnly) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(0);

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = CatalogRequest();
  request.WithFetchOption(CacheOnly);
  auto future = catalog_client->GetCatalog(request);
  CatalogResponse catalog_response = future.GetFuture().get();
  ASSERT_FALSE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());
}

TEST_P(CatalogClientTest, GetCatalogOnlineOnly) {
  olp::client::HRN hrn(GetTestCatalog());

  {
    testing::InSequence s;

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
        .WillOnce(NetworkMock::ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(429),
            "Server busy at the moment."));
  }

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = CatalogRequest();
  request.WithFetchOption(OnlineOnly);
  auto future = catalog_client->GetCatalog(request);
  CatalogResponse catalog_response = future.GetFuture().get();
  ASSERT_TRUE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());
  future = catalog_client->GetCatalog(request);
  // Should fail despite valid cache entry.
  catalog_response = future.GetFuture().get();
  ASSERT_FALSE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());
}

TEST_P(CatalogClientTest, GetCatalogCacheWithUpdate) {
  olp::logging::Log::setLevel(olp::logging::Level::Trace);

  olp::client::HRN hrn(GetTestCatalog());
  auto wait_to_start_signal = std::make_shared<std::promise<void>>();
  auto pre_callback_wait = std::make_shared<std::promise<void>>();
  pre_callback_wait->set_value();
  auto wait_for_end = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) =
      GenerateNetworkMockActions(wait_to_start_signal, pre_callback_wait,
                                 {200, HTTP_RESPONSE_CONFIG}, wait_for_end);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = CatalogRequest();
  request.WithFetchOption(CacheWithUpdate);
  // Request 1
  SCOPED_TRACE("Request Catalog, CacheWithUpdate");
  auto future = catalog_client->GetCatalog(request);

  SCOPED_TRACE("get CatalogResponse1");
  CatalogResponse catalog_response = future.GetFuture().get();

  // Request 1 return. Cached value (nothing)
  ASSERT_FALSE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());
  // Wait for background cache update to finish
  SCOPED_TRACE("wait some time for update to conclude");
  wait_for_end->get_future().get();

  // Request 2 to check there is a cached value.
  SCOPED_TRACE("Request Catalog, CacheOnly");
  request.WithFetchOption(CacheOnly);
  future = catalog_client->GetCatalog(request);
  SCOPED_TRACE("get CatalogResponse2");
  catalog_response = future.GetFuture().get();
  // Cache should be available here.
  ASSERT_TRUE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());
}

TEST_P(CatalogClientTest, GetDataCacheOnly) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .Times(0);
  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer")
      .WithPartitionId("269")
      .WithFetchOption(CacheOnly);
  auto future = catalog_client->GetData(request);
  auto data_response = future.GetFuture().get();
  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
}

TEST_P(CatalogClientTest, GetDataOnlineOnly) {
  olp::client::HRN hrn(GetTestCatalog());

  {
    testing::InSequence s;

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .WillOnce(NetworkMock::ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(429),
            "Server busy at the moment."));
  }

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer")
      .WithPartitionId("269")
      .WithFetchOption(OnlineOnly);
  auto future = catalog_client->GetData(request);

  auto data_response = future.GetFuture().get();

  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_LT(0, data_response.GetResult()->size());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ("DT_2_0031", data_string);
  // Should fail despite cached response
  future = catalog_client->GetData(request);
  data_response = future.GetFuture().get();
  ASSERT_FALSE(data_response.IsSuccessful());
}

TEST_P(CatalogClientTest, GetDataCacheWithUpdate) {
  olp::logging::Log::setLevel(olp::logging::Level::Trace);

  olp::client::HRN hrn(GetTestCatalog());
  // Setup the expected calls :
  auto wait_to_start_signal = std::make_shared<std::promise<void>>();
  auto pre_callback_wait = std::make_shared<std::promise<void>>();
  pre_callback_wait->set_value();
  auto wait_for_end_signal = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_to_start_signal, pre_callback_wait,
      {200, HTTP_RESPONSE_BLOB_DATA_269}, wait_for_end_signal);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = DataRequest();
  request.WithLayerId("testlayer")
      .WithPartitionId("269")
      .WithFetchOption(CacheWithUpdate);
  // Request 1
  auto future = catalog_client->GetData(request);
  DataResponse data_response = future.GetFuture().get();
  // Request 1 return. Cached value (nothing)
  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  // Request 2 to check there is a cached value.
  // waiting for cache to fill-in
  wait_for_end_signal->get_future().get();
  request.WithFetchOption(CacheOnly);
  future = catalog_client->GetData(request);
  data_response = future.GetFuture().get();
  // Cache should be available here.
  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
}

TEST_P(CatalogClientTest, GetPartitionsCacheOnly) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .Times(0);

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);
  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer").WithFetchOption(CacheOnly);
  auto future = catalog_client->GetPartitions(request);
  auto partitions_response = future.GetFuture().get();
  ASSERT_FALSE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
}

TEST_P(CatalogClientTest, GetPartitionsOnlineOnly) {
  olp::client::HRN hrn(GetTestCatalog());

  {
    testing::InSequence s;

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
        .WillOnce(NetworkMock::ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(429),
            "Server busy at the moment."));
  }

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer").WithFetchOption(OnlineOnly);
  auto future = catalog_client->GetPartitions(request);
  auto partitions_response = future.GetFuture().get();

  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(4u, partitions_response.GetResult().GetPartitions().size());

  future = catalog_client->GetPartitions(request);
  partitions_response = future.GetFuture().get();
  // Should fail despite valid cache entry
  ASSERT_FALSE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
}

TEST_P(CatalogClientTest, DISABLED_GetPartitionsCacheWithUpdate) {
  olp::logging::Log::setLevel(olp::logging::Level::Trace);

  olp::client::HRN hrn(GetTestCatalog());

  auto wait_to_start_signal = std::make_shared<std::promise<void>>();
  auto pre_callback_wait = std::make_shared<std::promise<void>>();
  pre_callback_wait->set_value();
  auto wait_for_end_signal = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_to_start_signal, pre_callback_wait, {200, HTTP_RESPONSE_PARTITIONS},
      wait_for_end_signal);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = PartitionsRequest();
  request.WithLayerId("testlayer").WithFetchOption(CacheWithUpdate);
  // Request 1
  auto future = catalog_client->GetPartitions(request);
  PartitionsResponse partitions_response = future.GetFuture().get();
  // Request 1 return. Cached value (nothing)
  ASSERT_FALSE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  // Request 2 to check there is a cached value.
  wait_for_end_signal->get_future().get();
  request.WithFetchOption(CacheOnly);
  future = catalog_client->GetPartitions(request);
  partitions_response = future.GetFuture().get();
  // Cache should be available here.
  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
}

TEST_P(CatalogClientTest, GetCatalog403CacheClear) {
  olp::client::HRN hrn(GetTestCatalog());
  {
    testing::InSequence s;

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
        .WillOnce(NetworkMock::ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(403), HTTP_RESPONSE_403));
  }

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = CatalogRequest();
  // Populate cache
  auto future = catalog_client->GetCatalog(request);
  CatalogResponse catalog_response = future.GetFuture().get();
  ASSERT_TRUE(catalog_response.IsSuccessful());
  auto data_request = DataRequest();
  data_request.WithLayerId("testlayer").WithPartitionId("269");
  auto data_future = catalog_client->GetData(data_request);
  auto data_response = data_future.GetFuture().get();
  // Receive 403
  request.WithFetchOption(OnlineOnly);
  future = catalog_client->GetCatalog(request);
  catalog_response = future.GetFuture().get();
  ASSERT_FALSE(catalog_response.IsSuccessful());
  ASSERT_EQ(403, catalog_response.GetError().GetHttpStatusCode());
  // Check for cached response
  request.WithFetchOption(CacheOnly);
  future = catalog_client->GetCatalog(request);
  catalog_response = future.GetFuture().get();
  ASSERT_FALSE(catalog_response.IsSuccessful());
  // Check the associated data has also been cleared
  data_request.WithFetchOption(CacheOnly);
  data_future = catalog_client->GetData(data_request);
  data_response = data_future.GetFuture().get();
  ASSERT_FALSE(data_response.IsSuccessful());
}

TEST_P(CatalogClientTest, GetData403CacheClear) {
  olp::client::HRN hrn(GetTestCatalog());
  {
    testing::InSequence s;
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .WillOnce(NetworkMock::ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(403), HTTP_RESPONSE_403));
  }

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");
  // Populate cache
  auto future = catalog_client->GetData(request);
  DataResponse data_response = future.GetFuture().get();
  ASSERT_TRUE(data_response.IsSuccessful());
  // Receive 403
  request.WithFetchOption(OnlineOnly);
  future = catalog_client->GetData(request);
  data_response = future.GetFuture().get();
  ASSERT_FALSE(data_response.IsSuccessful());
  ASSERT_EQ(403, data_response.GetError().GetHttpStatusCode());
  // Check for cached response
  request.WithFetchOption(CacheOnly);
  future = catalog_client->GetData(request);
  data_response = future.GetFuture().get();
  ASSERT_FALSE(data_response.IsSuccessful());
}

TEST_P(CatalogClientTest, GetPartitions403CacheClear) {
  olp::client::HRN hrn(GetTestCatalog());
  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);

  {
    testing::InSequence s;
    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
        .WillOnce(NetworkMock::ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(403), HTTP_RESPONSE_403));
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

TEST_P(CatalogClientTest, DISABLED_CancelPendingRequestsCatalog) {
  olp::client::HRN hrn(GetTestCatalog());
  testing::InSequence s;
  std::vector<std::shared_ptr<std::promise<void>>> waits;
  std::vector<std::shared_ptr<std::promise<void>>> pauses;

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);
  auto catalog_request = CatalogRequest().WithFetchOption(OnlineOnly);
  auto version_request = CatalogVersionRequest().WithFetchOption(OnlineOnly);

  // Make a few requests
  auto wait_for_cancel1 = std::make_shared<std::promise<void>>();
  auto pause_for_cancel1 = std::make_shared<std::promise<void>>();
  auto wait_for_cancel2 = std::make_shared<std::promise<void>>();
  auto pause_for_cancel2 = std::make_shared<std::promise<void>>();

  {
    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    std::tie(request_id, send_mock, cancel_mock) =
        GenerateNetworkMockActions(wait_for_cancel1, pause_for_cancel1,
                                   {200, HTTP_RESPONSE_LOOKUP_CONFIG});

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
        .Times(1)
        .WillOnce(testing::Invoke(std::move(send_mock)));

    EXPECT_CALL(*network_mock_, Cancel(request_id))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));
  }

  {
    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    std::tie(request_id, send_mock, cancel_mock) =
        GenerateNetworkMockActions(wait_for_cancel2, pause_for_cancel2,
                                   {200, HTTP_RESPONSE_LOOKUP_METADATA});

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
        .Times(1)
        .WillOnce(testing::Invoke(std::move(send_mock)));

    EXPECT_CALL(*network_mock_, Cancel(request_id))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));
  }

  waits.push_back(wait_for_cancel1);
  pauses.push_back(pause_for_cancel1);
  auto catalog_future = catalog_client->GetCatalog(catalog_request);

  waits.push_back(wait_for_cancel2);
  pauses.push_back(pause_for_cancel2);
  auto version_future =
      catalog_client->GetCatalogMetadataVersion(version_request);

  for (auto wait : waits) {
    wait->get_future().get();
  }
  // Cancel them all
  catalog_client->cancelPendingRequests();
  for (auto pause : pauses) {
    pause->set_value();
  }

  // Verify they are all cancelled
  CatalogResponse catalog_response = catalog_future.GetFuture().get();

  ASSERT_FALSE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());

  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            catalog_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            catalog_response.GetError().GetErrorCode());

  CatalogVersionResponse version_response = version_future.GetFuture().get();

  ASSERT_FALSE(version_response.IsSuccessful())
      << ApiErrorToString(version_response.GetError());

  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            version_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            version_response.GetError().GetErrorCode());
}

TEST_P(CatalogClientTest, DISABLED_CancelPendingRequestsPartitions) {
  olp::client::HRN hrn(GetTestCatalog());
  testing::InSequence s;
  std::vector<std::shared_ptr<std::promise<void>>> waits;
  std::vector<std::shared_ptr<std::promise<void>>> pauses;

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);
  auto partitions_request =
      PartitionsRequest().WithLayerId("testlayer").WithFetchOption(OnlineOnly);
  auto data_request = DataRequest()
                          .WithLayerId("testlayer")
                          .WithPartitionId("269")
                          .WithFetchOption(OnlineOnly);

  // Make a few requests
  auto wait_for_cancel1 = std::make_shared<std::promise<void>>();
  auto pause_for_cancel1 = std::make_shared<std::promise<void>>();
  auto wait_for_cancel2 = std::make_shared<std::promise<void>>();
  auto pause_for_cancel2 = std::make_shared<std::promise<void>>();

  {
    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    std::tie(request_id, send_mock, cancel_mock) =
        GenerateNetworkMockActions(wait_for_cancel1, pause_for_cancel1,
                                   {200, HTTP_RESPONSE_LAYER_VERSIONS});

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LAYER_VERSIONS), _, _, _, _))
        .Times(1)
        .WillOnce(testing::Invoke(std::move(send_mock)));

    EXPECT_CALL(*network_mock_, Cancel(request_id))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));
  }
  {
    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    std::tie(request_id, send_mock, cancel_mock) =
        GenerateNetworkMockActions(wait_for_cancel2, pause_for_cancel2,
                                   {200, HTTP_RESPONSE_BLOB_DATA_269});

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .Times(1)
        .WillOnce(testing::Invoke(std::move(send_mock)));

    EXPECT_CALL(*network_mock_, Cancel(request_id))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));
  }

  waits.push_back(wait_for_cancel1);
  pauses.push_back(pause_for_cancel1);
  auto partitions_future = catalog_client->GetPartitions(partitions_request);

  waits.push_back(wait_for_cancel2);
  pauses.push_back(pause_for_cancel2);
  auto data_future = catalog_client->GetData(data_request);
  std::cout << "waiting" << std::endl;
  for (auto wait : waits) {
    wait->get_future().get();
  }
  std::cout << "done waitingg" << std::endl;
  // Cancel them all
  catalog_client->cancelPendingRequests();
  std::cout << "done cancelling" << std::endl;
  for (auto pause : pauses) {
    pause->set_value();
  }

  // Verify they are all cancelled
  PartitionsResponse partitions_response = partitions_future.GetFuture().get();

  ASSERT_FALSE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());

  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            partitions_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            partitions_response.GetError().GetErrorCode());

  DataResponse data_response = data_future.GetFuture().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());

  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode());
}

TEST_P(CatalogClientTest, Prefetch) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  std::vector<olp::geo::TileKey> tile_keys;
  tile_keys.emplace_back(olp::geo::TileKey::FromHereTile("5904591"));

  auto request = olp::dataservice::read::PrefetchTilesRequest()
                     .WithLayerId("hype-test-prefetch")
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(10)
                     .WithMaxLevel(12);

  auto future = catalog_client->PrefetchTiles(request);

  auto response = future.GetFuture().get();
  ASSERT_TRUE(response.IsSuccessful());

  auto& result = response.GetResult();

  for (auto tile_result : result) {
    ASSERT_TRUE(tile_result->IsSuccessful());
    ASSERT_TRUE(tile_result->tile_key_.IsValid());

    DumpTileKey(tile_result->tile_key_);
  }
  ASSERT_EQ(6u, result.size());

  // Second part, use the cache, fetch a partition that's the child of 5904591
  {
    auto request = olp::dataservice::read::DataRequest();
    request.WithLayerId("hype-test-prefetch")
        .WithPartitionId("23618365")
        .WithFetchOption(FetchOptions::CacheOnly);
    auto future = catalog_client->GetData(request);

    auto data_response = future.GetFuture().get();

    ASSERT_TRUE(data_response.IsSuccessful())
        << ApiErrorToString(data_response.GetError());
    ASSERT_LT(0, data_response.GetResult()->size());
  }
  // The parent of 5904591 should be fetched too
  {
    auto request = olp::dataservice::read::DataRequest();
    request.WithLayerId("hype-test-prefetch")
        .WithPartitionId("1476147")
        .WithFetchOption(FetchOptions::CacheOnly);
    auto future = catalog_client->GetData(request);

    auto data_response = future.GetFuture().get();

    ASSERT_TRUE(data_response.IsSuccessful())
        << ApiErrorToString(data_response.GetError());
    ASSERT_LT(0, data_response.GetResult()->size());
  }
}

TEST_P(CatalogClientTest, PrefetchEmbedded) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  std::vector<olp::geo::TileKey> tile_keys;
  tile_keys.emplace_back(olp::geo::TileKey::FromHereTile("369036"));

  auto request = olp::dataservice::read::PrefetchTilesRequest()
                     .WithLayerId("hype-test-prefetch")
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(9)
                     .WithMaxLevel(9);

  auto future = catalog_client->PrefetchTiles(request);

  auto response = future.GetFuture().get();
  ASSERT_TRUE(response.IsSuccessful());

  auto& result = response.GetResult();

  for (auto tile_result : result) {
    ASSERT_TRUE(tile_result->IsSuccessful());
    ASSERT_TRUE(tile_result->tile_key_.IsValid());

    DumpTileKey(tile_result->tile_key_);
  }
  ASSERT_EQ(1u, result.size());

  // Second part, use the cache to fetch the partition
  {
    auto request = olp::dataservice::read::DataRequest();
    request.WithLayerId("hype-test-prefetch")
        .WithPartitionId("369036")
        .WithFetchOption(FetchOptions::CacheOnly);
    auto future = catalog_client->GetData(request);

    auto data_response = future.GetFuture().get();

    ASSERT_TRUE(data_response.IsSuccessful())
        << ApiErrorToString(data_response.GetError());
    ASSERT_LT(0, data_response.GetResult()->size());

    // expected data = "data:Embedded Data for 369036"
    std::string data_string_duplicate(data_response.GetResult()->begin(),
                                      data_response.GetResult()->end());
    ASSERT_EQ("data:Embedded Data for 369036", data_string_duplicate);
  }
}

TEST_P(CatalogClientTest, DISABLED_PrefetchBusy) {
  olp::client::HRN hrn(GetTestCatalog());

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  // Prepare the first request
  std::vector<olp::geo::TileKey> tile_keys1;
  tile_keys1.emplace_back(olp::geo::TileKey::FromHereTile("5904591"));

  auto request1 = olp::dataservice::read::PrefetchTilesRequest()
                      .WithLayerId("hype-test-prefetch")
                      .WithTileKeys(tile_keys1)
                      .WithMinLevel(10)
                      .WithMaxLevel(12);

  // Prepare to delay the response of URL_QUADKEYS_5904591 until we've issued
  // the second request
  auto wait_for_quad_key_request = std::make_shared<std::promise<void>>();
  auto pause_for_second_request = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_quad_key_request, pause_for_second_request,
      {200, HTTP_RESPONSE_QUADKEYS_5904591});

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_QUADKEYS_5904591), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  // Issue the first request
  auto future1 = catalog_client->PrefetchTiles(request1);

  // Wait for QuadKey request
  wait_for_quad_key_request->get_future().get();

  // Prepare the second request
  std::vector<olp::geo::TileKey> tile_keys2;
  tile_keys2.emplace_back(olp::geo::TileKey::FromHereTile("369036"));

  auto request2 = olp::dataservice::read::PrefetchTilesRequest()
                      .WithLayerId("hype-test-prefetch")
                      .WithTileKeys(tile_keys2)
                      .WithMinLevel(9)
                      .WithMaxLevel(9);

  // Issue the second request
  auto future2 = catalog_client->PrefetchTiles(request2);

  // Unblock the QuadKey request
  pause_for_second_request->set_value();

  // Validate that the second request failed
  auto response2 = future2.GetFuture().get();
  ASSERT_FALSE(response2.IsSuccessful());

  auto& error = response2.GetError();
  ASSERT_EQ(olp::client::ErrorCode::SlowDown, error.GetErrorCode());

  // Get and validate the first request
  auto response1 = future1.GetFuture().get();
  ASSERT_TRUE(response1.IsSuccessful());

  auto& result1 = response1.GetResult();

  for (auto tile_result : result1) {
    ASSERT_TRUE(tile_result->IsSuccessful());
    ASSERT_TRUE(tile_result->tile_key_.IsValid());

    DumpTileKey(tile_result->tile_key_);
  }
  ASSERT_EQ(6u, result1.size());
}

INSTANTIATE_TEST_SUITE_P(, CatalogClientTest,
                         ::testing::Values(CacheType::BOTH));

}  // namespace
