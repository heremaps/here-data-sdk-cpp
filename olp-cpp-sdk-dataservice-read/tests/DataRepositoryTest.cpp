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
#include <mocks/NetworkMock.h>
#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/client/OlpClientFactory.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/TileRequest.h>
#include <repositories/DataRepository.h>

#define URL_LOOKUP_BLOB \
  R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data::olp-here-test:hereos-internal-test-v2/apis/blob/v1)"

#define URL_BLOB_DATA_269 \
  R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/testlayer/data/4eed6ed1-0d32-43b9-ae79-043cb4256432)"

#define URL_BLOB_DATA_5904591 \
  R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/testlayer/data/e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1)"

#define URL_BLOB_DATA_23618364 \
  R"(https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/testlayer/data/f9a9fd8e-eb1b-48e5-bfdb-4392b3826443)"

#define HTTP_RESPONSE_LOOKUP_BLOB \
  R"jsonString([{"api":"blob","version":"v1","baseURL":"https://blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString"

#define HTTP_RESPONSE_403 \
  R"jsonString("Forbidden - A catalog with the specified HRN doesn't exist or access to this catalog is forbidden)jsonString"

#define BLOB_DATA_HANDLE R"(4eed6ed1-0d32-43b9-ae79-043cb4256432)"

#define BLOB_DATA_HANDLE_23618364 R"(f9a9fd8e-eb1b-48e5-bfdb-4392b3826443)"

#define BLOB_DATA_HANDLE_5904591 R"(e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1)"

#define URL_LOOKUP_QUERY \
  R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data::olp-here-test:hereos-internal-test-v2/apis/query/v1)"

#define HTTP_RESPONSE_LOOKUP_QUERY \
  R"jsonString([{"api":"query","version":"v1","baseURL":"https://sab.query.data.api.platform.here.com/query/v1/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2","parameters":{}}])jsonString"

#define QUERY_TREE_INDEX \
  R"(https://sab.query.data.api.platform.here.com/query/v1/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/testlayer/versions/4/quadkeys/5904591/depths/4)"

#define SUB_QUADS \
  R"jsonString({"subQuads": [{"subQuadKey":"4","version":4,"dataHandle":"f9a9fd8e-eb1b-48e5-bfdb-4392b3826443"},{"subQuadKey":"5","version":4,"dataHandle":"e119d20e-c7c6-4563-ae88-8aa5c6ca75c3"},{"subQuadKey":"6","version":4,"dataHandle":"a7a1afdf-db7e-4833-9627-d38bee6e2f81"},{"subQuadKey":"7","version":4,"dataHandle":"9d515348-afce-44e8-bc6f-3693cfbed104"},{"subQuadKey":"1","version":4,"dataHandle":"e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1"}],"parentQuads": [{"partition":"1476147","version":4,"dataHandle":"95c5c703-e00e-4c38-841e-e419367474f1"}]})jsonString"
namespace {

using testing::_;
using namespace olp::tests::common;

const std::string kLayerId = "testlayer";
const std::string kService = "blob";

class DataRepositoryTest : public ::testing::Test {
 protected:
  void SetUp() override;
  void TearDown() override;

  std::string GetTestCatalog();

  std::shared_ptr<olp::client::OlpClientSettings> settings_;
  std::shared_ptr<NetworkMock> network_mock_;
};

void DataRepositoryTest::SetUp() {
  network_mock_ = std::make_shared<NetworkMock>();
  settings_ = std::make_shared<olp::client::OlpClientSettings>();
  settings_->cache =
      olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
  settings_->network_request_handler = network_mock_;
}

void DataRepositoryTest::TearDown() {
  settings_.reset();
  network_mock_.reset();
}

std::string DataRepositoryTest::GetTestCatalog() {
  return "hrn:here:data::olp-here-test:hereos-internal-test-v2";
}

TEST_F(DataRepositoryTest, GetBlobData) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP_BLOB));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   "someData"));

  olp::client::CancellationContext context;

  olp::dataservice::read::DataRequest request;
  request.WithDataHandle(BLOB_DATA_HANDLE);

  olp::client::HRN hrn(GetTestCatalog());

  auto response =
      olp::dataservice::read::repository::DataRepository::GetBlobData(
          hrn, kLayerId, kService, request, context, *settings_);

  ASSERT_TRUE(response.IsSuccessful());
}

TEST_F(DataRepositoryTest, GetBlobDataApiLookupFailed403) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::FORBIDDEN),
                                   HTTP_RESPONSE_403));

  olp::client::CancellationContext context;

  olp::dataservice::read::DataRequest request;
  request.WithDataHandle(BLOB_DATA_HANDLE);

  olp::client::HRN hrn(GetTestCatalog());

  auto response =
      olp::dataservice::read::repository::DataRepository::GetBlobData(
          hrn, kLayerId, kService, request, context, *settings_);

  ASSERT_FALSE(response.IsSuccessful());
}

