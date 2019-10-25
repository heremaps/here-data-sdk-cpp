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
#include <olp/core/cache/DefaultCache.h>
#include <olp/core/client/Condition.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/http/Network.h>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include <olp/dataservice/read/PartitionsRequest.h>
#include <olp/dataservice/read/VolatileLayerClient.h>

#include "HttpResponses.h"

namespace {

using namespace olp::dataservice::read;
using namespace testing;
using namespace olp::tests::common;

const auto kTimeout = std::chrono::seconds(5);

class DataserviceReadVolatileLayerClientTest : public ::testing::Test {
 protected:
  DataserviceReadVolatileLayerClientTest();
  ~DataserviceReadVolatileLayerClientTest();
  std::string GetTestCatalog();
  static std::string ApiErrorToString(const olp::client::ApiError& error);

  void SetUp() override;
  void TearDown() override;

 private:
  void SetUpCommonNetworkMockCalls();

 protected:
  olp::client::OlpClientSettings settings_;
  std::shared_ptr<olp::tests::common::NetworkMock> network_mock_;
};

DataserviceReadVolatileLayerClientTest::
    DataserviceReadVolatileLayerClientTest() = default;

DataserviceReadVolatileLayerClientTest::
    ~DataserviceReadVolatileLayerClientTest() = default;

std::string DataserviceReadVolatileLayerClientTest::GetTestCatalog() {
  return "hrn:here:data:::hereos-internal-test-v2";
}

std::string DataserviceReadVolatileLayerClientTest::ApiErrorToString(
    const olp::client::ApiError& error) {
  std::ostringstream result_stream;
  result_stream << "ERROR: code: " << static_cast<int>(error.GetErrorCode())
                << ", status: " << error.GetHttpStatusCode()
                << ", message: " << error.GetMessage();
  return result_stream.str();
}

void DataserviceReadVolatileLayerClientTest::SetUp() {
  network_mock_ = std::make_shared<NetworkMock>();
  settings_ = olp::client::OlpClientSettings();
  settings_.network_request_handler = network_mock_;
  olp::cache::CacheSettings cache_settings;
  settings_.cache =
      olp::client::OlpClientSettingsFactory::CreateDefaultCache(cache_settings);

  SetUpCommonNetworkMockCalls();
}

void DataserviceReadVolatileLayerClientTest::TearDown() {
  ::testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  network_mock_.reset();
}

void DataserviceReadVolatileLayerClientTest::SetUpCommonNetworkMockCalls() {
  ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_LOOKUP_CONFIG));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(200), HTTP_RESPONSE_CONFIG));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_LOOKUP_METADATA));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_LATEST_CATALOG_VERSION));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_LAYER_VERSIONS), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_LAYER_VERSIONS));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_PARTITIONS));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_QUERY), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_LOOKUP_QUERY));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_QUERY_PARTITION_269), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_PARTITION_269));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_LOOKUP_BLOB));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_BLOB_DATA_269));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITION_3), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_PARTITION_3));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_LOOKUP_VOLATILE_BLOB), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_LOOKUP_VOLATILE_BLOB));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_LAYER_VERSIONS_V2), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_LAYER_VERSIONS_V2));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS_V2), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_PARTITIONS_V2));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_QUERY_PARTITION_269_V2), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_PARTITION_269_V2));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269_V2), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_BLOB_DATA_269_V2));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_QUERY_PARTITION_269_V10), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(400),
                             HTTP_RESPONSE_INVALID_VERSION_V10));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_QUERY_PARTITION_269_VN1), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(400),
                             HTTP_RESPONSE_INVALID_VERSION_VN1));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_LAYER_VERSIONS_V10), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(400),
                             HTTP_RESPONSE_INVALID_VERSION_V10));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_LAYER_VERSIONS_VN1), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(400),
                             HTTP_RESPONSE_INVALID_VERSION_VN1));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG_V2), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_CONFIG_V2));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_QUADKEYS_23618364), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_QUADKEYS_23618364));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_QUADKEYS_1476147), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_QUADKEYS_1476147));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_QUADKEYS_5904591), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_QUADKEYS_5904591));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_QUADKEYS_369036), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_QUADKEYS_369036));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_1), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_BLOB_DATA_PREFETCH_1));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_2), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_BLOB_DATA_PREFETCH_2));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_3), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_BLOB_DATA_PREFETCH_3));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_4), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_BLOB_DATA_PREFETCH_4));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_5), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_BLOB_DATA_PREFETCH_5));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_6), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_BLOB_DATA_PREFETCH_6));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_BLOB_DATA_PREFETCH_7), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_BLOB_DATA_PREFETCH_7));

  // Catch any non-interesting network calls that don't need to be verified
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _)).Times(testing::AtLeast(0));
}

