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
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/dataservice/read/CatalogClient.h>
#include "VolatileLayerClientImpl.h"

namespace {
using namespace olp::dataservice::read;
using namespace ::testing;
using namespace olp::tests::common;
using DataResponse = VolatileLayerClientImpl::DataResponse;

constexpr auto kUrlLookupVolatileBlob =
    R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data:::hereos-internal-test-v2/apis/volatile-blob/v1)";

constexpr auto kUrlVolatileBlobData =
    R"(https://volatile-blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2/layers/testlayer/data/4eed6ed1-0d32-43b9-ae79-043cb4256432)";

constexpr auto kUrlLookupQuery =
    R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data:::hereos-internal-test-v2/apis/query/v1)";

constexpr auto kUrlQueryPartition269 =
    R"(https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2/layers/testlayer/partitions?partition=269)";

constexpr auto kHttpResponseLookupVolatileBlob =
    R"jsonString([{"api":"volatile-blob","version":"v1","baseURL":"https://volatile-blob-ireland.data.api.platform.here.com/blobstore/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString";

constexpr auto kHttpResponseLookupQuery =
    R"jsonString([{"api":"query","version":"v1","baseURL":"https://query.data.api.platform.here.com/query/v1/catalogs/hereos-internal-test-v2","parameters":{}}])jsonString";

constexpr auto kHttpResponsePartition269 =
    R"jsonString({ "partitions": [{"version":4,"partition":"269","layer":"testlayer","dataHandle":"4eed6ed1-0d32-43b9-ae79-043cb4256432"}]})jsonString";

constexpr auto kHttpResponseNoPartition =
    R"jsonString({ "partitions": []})jsonString";

constexpr auto kBlobDataHandle = R"(4eed6ed1-0d32-43b9-ae79-043cb4256432)";

const std::string kCatalog = "hrn:here:data:::hereos-internal-test-v2";
const std::string kLayerId = "testlayer";
const auto kHRN = olp::client::HRN::FromString(kCatalog);
const auto kPartitionId = "269";
const auto kTimeout = std::chrono::seconds(5);

template <class T>
void SetupNetworkExpectation(NetworkMock& network_mock, T url, T response,
                             int status) {
  EXPECT_CALL(network_mock, Send(IsGetRequest(url), _, _, _, _))
      .WillOnce(ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(status), response));
}

TEST(VolatileLayerClientImplTest, GetData) {
  std::shared_ptr<NetworkMock> network_mock = std::make_shared<NetworkMock>();
  std::shared_ptr<CacheMock> cache_mock = std::make_shared<CacheMock>();
  olp::client::OlpClientSettings settings;
  settings.network_request_handler = network_mock;
  settings.cache = cache_mock;

  VolatileLayerClientImpl client(kHRN, kLayerId, settings);

  {
    SCOPED_TRACE("Get Data with DataHandle");

    SetupNetworkExpectation(*network_mock, kUrlLookupVolatileBlob,
                            kHttpResponseLookupVolatileBlob,
                            olp::http::HttpStatusCode::OK);

    SetupNetworkExpectation(*network_mock, kUrlVolatileBlobData, "someData",
                            olp::http::HttpStatusCode::OK);

    std::promise<DataResponse> promise;
    std::future<DataResponse> future(promise.get_future());

    auto token = client.GetData(
        DataRequest().WithDataHandle(kBlobDataHandle),
        [&](DataResponse response) { promise.set_value(response); });

    EXPECT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    ASSERT_TRUE(response.IsSuccessful());

    Mock::VerifyAndClearExpectations(network_mock.get());
  }

  {
    SCOPED_TRACE("Get Data with PartitionId");

    SetupNetworkExpectation(*network_mock, kUrlLookupQuery,
                            kHttpResponseLookupQuery,
                            olp::http::HttpStatusCode::OK);

    SetupNetworkExpectation(*network_mock, kUrlQueryPartition269,
                            kHttpResponsePartition269,
                            olp::http::HttpStatusCode::OK);

    SetupNetworkExpectation(*network_mock, kUrlLookupVolatileBlob,
                            kHttpResponseLookupVolatileBlob,
                            olp::http::HttpStatusCode::OK);

    SetupNetworkExpectation(*network_mock, kUrlVolatileBlobData, "someData",
                            olp::http::HttpStatusCode::OK);

    std::promise<DataResponse> promise;
    std::future<DataResponse> future = promise.get_future();

    auto token = client.GetData(
        DataRequest().WithPartitionId(kPartitionId),
        [&](DataResponse response) { promise.set_value(response); });

    EXPECT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    ASSERT_TRUE(response.IsSuccessful());

    Mock::VerifyAndClearExpectations(network_mock.get());
  }

  {
    SCOPED_TRACE("Get Data from non existent partition");

    SetupNetworkExpectation(*network_mock, kUrlLookupQuery,
                            kHttpResponseLookupQuery,
                            olp::http::HttpStatusCode::OK);

    SetupNetworkExpectation(*network_mock, kUrlQueryPartition269,
                            kHttpResponseNoPartition,
                            olp::http::HttpStatusCode::OK);

    std::promise<DataResponse> promise;
    std::future<DataResponse> future = promise.get_future();

    auto token = client.GetData(
        DataRequest().WithPartitionId(kPartitionId),
        [&](DataResponse response) { promise.set_value(response); });

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

    DataResponse data_response;
    {
      // Client owns the task scheduler
      auto caller_thread_id = std::this_thread::get_id();
      VolatileLayerClientImpl client(kHRN, kLayerId, std::move(settings));
      client.GetData(DataRequest().WithPartitionId(kPartitionId),
                     [&](DataResponse response) {
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

}  // namespace