TEST_F(DataRepositoryTest, GetBlobDataNoDataHandle) {
  olp::client::CancellationContext context;
  olp::dataservice::read::DataRequest request;
  olp::client::HRN hrn(GetTestCatalog());
  auto response =
      olp::dataservice::read::repository::DataRepository::GetBlobData(
          hrn, kLayerId, kService, request, context, *settings_);
  ASSERT_FALSE(response.IsSuccessful());
}

TEST_F(DataRepositoryTest, GetBlobDataFailedDataFetch403) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP_BLOB));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::FORBIDDEN),
                                   HTTP_RESPONSE_403));

  olp::client::CancellationContext context;

  olp::dataservice::read::DataRequest request;
  request.WithDataHandle(BLOB_DATA_HANDLE);

  olp::client::HRN hrn(GetTestCatalog());

  auto response =
      olp::dataservice::read::repository::DataRepository::GetBlobData(
          hrn, kLayerId, kService, request, context, *settings_);

  ASSERT_FALSE(response.IsSuccessful());
}

TEST_F(DataRepositoryTest, GetBlobDataCache) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP_BLOB));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   "someData"));

  olp::client::CancellationContext context;

  olp::dataservice::read::DataRequest request;
  request.WithDataHandle(BLOB_DATA_HANDLE);

  olp::client::HRN hrn(GetTestCatalog());

  // This should download data from network and cache it
  auto response =
      olp::dataservice::read::repository::DataRepository::GetBlobData(
          hrn, kLayerId, kService, request, context, *settings_);

  ASSERT_TRUE(response.IsSuccessful());

  // This call should not do any network calls and use already cached values
  // instead
  response = olp::dataservice::read::repository::DataRepository::GetBlobData(
      hrn, kLayerId, kService, request, context, *settings_);

  ASSERT_TRUE(response.IsSuccessful());
}

TEST_F(DataRepositoryTest, GetBlobDataImmediateCancel) {
  ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_LOOKUP_BLOB));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        "someData"));

  olp::client::CancellationContext context;

  olp::dataservice::read::DataRequest request;
  request.WithDataHandle(BLOB_DATA_HANDLE);

  olp::client::HRN hrn(GetTestCatalog());

  context.CancelOperation();
  ASSERT_TRUE(context.IsCancelled());

  auto response =
      olp::dataservice::read::repository::DataRepository::GetBlobData(
          hrn, kLayerId, kService, request, context, *settings_);
  ASSERT_EQ(response.GetError().GetErrorCode(),
            olp::client::ErrorCode::Cancelled);
}

TEST_F(DataRepositoryTest, GetBlobDataInProgressCancel) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP_BLOB));

  olp::client::CancellationContext context;

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .WillOnce(
          [&](olp::http::NetworkRequest, olp::http::Network::Payload,
              olp::http::Network::Callback, olp::http::Network::HeaderCallback,
              olp::http::Network::DataCallback) -> olp::http::SendOutcome {
            std::thread([&]() { context.CancelOperation(); }).detach();
            constexpr auto unused_request_id = 12;
            return olp::http::SendOutcome(unused_request_id);
          });
  EXPECT_CALL(*network_mock_, Cancel(_)).WillOnce(testing::Return());

  olp::dataservice::read::DataRequest request;
  request.WithDataHandle(BLOB_DATA_HANDLE);

  olp::client::HRN hrn(GetTestCatalog());

  auto response =
      olp::dataservice::read::repository::DataRepository::GetBlobData(
          hrn, kLayerId, kService, request, context, *settings_);
  ASSERT_EQ(response.GetError().GetErrorCode(),
            olp::client::ErrorCode::Cancelled);
}

TEST_F(DataRepositoryTest, CheckCashedPartitions) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_QUERY), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP_QUERY));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(QUERY_TREE_INDEX), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   SUB_QUADS));

  olp::client::HRN hrn(GetTestCatalog());
  int64_t version = 4;

  // query partitions and store to cache
  {
    auto request = olp::dataservice::read::TileRequest().WithTileKey(
        olp::geo::TileKey::FromHereTile("5904591"));
    olp::client::CancellationContext context;
    std::string requested_tile_data_handle;
    auto response = olp::dataservice::read::repository::DataRepository::
        QueryPartitionsAndGetDataHandle(hrn, kLayerId, request, version,
                                        context, *settings_,
                                        requested_tile_data_handle);

    ASSERT_TRUE(response.GetHttpStatusCode() !=
                static_cast<int>(olp::http::ErrorCode::SUCCESS));
    ASSERT_EQ(requested_tile_data_handle,"e83b397a-2be5-45a8-b7fb-ad4cb3ea13b1");
  }

  //check if all partitions stored in cache
  {
    auto request = olp::dataservice::read::TileRequest().WithTileKey(
        olp::geo::TileKey::FromHereTile("23618364"));
    auto partitions = olp::dataservice::read::repository::DataRepository::
        GetPartitionsFromCache(hrn, kLayerId, request, version, *settings_);

    //check if partitions was stored to cache
    ASSERT_FALSE(partitions.GetPartitions().size() == 0);
    ASSERT_EQ(partitions.GetPartitions().front().GetDataHandle(),
              BLOB_DATA_HANDLE_23618364);
  }
}