TEST_F(DataserviceReadVolatileLayerClientTest, GetPartitions) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1);

  olp::dataservice::read::VolatileLayerClient client(hrn, "testlayer",
                                                     settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  PartitionsResponse partitions_response;
  olp::client::Condition condition;
  client.GetPartitions(request, [&](PartitionsResponse response) {
    partitions_response = std::move(response);
    condition.Notify();
  });

  ASSERT_TRUE(condition.Wait(kTimeout));

  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(4u, partitions_response.GetResult().GetPartitions().size());
}

TEST_F(DataserviceReadVolatileLayerClientTest, GetPartitionsCancellableFuture) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1);

  olp::dataservice::read::VolatileLayerClient client(hrn, "testlayer",
                                                     settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  auto cancellable = client.GetPartitions(request);
  auto future = cancellable.GetFuture();

  ASSERT_EQ(std::future_status::ready, future.wait_for(kTimeout));

  auto response = future.get();
  ASSERT_TRUE(response.IsSuccessful()) << ApiErrorToString(response.GetError());
  ASSERT_EQ(4u, response.GetResult().GetPartitions().size());
}

TEST_F(DataserviceReadVolatileLayerClientTest,
       GetPartitionsCancellableFutureCancellation) {
  olp::client::HRN hrn(GetTestCatalog());

  settings_.task_scheduler =
      olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);
  // Simulate a loaded queue
  settings_.task_scheduler->ScheduleTask(
      []() { std::this_thread::sleep_for(std::chrono::seconds(1)); });

  olp::dataservice::read::VolatileLayerClient client(hrn, "testlayer",
                                                     settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  auto cancellable = client.GetPartitions(request);
  auto future = cancellable.GetFuture();

  cancellable.GetCancellationToken().cancel();
  ASSERT_EQ(std::future_status::ready, future.wait_for(kTimeout));

  auto response = future.get();
  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(response.GetError().GetErrorCode(),
            olp::client::ErrorCode::Cancelled);
}

TEST_F(DataserviceReadVolatileLayerClientTest, GetEmptyPartitions) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                                   HTTP_RESPONSE_EMPTY_PARTITIONS));

  olp::dataservice::read::VolatileLayerClient client(hrn, "testlayer",
                                                     settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  PartitionsResponse partitions_response;
  olp::client::Condition condition;
  client.GetPartitions(request, [&](PartitionsResponse response) {
    partitions_response = std::move(response);
    condition.Notify();
  });

  ASSERT_TRUE(condition.Wait(kTimeout));

  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(0u, partitions_response.GetResult().GetPartitions().size());
}

TEST_F(DataserviceReadVolatileLayerClientTest, GetVolatilePartitions) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest("https://metadata.data.api.platform.here.com/"
                                "metadata/v1/catalogs/hereos-internal-test-v2/"
                                "layers/testlayer_volatile/partitions"),
                   _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                                   HTTP_RESPONSE_PARTITIONS_V2));

  olp::dataservice::read::VolatileLayerClient client(hrn, "testlayer_volatile",
                                                     settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  PartitionsResponse partitions_response;
  olp::client::Condition condition;
  client.GetPartitions(request, [&](PartitionsResponse response) {
    partitions_response = std::move(response);
    condition.Notify();
  });

  ASSERT_TRUE(condition.Wait(kTimeout));

  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(1u, partitions_response.GetResult().GetPartitions().size());
}

