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
#include <olp/dataservice/read/CatalogClient.h>
#include "VolatileLayerClientImpl.h"

#define URL_LOOKUP_VOLATILE_BLOB \
  R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data:::hereos-internal-test-v2/apis/volatile-blob/v1)"

#define URL_VOLATILE_BLOB_DATA \
  R"(https://volatile-blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/testlayer/data/4eed6ed1-0d32-43b9-ae79-043cb4256432)"

#define URL_LOOKUP_QUERY \
  R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data:::hereos-internal-test-v2/apis/query/v1)"

#define URL_QUERY_PARTITION_269 \
  R"(https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2/layers/testlayer/partitions?partition=269)"

#define HTTP_RESPONSE_LOOKUP_VOLATILE_BLOB \
  R"jsonString([{"api":"volatile-blob","version":"v1","baseURL":"https://volatile-blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString"

#define HTTP_RESPONSE_LOOKUP_QUERY \
  R"jsonString([{"api":"query","version":"v1","baseURL":"https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString"

#define HTTP_RESPONSE_PARTITION_269 \
  R"jsonString({ "partitions": [{"version":4,"partition":"269","layer":"testlayer","dataHandle":"4eed6ed1-0d32-43b9-ae79-043cb4256432"}]})jsonString"

#define HTTP_RESPONSE_NO_PARTITION \
  R"jsonString({ "partitions": []})jsonString"

#define BLOB_DATA_HANDLE R"(4eed6ed1-0d32-43b9-ae79-043cb4256432)"

namespace {
using namespace olp::dataservice::read;
using namespace ::testing;
const std::string kCatalog = "hrn:here:data:::hereos-internal-test-v2";
const std::string kLayerId = "testlayer";
const auto kHRN = olp::client::HRN::FromString(kCatalog);

class VolatileLayerClientImplTest : public ::testing::Test {
 protected:
  void SetUp() override;
  void TearDown() override;

  std::unique_ptr<olp::client::OlpClientSettings> settings_;
  std::shared_ptr<NetworkMock> network_mock_;
};

void VolatileLayerClientImplTest::SetUp() {
  network_mock_ = std::make_shared<NetworkMock>();
  settings_.reset(new olp::client::OlpClientSettings());
  settings_->cache = olp::dataservice::read::CreateDefaultCache();
  settings_->network_request_handler = network_mock_;
}

void VolatileLayerClientImplTest::TearDown() {
  settings_.reset();
  network_mock_.reset();
}

TEST_F(VolatileLayerClientImplTest, GetDataWithDataHandle) {
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_VOLATILE_BLOB), _, _, _, _))
      .WillOnce(NetworkMock::ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(
              olp::http::HttpStatusCode::OK),
          HTTP_RESPONSE_LOOKUP_VOLATILE_BLOB));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_VOLATILE_BLOB_DATA), _, _, _, _))
      .WillOnce(NetworkMock::ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(
              olp::http::HttpStatusCode::OK),
          "SomeData"));

  VolatileLayerClientImpl client(kHRN, kLayerId, *settings_);

  DataRequest request;
  request.WithDataHandle(BLOB_DATA_HANDLE);

  using DataResponse =
      olp::client::ApiResponse<VolatileLayerClientImpl::DataResult,
                               olp::client::ApiError>;
  std::promise<DataResponse> promise;
  std::future<DataResponse> future(promise.get_future());

  auto token = client.GetData(
      request, [&](DataResponse response) { promise.set_value(response); });

  const auto& response = future.get();
  ASSERT_TRUE(response.IsSuccessful());
}

TEST_F(VolatileLayerClientImplTest, GetDataWithPartitionId) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_QUERY), _, _, _, _))
      .WillOnce(NetworkMock::ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(
              olp::http::HttpStatusCode::OK),
          HTTP_RESPONSE_LOOKUP_QUERY));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_QUERY_PARTITION_269), _, _, _, _))
      .WillOnce(NetworkMock::ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(
              olp::http::HttpStatusCode::OK),
          HTTP_RESPONSE_PARTITION_269));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_VOLATILE_BLOB), _, _, _, _))
      .WillOnce(NetworkMock::ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(
              olp::http::HttpStatusCode::OK),
          HTTP_RESPONSE_LOOKUP_VOLATILE_BLOB));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_VOLATILE_BLOB_DATA), _, _, _, _))
      .WillOnce(NetworkMock::ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(
              olp::http::HttpStatusCode::OK),
          "SomeData"));

  VolatileLayerClientImpl client(kHRN, kLayerId, *settings_);

  DataRequest request;
  request.WithPartitionId("269");

  using DataResponse =
      olp::client::ApiResponse<VolatileLayerClientImpl::DataResult,
                               olp::client::ApiError>;
  std::promise<DataResponse> promise;
  std::future<DataResponse> future = promise.get_future();

  auto token = client.GetData(
      request, [&](DataResponse response) { promise.set_value(response); });

  const auto& response = future.get();
  ASSERT_TRUE(response.IsSuccessful());
}

TEST_F(VolatileLayerClientImplTest, GetDataNoPartition) {
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_QUERY), _, _, _, _))
      .WillOnce(NetworkMock::ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(
              olp::http::HttpStatusCode::OK),
          HTTP_RESPONSE_LOOKUP_QUERY));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_QUERY_PARTITION_269), _, _, _, _))
      .WillOnce(NetworkMock::ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(
              olp::http::HttpStatusCode::OK),
          HTTP_RESPONSE_NO_PARTITION));

  VolatileLayerClientImpl client(kHRN, kLayerId, *settings_);

  DataRequest request;
  request.WithPartitionId("269");

  using DataResponse =
      olp::client::ApiResponse<VolatileLayerClientImpl::DataResult,
                               olp::client::ApiError>;
  std::promise<DataResponse> promise;
  std::future<DataResponse> future = promise.get_future();

  auto token = client.GetData(
      request, [&](DataResponse response) { promise.set_value(response); });

  const auto& response = future.get();
  ASSERT_FALSE(response.IsSuccessful());
}

}  // namespace