TEST_F(DataRepositoryTest, GetVersionedDataTileQuadTree) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_QUERY), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP_QUERY));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(QUERY_TREE_INDEX), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   SUB_QUADS));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP_BLOB));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_BLOB_DATA_5904591), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   "someData"));

  olp::client::HRN hrn(GetTestCatalog());
  int64_t version = 4;

  // request data for tile
  {
    auto request = olp::dataservice::read::TileRequest().WithTileKey(
        olp::geo::TileKey::FromHereTile("5904591"));
    olp::client::CancellationContext context;
    auto response = olp::dataservice::read::repository::DataRepository::
        GetVersionedDataTileQuadTree(hrn, kLayerId, request, version, context,
                                     *settings_);
    ASSERT_TRUE(response.IsSuccessful());
  }

  // second request for another tile key, data handle should be found in cache,
  // no need to query online
  {
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_23618364), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     "someData"));

    auto request = olp::dataservice::read::TileRequest().WithTileKey(
        olp::geo::TileKey::FromHereTile("23618364"));
    olp::client::CancellationContext context;
    auto response = olp::dataservice::read::repository::DataRepository::
        GetVersionedDataTileQuadTree(hrn, kLayerId, request, version, context,
                                     *settings_);
    ASSERT_TRUE(response.IsSuccessful());
  }
}

TEST_F(DataRepositoryTest, GetVersionedDataTileQuadTreeOnlineOnly) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_QUERY), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP_QUERY));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(QUERY_TREE_INDEX), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   SUB_QUADS));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP_BLOB));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_BLOB_DATA_5904591), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   "someData"));

  olp::client::HRN hrn(GetTestCatalog());
  int64_t version = 4;

  // request for tile key, but use OnlineOnly option,
  {
    auto request =
        olp::dataservice::read::TileRequest()
            .WithTileKey(olp::geo::TileKey::FromHereTile("5904591"))
            .WithFetchOption(olp::dataservice::read::FetchOptions::OnlineOnly);
    olp::client::CancellationContext context;
    auto response = olp::dataservice::read::repository::DataRepository::
        GetVersionedDataTileQuadTree(hrn, kLayerId, request, version, context,
                                     *settings_);
    ASSERT_TRUE(response.IsSuccessful());
  }
}

TEST_F(DataRepositoryTest, GetVersionedDataTileQuadTreeImmediateCancel) {
  olp::client::HRN hrn(GetTestCatalog());
  int64_t version = 4;

  auto request = olp::dataservice::read::TileRequest().WithTileKey(
      olp::geo::TileKey::FromHereTile("5904591"));
  olp::client::CancellationContext context;

  context.CancelOperation();
  ASSERT_TRUE(context.IsCancelled());

  auto response = olp::dataservice::read::repository::DataRepository::
      GetVersionedDataTileQuadTree(hrn, kLayerId, request, version, context,
                                   *settings_);

  ASSERT_EQ(response.GetError().GetErrorCode(),
            olp::client::ErrorCode::Cancelled);
}

TEST_F(DataRepositoryTest, GetVersionedDataTileQuadTreeInProgressCancel) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_QUERY), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP_QUERY));

  olp::client::CancellationContext context;

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(QUERY_TREE_INDEX), _, _, _, _))
      .WillOnce(
          [&](olp::http::NetworkRequest, olp::http::Network::Payload,
              olp::http::Network::Callback, olp::http::Network::HeaderCallback,
              olp::http::Network::DataCallback) -> olp::http::SendOutcome {
            std::thread([&]() { context.CancelOperation(); }).detach();
            constexpr auto unused_request_id = 12;
            return olp::http::SendOutcome(unused_request_id);
          });

  EXPECT_CALL(*network_mock_, Cancel(_)).WillOnce(testing::Return());

  olp::client::HRN hrn(GetTestCatalog());
  int64_t version = 4;

  auto request = olp::dataservice::read::TileRequest().WithTileKey(
      olp::geo::TileKey::FromHereTile("5904591"));

  auto response = olp::dataservice::read::repository::DataRepository::
      GetVersionedDataTileQuadTree(hrn, kLayerId, request, version, context,
                                   *settings_);

  ASSERT_EQ(response.GetError().GetErrorCode(),
            olp::client::ErrorCode::Cancelled);
}

TEST_F(DataRepositoryTest, GetVersionedDataTileQuadTreeReturnEmpty) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_QUERY), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_LOOKUP_QUERY));

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(QUERY_TREE_INDEX), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   "no_data_handle_in_responce"));

  olp::client::HRN hrn(GetTestCatalog());
  int64_t version = 4;
  olp::client::CancellationContext context;

  auto request = olp::dataservice::read::TileRequest().WithTileKey(
      olp::geo::TileKey::FromHereTile("5904591"));

  auto response = olp::dataservice::read::repository::DataRepository::
      GetVersionedDataTileQuadTree(hrn, kLayerId, request, version, context,
                                   *settings_);

  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_EQ(response.GetError().GetErrorCode(),
            olp::client::ErrorCode::NotFound);
}

}  // namespace