TEST_F(DataserviceReadVolatileLayerClientTest, GetPartitions429Error) {
  olp::client::HRN hrn(GetTestCatalog());

  {
    testing::InSequence s;

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
        .Times(2)
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(429),
                               "Server busy at the moment."));

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
        .Times(1);
  }

  olp::client::RetrySettings retry_settings;
  retry_settings.retry_condition =
      [](const olp::client::HttpResponse& response) {
        return 429 == response.status;
      };
  settings_.retry_settings = retry_settings;
  olp::dataservice::read::VolatileLayerClient client(hrn, "testlayer",
                                                     settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  PartitionsResponse partitions_response;
  olp::client::Condition condition;
  client.GetPartitions(request, [&](PartitionsResponse response) {
    partitions_response = std::move(response);
    condition.Notify();
  });

  ASSERT_TRUE(condition.Wait(kTimeout));

  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(4u, partitions_response.GetResult().GetPartitions().size());
}

TEST_F(DataserviceReadVolatileLayerClientTest, ApiLookup429) {
  olp::client::HRN hrn(GetTestCatalog());

  {
    testing::InSequence s;

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
        .Times(2)
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(429),
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
  settings_.retry_settings = retry_settings;
  olp::dataservice::read::VolatileLayerClient client(hrn, "testlayer",
                                                     settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  PartitionsResponse partitions_response;
  olp::client::Condition condition;
  client.GetPartitions(request, [&](PartitionsResponse response) {
    partitions_response = std::move(response);
    condition.Notify();
  });

  ASSERT_TRUE(condition.Wait(kTimeout));

  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(4u, partitions_response.GetResult().GetPartitions().size());
}

TEST_F(DataserviceReadVolatileLayerClientTest, GetPartitionsForInvalidLayer) {
  olp::client::HRN hrn(GetTestCatalog());

  olp::dataservice::read::VolatileLayerClient client(hrn, "InvalidLayer",
                                                     settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  PartitionsResponse partitions_response;
  olp::client::Condition condition;
  client.GetPartitions(request, [&](PartitionsResponse response) {
    partitions_response = std::move(response);
    condition.Notify();
  });

  ASSERT_TRUE(condition.Wait(kTimeout));

  ASSERT_FALSE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(olp::client::ErrorCode::InvalidArgument,
            partitions_response.GetError().GetErrorCode());
}

TEST_F(DataserviceReadVolatileLayerClientTest, GetPartitionsGarbageResponse) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                                   R"jsonString(kd3sdf\)jsonString"));

  olp::dataservice::read::VolatileLayerClient client(hrn, "testlayer",
                                                     settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  PartitionsResponse partitions_response;
  olp::client::Condition condition;
  client.GetPartitions(request, [&](PartitionsResponse response) {
    partitions_response = std::move(response);
    condition.Notify();
  });

  ASSERT_TRUE(condition.Wait(kTimeout));

  ASSERT_FALSE(partitions_response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::ServiceUnavailable,
            partitions_response.GetError().GetErrorCode());
}

TEST_F(DataserviceReadVolatileLayerClientTest,
       GetPartitionsCancelLookupMetadata) {
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

  std::promise<PartitionsResponse> promise;
  auto callback = [&promise](PartitionsResponse response) {
    promise.set_value(response);
  };

  VolatileLayerClient client(hrn, "testlayer", settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  auto cancel_token = client.GetPartitions(request, callback);

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

TEST_F(DataserviceReadVolatileLayerClientTest,
       GetPartitionsCancelLatestCatalogVersion) {
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

  std::promise<PartitionsResponse> promise;
  auto callback = [&promise](PartitionsResponse response) {
    promise.set_value(response);
  };

  VolatileLayerClient client(hrn, "testlayer", settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  auto cancel_token = client.GetPartitions(request, callback);

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

TEST_F(DataserviceReadVolatileLayerClientTest,
       DISABLED_GetPartitionsCancelLayerVersions) {
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

  std::promise<PartitionsResponse> promise;
  auto callback = [&promise](PartitionsResponse response) {
    promise.set_value(response);
  };

  VolatileLayerClient client(hrn, "testlayer", settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  auto cancel_token = client.GetPartitions(request, callback);

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

TEST_F(DataserviceReadVolatileLayerClientTest, GetPartitionsCacheOnly) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .Times(0);

  olp::dataservice::read::VolatileLayerClient client(hrn, "testlayer_volatile",
                                                     settings_);

  auto request = olp::dataservice::read::PartitionsRequest().WithFetchOption(
      FetchOptions::CacheOnly);
  PartitionsResponse partitions_response;
  olp::client::Condition condition;
  client.GetPartitions(request, [&](PartitionsResponse response) {
    partitions_response = std::move(response);
    condition.Notify();
  });

  ASSERT_TRUE(condition.Wait(kTimeout));

  ASSERT_FALSE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
}

TEST_F(DataserviceReadVolatileLayerClientTest, GetPartitionsOnlineOnly) {
  olp::client::HRN hrn(GetTestCatalog());

  {
    testing::InSequence s;

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(429),
                               "Server busy at the moment."));
  }

  olp::dataservice::read::VolatileLayerClient client(hrn, "testlayer",
                                                     settings_);

  auto request = olp::dataservice::read::PartitionsRequest().WithFetchOption(
      FetchOptions::OnlineOnly);
  {
    PartitionsResponse partitions_response;
    olp::client::Condition condition;
    client.GetPartitions(request, [&](PartitionsResponse response) {
      partitions_response = std::move(response);
      condition.Notify();
    });

    ASSERT_TRUE(condition.Wait(kTimeout));

    ASSERT_TRUE(partitions_response.IsSuccessful())
        << ApiErrorToString(partitions_response.GetError());
    ASSERT_EQ(4u, partitions_response.GetResult().GetPartitions().size());
  }

  {
    PartitionsResponse partitions_response;
    olp::client::Condition condition;
    client.GetPartitions(request, [&](PartitionsResponse response) {
      partitions_response = std::move(response);
      condition.Notify();
    });

    ASSERT_TRUE(condition.Wait(kTimeout));
    // Should fail despite valid cache entry
    ASSERT_FALSE(partitions_response.IsSuccessful())
        << ApiErrorToString(partitions_response.GetError());
  }
}

TEST_F(DataserviceReadVolatileLayerClientTest,
       DISABLED_GetPartitionsCacheWithUpdate) {
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

  olp::dataservice::read::VolatileLayerClient client(hrn, "testlayer",
                                                     settings_);
  auto request = olp::dataservice::read::PartitionsRequest().WithFetchOption(
      FetchOptions::CacheWithUpdate);
  {
    PartitionsResponse partitions_response;
    olp::client::Condition condition;
    client.GetPartitions(request, [&](PartitionsResponse response) {
      partitions_response = std::move(response);
      condition.Notify();
    });

    ASSERT_TRUE(condition.Wait(kTimeout));

    // Request 1 return. Cached value (nothing)
    ASSERT_FALSE(partitions_response.IsSuccessful())
        << ApiErrorToString(partitions_response.GetError());
  }

  {
    // Request 2 to check there is a cached value.
    wait_for_end_signal->get_future().get();
    request.WithFetchOption(CacheOnly);
    PartitionsResponse partitions_response;
    olp::client::Condition condition;
    client.GetPartitions(request, [&](PartitionsResponse response) {
      partitions_response = std::move(response);
      condition.Notify();
    });

    ASSERT_TRUE(condition.Wait(kTimeout));
    // Cache should be available here.
    ASSERT_TRUE(partitions_response.IsSuccessful())
        << ApiErrorToString(partitions_response.GetError());
  }
}

TEST_F(DataserviceReadVolatileLayerClientTest, GetPartitions403CacheClear) {
  olp::client::HRN hrn(GetTestCatalog());
  olp::dataservice::read::VolatileLayerClient client(hrn, "testlayer",
                                                     settings_);
  {
    testing::InSequence s;
    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(403), HTTP_RESPONSE_403));
  }

  // Populate cache
  auto request = olp::dataservice::read::PartitionsRequest();
  PartitionsResponse partitions_response;
  olp::client::Condition condition;
  client.GetPartitions(request, [&](PartitionsResponse response) {
    partitions_response = std::move(response);
    condition.Notify();
  });

  ASSERT_TRUE(condition.Wait(kTimeout));
  ASSERT_TRUE(partitions_response.IsSuccessful());

  // Receive 403
  request.WithFetchOption(OnlineOnly);
  client.GetPartitions(request, [&](PartitionsResponse response) {
    partitions_response = std::move(response);
    condition.Notify();
  });
  ASSERT_TRUE(condition.Wait(kTimeout));
  ASSERT_FALSE(partitions_response.IsSuccessful());
  ASSERT_EQ(403, partitions_response.GetError().GetHttpStatusCode());

  // Check for cached response
  request.WithFetchOption(CacheOnly);
  client.GetPartitions(request, [&](PartitionsResponse response) {
    partitions_response = std::move(response);
    condition.Notify();
  });
  ASSERT_TRUE(condition.Wait(kTimeout));
  ASSERT_FALSE(partitions_response.IsSuccessful());
}

/*
 * VolatileLayerClient::GetData ignores versions, as VolatileLayer should, but
 * PrefetchTiles do not, it fetches latest version and versioned tiles end up in
 * Cache. VolatileLayerClient::GetData cannot query verisoned tiles from cache.
 * Relates: OLPEDGE-965
 */
TEST_F(DataserviceReadVolatileLayerClientTest,
       DISABLED_PrefetchTilesWithCache) {
  olp::client::HRN catalog(GetTestCatalog());
  constexpr auto kLayerId = "hype-test-prefetch";
  settings_.cache = nullptr;
  auto client = std::make_unique<olp::dataservice::read::VolatileLayerClient>(
      catalog, kLayerId, settings_);
  ASSERT_TRUE(client);

  {
    SCOPED_TRACE("Prefetch tiles online and store them in memory cache");
    std::vector<olp::geo::TileKey> tile_keys = {
        olp::geo::TileKey::FromHereTile("5904591")};

    auto request = olp::dataservice::read::PrefetchTilesRequest()
                       .WithTileKeys(tile_keys)
                       .WithMinLevel(10)
                       .WithMaxLevel(12);

    std::promise<PrefetchTilesResponse> promise;
    std::future<PrefetchTilesResponse> future = promise.get_future();
    auto token = client->PrefetchTiles(
        request, [&promise](PrefetchTilesResponse response) {
          promise.set_value(response);
        });

    ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
    PrefetchTilesResponse response = future.get();
    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    ASSERT_FALSE(response.GetResult().empty());

    const auto& result = response.GetResult();

    for (auto tile_result : result) {
      ASSERT_TRUE(tile_result->IsSuccessful());
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
    }
  }

  {
    SCOPED_TRACE("Read cached data from pre-fetched sub-partition #1");
    std::promise<DataResponse> promise;
    std::future<DataResponse> future = promise.get_future();
    auto token = client->GetData(olp::dataservice::read::DataRequest()
                                     .WithPartitionId("23618365")
                                     .WithFetchOption(CacheOnly),
                                 [&promise](DataResponse response) {
                                   promise.set_value(std::move(response));
                                 });
    ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);

    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful())
        << ApiErrorToString(response.GetError());
    ASSERT_TRUE(response.GetResult() != nullptr);
    ASSERT_NE(response.GetResult()->size(), 0u);
  }

  {
    SCOPED_TRACE("Read cached data from pre-fetched sub-partition #2");
    std::promise<DataResponse> promise;
    std::future<DataResponse> future = promise.get_future();

    auto token = client->GetData(olp::dataservice::read::DataRequest()
                                     .WithPartitionId("1476147")
                                     .WithFetchOption(CacheOnly),
                                 [&promise](DataResponse response) {
                                   promise.set_value(std::move(response));
                                 });
    ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);

    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful())
        << ApiErrorToString(response.GetError());
    ASSERT_TRUE(response.GetResult() != nullptr);
    ASSERT_NE(response.GetResult()->size(), 0u);
  }
}

TEST_F(DataserviceReadVolatileLayerClientTest, PrefetchTilesBusy) {
  olp::client::HRN catalog{GetTestCatalog()};
  constexpr auto kLayerId = "hype-test-prefetch";

  auto client = std::make_unique<olp::dataservice::read::VolatileLayerClient>(
      catalog, kLayerId, settings_);
  ASSERT_TRUE(client);

  // Prepare the first request
  std::vector<olp::geo::TileKey> tile_keys1 = {
      olp::geo::TileKey::FromHereTile("5904591")};

  auto request1 = olp::dataservice::read::PrefetchTilesRequest()
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
      {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_QUADKEYS_5904591});

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_QUADKEYS_5904591), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  // Issue the first request
  std::promise<PrefetchTilesResponse> promise1;
  std::future<PrefetchTilesResponse> future1 = promise1.get_future();
  auto token1 = client->PrefetchTiles(
      request1, [&promise1](PrefetchTilesResponse response) {
        promise1.set_value(response);
      });

  // Wait for QuadKey request
  wait_for_quad_key_request->get_future().get();

  // Prepare the second request
  std::vector<olp::geo::TileKey> tile_keys2;
  tile_keys2.emplace_back(olp::geo::TileKey::FromHereTile("369036"));

  auto request2 = olp::dataservice::read::PrefetchTilesRequest()
                      .WithTileKeys(tile_keys2)
                      .WithMinLevel(9)
                      .WithMaxLevel(9);

  // Issue the second request
  std::promise<PrefetchTilesResponse> promise2;
  std::future<PrefetchTilesResponse> future2 = promise2.get_future();
  auto token2 = client->PrefetchTiles(
      request2, [&promise2](PrefetchTilesResponse response) {
        promise2.set_value(response);
      });

  // Unblock the QuadKey request
  pause_for_second_request->set_value();

  // Validate that the second request failed
  auto response2 = future2.get();
  ASSERT_FALSE(response2.IsSuccessful());

  auto& error = response2.GetError();
  ASSERT_EQ(olp::client::ErrorCode::SlowDown, error.GetErrorCode());

  // Get and validate the first request
  auto response1 = future1.get();
  ASSERT_TRUE(response1.IsSuccessful());

  auto& result1 = response1.GetResult();

  for (auto tile_result : result1) {
    ASSERT_TRUE(tile_result->IsSuccessful());
    ASSERT_TRUE(tile_result->tile_key_.IsValid());
  }
  ASSERT_EQ(6u, result1.size());
}

TEST_F(DataserviceReadVolatileLayerClientTest,
       PrefetchTilesCancelOnClientDeletion) {
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;
  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP_QUERY});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  std::promise<PrefetchTilesResponse> promise;
  std::future<PrefetchTilesResponse> future = promise.get_future();

  const olp::client::HRN catalog(GetTestCatalog());
  constexpr auto kLayerId = "prefetch-catalog";
  constexpr auto kParitionId = "prefetch-partition";

  auto client = std::make_unique<olp::dataservice::read::VolatileLayerClient>(
      catalog, kLayerId, settings_);
  ASSERT_TRUE(client);

  std::vector<olp::geo::TileKey> tile_keys = {
      olp::geo::TileKey::FromHereTile(kParitionId)};
  auto request = olp::dataservice::read::PrefetchTilesRequest()
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(10)
                     .WithMaxLevel(12);

  auto token = client->PrefetchTiles(
      request, [&promise](PrefetchTilesResponse response) {
        promise.set_value(response);
      });

  wait_for_cancel->get_future().get();
  client.reset();
  pause_for_cancel->set_value();

  ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
  PrefetchTilesResponse response = future.get();
  ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_EQ(response.GetError().GetErrorCode(),
            olp::client::ErrorCode::Cancelled);
}

TEST_F(DataserviceReadVolatileLayerClientTest, PrefetchTilesCancelOnLookup) {
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;
  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP_QUERY});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  std::promise<PrefetchTilesResponse> promise;
  std::future<PrefetchTilesResponse> future = promise.get_future();

  const olp::client::HRN catalog(GetTestCatalog());
  constexpr auto kLayerId = "prefetch-catalog";
  constexpr auto kParitionId = "prefetch-partition";

  auto client = std::make_unique<olp::dataservice::read::VolatileLayerClient>(
      catalog, kLayerId, settings_);
  ASSERT_TRUE(client);

  std::vector<olp::geo::TileKey> tile_keys = {
      olp::geo::TileKey::FromHereTile(kParitionId)};
  auto request = olp::dataservice::read::PrefetchTilesRequest()
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(10)
                     .WithMaxLevel(12);

  auto token = client->PrefetchTiles(
      request, [&promise](PrefetchTilesResponse response) {
        promise.set_value(response);
      });

  wait_for_cancel->get_future().get();
  token.cancel();
  pause_for_cancel->set_value();

  ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
  PrefetchTilesResponse response = future.get();
  ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_EQ(response.GetError().GetErrorCode(),
            olp::client::ErrorCode::Cancelled);
}

}  // namespace
