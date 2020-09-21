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
#include <olp/dataservice/read/PrefetchTileResult.h>
#include <olp/dataservice/read/VolatileLayerClient.h>

#include "HttpResponses.h"

namespace {

namespace read = olp::dataservice::read;
using testing::_;

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
  std::shared_ptr<NetworkMock> network_mock_;
};

DataserviceReadVolatileLayerClientTest::
    DataserviceReadVolatileLayerClientTest() = default;

DataserviceReadVolatileLayerClientTest::
    ~DataserviceReadVolatileLayerClientTest() = default;

std::string DataserviceReadVolatileLayerClientTest::GetTestCatalog() {
  return "hrn:here:data::olp-here-test:hereos-internal-test-v2";
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
  settings_.task_scheduler =
      olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1u);

  SetUpCommonNetworkMockCalls();
}

void DataserviceReadVolatileLayerClientTest::TearDown() {
  ::testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  network_mock_.reset();
}

void DataserviceReadVolatileLayerClientTest::SetUpCommonNetworkMockCalls() {
  ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_LOOKUP_CONFIG));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_CONFIG));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_LOOKUP));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_PARTITIONS_VOLATILE), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_PARTITIONS));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_PARTITIONS_VOLATILE_INVALID_LAYER), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                 olp::http::HttpStatusCode::NOT_FOUND),
                             HTTP_RESPONSE_INVALID_VERSION_VN1));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_LOOKUP));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_QUADKEYS_VOLATILE_1476147), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_QUADKEYS_1476147));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_QUADKEYS_VOLATILE_92259), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_QUADKEYS_92259));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_BLOB_DATA_VOLATILE_PREFETCH_1), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_BLOB_DATA_PREFETCH_1));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_BLOB_DATA_VOLATILE_PREFETCH_2), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_BLOB_DATA_PREFETCH_2));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_BLOB_DATA_VOLATILE_PREFETCH_4), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_BLOB_DATA_PREFETCH_4));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_BLOB_DATA_VOLATILE_PREFETCH_5), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_BLOB_DATA_PREFETCH_5));

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_BLOB_DATA_VOLATILE_PREFETCH_6), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_BLOB_DATA_PREFETCH_6));
  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_BLOB_DATA_VOLATILE_PREFETCH_7), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_BLOB_DATA_PREFETCH_7));

  // Catch any non-interesting network calls that don't need to be verified
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _)).Times(testing::AtLeast(0));
}

TEST_F(DataserviceReadVolatileLayerClientTest, GetPartitions) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1);

  read::VolatileLayerClient client(hrn, "testlayer", settings_);

  auto request = read::PartitionsRequest();
  read::PartitionsResponse partitions_response;
  olp::client::Condition condition;
  client.GetPartitions(request, [&](read::PartitionsResponse response) {
    partitions_response = std::move(response);
    condition.Notify();
  });

  ASSERT_TRUE(condition.Wait(kTimeout));

  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(4u, partitions_response.GetResult().GetPartitions().size());
}

TEST_F(DataserviceReadVolatileLayerClientTest, GetPartitionsVersionIsIgnored) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1);

  read::VolatileLayerClient client(hrn, "testlayer", settings_);

  {
    SCOPED_TRACE("Online request.");
    auto request = read::PartitionsRequest();
    request.WithFetchOption(read::FetchOptions::OnlineIfNotFound);

    read::PartitionsResponse partitions_response;
    olp::client::Condition condition;
    client.GetPartitions(request, [&](read::PartitionsResponse response) {
      partitions_response = std::move(response);
      condition.Notify();
    });

    ASSERT_TRUE(condition.Wait(kTimeout));

    ASSERT_TRUE(partitions_response.IsSuccessful())
        << ApiErrorToString(partitions_response.GetError());
    ASSERT_EQ(4u, partitions_response.GetResult().GetPartitions().size());
  }

  {
    SCOPED_TRACE("Cache have data without version");
    auto request = read::PartitionsRequest();
    request.WithFetchOption(read::FetchOptions::CacheOnly);

    read::PartitionsResponse partitions_response;
    olp::client::Condition condition;
    client.GetPartitions(request, [&](read::PartitionsResponse response) {
      partitions_response = std::move(response);
      condition.Notify();
    });

    ASSERT_TRUE(condition.Wait(kTimeout));

    ASSERT_TRUE(partitions_response.IsSuccessful())
        << ApiErrorToString(partitions_response.GetError());
    ASSERT_EQ(4u, partitions_response.GetResult().GetPartitions().size());
  }
}

TEST_F(DataserviceReadVolatileLayerClientTest, GetPartitionsCancellableFuture) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1);

  read::VolatileLayerClient client(hrn, "testlayer", settings_);

  auto request = read::PartitionsRequest();
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

  read::VolatileLayerClient client(hrn, "testlayer", settings_);

  auto request = read::PartitionsRequest();
  auto cancellable = client.GetPartitions(request);
  auto future = cancellable.GetFuture();

  cancellable.GetCancellationToken().Cancel();
  ASSERT_EQ(std::future_status::ready, future.wait_for(kTimeout));

  auto response = future.get();
  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(response.GetError().GetErrorCode(),
            olp::client::ErrorCode::Cancelled);
}

TEST_F(DataserviceReadVolatileLayerClientTest, GetEmptyPartitions) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_PARTITIONS_VOLATILE), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_EMPTY_PARTITIONS));

  read::VolatileLayerClient client(hrn, "testlayer", settings_);

  auto request = read::PartitionsRequest();
  read::PartitionsResponse partitions_response;
  olp::client::Condition condition;
  client.GetPartitions(request, [&](read::PartitionsResponse response) {
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
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_PARTITIONS_V2));

  read::VolatileLayerClient client(hrn, "testlayer_volatile", settings_);

  auto request = read::PartitionsRequest();
  read::PartitionsResponse partitions_response;
  olp::client::Condition condition;
  client.GetPartitions(request, [&](read::PartitionsResponse response) {
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

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_PARTITIONS_VOLATILE), _, _, _, _))
        .Times(2)
        .WillRepeatedly(ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(
                olp::http::HttpStatusCode::TOO_MANY_REQUESTS),
            "Server busy at the moment."));

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_PARTITIONS_VOLATILE), _, _, _, _))
        .Times(1);
  }

  olp::client::RetrySettings retry_settings;
  retry_settings.retry_condition =
      [](const olp::client::HttpResponse& response) {
        return olp::http::HttpStatusCode::TOO_MANY_REQUESTS == response.status;
      };
  settings_.retry_settings = retry_settings;
  read::VolatileLayerClient client(hrn, "testlayer", settings_);

  auto request = read::PartitionsRequest();
  read::PartitionsResponse partitions_response;
  olp::client::Condition condition;
  client.GetPartitions(request, [&](read::PartitionsResponse response) {
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

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(2)
        .WillRepeatedly(ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(
                olp::http::HttpStatusCode::TOO_MANY_REQUESTS),
            "Server busy at the moment."));

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(1);
  }

  olp::client::RetrySettings retry_settings;
  retry_settings.retry_condition =
      [](const olp::client::HttpResponse& response) {
        return olp::http::HttpStatusCode::TOO_MANY_REQUESTS == response.status;
      };
  settings_.retry_settings = retry_settings;
  read::VolatileLayerClient client(hrn, "testlayer", settings_);

  auto request = read::PartitionsRequest();
  read::PartitionsResponse partitions_response;
  olp::client::Condition condition;
  client.GetPartitions(request, [&](read::PartitionsResponse response) {
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

  read::VolatileLayerClient client(hrn, "somewhat_not_okay", settings_);

  auto request = read::PartitionsRequest();
  read::PartitionsResponse partitions_response;
  olp::client::Condition condition;
  client.GetPartitions(request, [&](read::PartitionsResponse response) {
    partitions_response = std::move(response);
    condition.Notify();
  });

  ASSERT_TRUE(condition.Wait(kTimeout));

  ASSERT_FALSE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(olp::client::ErrorCode::NotFound,
            partitions_response.GetError().GetErrorCode());
}

TEST_F(DataserviceReadVolatileLayerClientTest, GetPartitionsGarbageResponse) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   R"jsonString(kd3sdf\)jsonString"));

  read::VolatileLayerClient client(hrn, "testlayer", settings_);

  auto request = read::PartitionsRequest();
  read::PartitionsResponse partitions_response;
  olp::client::Condition condition;
  client.GetPartitions(request, [&](read::PartitionsResponse response) {
    partitions_response = std::move(response);
    condition.Notify();
  });

  ASSERT_TRUE(condition.Wait(kTimeout));

  ASSERT_FALSE(partitions_response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::Unknown,
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
      wait_for_cancel, pause_for_cancel,
      {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP});

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(request_id))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);

  std::promise<read::PartitionsResponse> promise;
  auto callback = [&promise](read::PartitionsResponse response) {
    promise.set_value(response);
  };

  read::VolatileLayerClient client(hrn, "testlayer", settings_);

  auto request = read::PartitionsRequest();
  auto cancel_token = client.GetPartitions(request, callback);

  wait_for_cancel->get_future().get();  // wait for handler to get the request
  cancel_token.Cancel();
  pause_for_cancel->set_value();  // unblock the handler
  read::PartitionsResponse partitions_response = promise.get_future().get();

  ASSERT_FALSE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            partitions_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            partitions_response.GetError().GetErrorCode());
}

TEST_F(DataserviceReadVolatileLayerClientTest, GetPartitionsCacheOnly) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .Times(0);

  read::VolatileLayerClient client(hrn, "testlayer_volatile", settings_);

  auto request =
      read::PartitionsRequest().WithFetchOption(read::FetchOptions::CacheOnly);
  read::PartitionsResponse partitions_response;
  olp::client::Condition condition;
  client.GetPartitions(request, [&](read::PartitionsResponse response) {
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
        .Times(4)
        .WillRepeatedly(ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(
                olp::http::HttpStatusCode::TOO_MANY_REQUESTS),
            "Server busy at the moment."));
  }

  read::VolatileLayerClient client(hrn, "testlayer", settings_);

  auto request =
      read::PartitionsRequest().WithFetchOption(read::FetchOptions::OnlineOnly);
  {
    read::PartitionsResponse partitions_response;
    olp::client::Condition condition;
    client.GetPartitions(request, [&](read::PartitionsResponse response) {
      partitions_response = std::move(response);
      condition.Notify();
    });

    ASSERT_TRUE(condition.Wait(kTimeout));

    ASSERT_TRUE(partitions_response.IsSuccessful())
        << ApiErrorToString(partitions_response.GetError());
    ASSERT_EQ(4u, partitions_response.GetResult().GetPartitions().size());
  }

  {
    read::PartitionsResponse partitions_response;
    olp::client::Condition condition;
    client.GetPartitions(request, [&](read::PartitionsResponse response) {
      partitions_response = std::move(response);
      condition.Notify();
    });

    ASSERT_TRUE(condition.Wait(kTimeout));
    // Should fail despite valid cache entry
    ASSERT_FALSE(partitions_response.IsSuccessful())
        << ApiErrorToString(partitions_response.GetError());
  }
}

TEST_F(DataserviceReadVolatileLayerClientTest, GetPartitionsCacheWithUpdate) {
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
      wait_to_start_signal, pre_callback_wait,
      {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_PARTITIONS},
      wait_for_end_signal);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_PARTITIONS_VOLATILE), _, _, _, _))
      .Times(1)
      .WillOnce(testing::Invoke(std::move(send_mock)));

  read::VolatileLayerClient client(hrn, "testlayer", settings_);
  auto request = read::PartitionsRequest().WithFetchOption(
      read::FetchOptions::CacheWithUpdate);
  {
    read::PartitionsResponse partitions_response;
    olp::client::Condition condition;
    client.GetPartitions(request, [&](read::PartitionsResponse response) {
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
    request.WithFetchOption(read::FetchOptions::CacheOnly);
    read::PartitionsResponse partitions_response;
    olp::client::Condition condition;
    client.GetPartitions(request, [&](read::PartitionsResponse response) {
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
  read::VolatileLayerClient client(hrn, "testlayer", settings_);
  {
    testing::InSequence s;
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_PARTITIONS_VOLATILE), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_PARTITIONS_VOLATILE), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::FORBIDDEN),
                                     HTTP_RESPONSE_403));
  }

  // Populate cache
  auto request = read::PartitionsRequest();
  read::PartitionsResponse partitions_response;
  olp::client::Condition condition;
  client.GetPartitions(request, [&](read::PartitionsResponse response) {
    partitions_response = std::move(response);
    condition.Notify();
  });

  ASSERT_TRUE(condition.Wait(kTimeout));
  ASSERT_TRUE(partitions_response.IsSuccessful());

  // Receive 403
  request.WithFetchOption(read::FetchOptions::OnlineOnly);
  client.GetPartitions(request, [&](read::PartitionsResponse response) {
    partitions_response = std::move(response);
    condition.Notify();
  });
  ASSERT_TRUE(condition.Wait(kTimeout));
  ASSERT_FALSE(partitions_response.IsSuccessful());
  ASSERT_EQ(olp::http::HttpStatusCode::FORBIDDEN,
            partitions_response.GetError().GetHttpStatusCode());

  // Check for cached response
  request.WithFetchOption(read::FetchOptions::CacheOnly);
  client.GetPartitions(request, [&](read::PartitionsResponse response) {
    partitions_response = std::move(response);
    condition.Notify();
  });
  ASSERT_TRUE(condition.Wait(kTimeout));
  ASSERT_FALSE(partitions_response.IsSuccessful());
}

TEST_F(DataserviceReadVolatileLayerClientTest, GetVolatileDataHandle) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(
      *network_mock_,
      Send(IsGetRequest(
               "https://volatile-blob-ireland.data.api.platform.here.com/"
               "blobstore/v1/catalogs/hereos-internal-test-v2/layers/"
               "testlayer/data/volatileHandle"),
           _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   "someData"));

  auto client =
      std::make_unique<read::VolatileLayerClient>(hrn, "testlayer", settings_);

  auto request = read::DataRequest();
  request.WithDataHandle("volatileHandle");

  auto future = client->GetData(request);

  auto data_response = future.GetFuture().get();

  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_FALSE(data_response.GetResult()->empty());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ("someData", data_string);
}

TEST_F(DataserviceReadVolatileLayerClientTest, GetVolatileDataByPartitionId) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_QUERY_VOLATILE_PARTITION_269), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_PARTITIONS_V2));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_VOLATILE_BLOB_DATA), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   "someData"));

  auto client = std::make_unique<read::VolatileLayerClient>(
      hrn, "testlayer_volatile", settings_);

  auto request = read::DataRequest();
  request.WithPartitionId("269");

  auto future = client->GetData(request);

  auto data_response = future.GetFuture().get();

  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_FALSE(data_response.GetResult()->empty());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ("someData", data_string);
}

TEST_F(DataserviceReadVolatileLayerClientTest,
       CancelPendingRequestsPartitions) {
  olp::client::HRN hrn(GetTestCatalog());

  auto client = std::make_unique<read::VolatileLayerClient>(
      hrn, "testlayer_volatile", settings_);
  auto partitions_request =
      read::PartitionsRequest().WithFetchOption(read::FetchOptions::OnlineOnly);
  auto data_request =
      read::DataRequest().WithPartitionId("269").WithFetchOption(
          read::FetchOptions::OnlineOnly);

  auto request_started = std::make_shared<std::promise<void>>();
  auto continue_request = std::make_shared<std::promise<void>>();

  {
    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
        request_started, continue_request,
        {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_PARTITIONS_V2});

    EXPECT_CALL(
        *network_mock_,
        Send(IsGetRequest(URL_QUERY_VOLATILE_PARTITION_269), _, _, _, _))
        .Times(1)
        .WillOnce(testing::Invoke(std::move(send_mock)));

    EXPECT_CALL(*network_mock_, Cancel(request_id))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));

    ON_CALL(*network_mock_,
            Send(IsGetRequest(URL_VOLATILE_BLOB_DATA), _, _, _, _))
        .WillByDefault(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               "someData"));
  }

  auto data_future = client->GetData(data_request);
  auto partitions_future = client->GetPartitions(partitions_request);

  request_started->get_future().get();
  client->CancelPendingRequests();
  continue_request->set_value();

  read::PartitionsResponse partitions_response =
      partitions_future.GetFuture().get();

  ASSERT_FALSE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());

  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            partitions_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            partitions_response.GetError().GetErrorCode());

  read::DataResponse data_response = data_future.GetFuture().get();

  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());

  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode());
}

TEST_F(DataserviceReadVolatileLayerClientTest, RemoveFromCachePartition) {
  olp::client::HRN hrn(GetTestCatalog());

  auto partition_id = "269";
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_QUERY_VOLATILE_PARTITION_269), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_PARTITIONS_V2));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_VOLATILE_BLOB_DATA), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   "someData"));

  read::VolatileLayerClient client(hrn, "testlayer_volatile", settings_);

  auto request = read::DataRequest();
  request.WithPartitionId(partition_id);

  auto future = client.GetData(request);

  auto data_response = future.GetFuture().get();

  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_FALSE(data_response.GetResult()->empty());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ("someData", data_string);

  // remove the data from cache
  ASSERT_TRUE(client.RemoveFromCache(partition_id));

  // check the data is not available in cache
  request.WithFetchOption(read::FetchOptions::CacheOnly);
  future = client.GetData(request);
  data_response = future.GetFuture().get();
  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
}

TEST_F(DataserviceReadVolatileLayerClientTest, RemoveFromCacheTileKey) {
  olp::client::HRN hrn(GetTestCatalog());

  auto partition_id = "269";
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_QUERY_VOLATILE_PARTITION_269), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_PARTITIONS_V2));

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_VOLATILE_BLOB_DATA), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   "someData"));

  read::VolatileLayerClient client(hrn, "testlayer_volatile", settings_);

  auto request = read::DataRequest().WithPartitionId(partition_id);

  auto future = client.GetData(request);

  auto data_response = future.GetFuture().get();

  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_FALSE(data_response.GetResult()->empty());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ("someData", data_string);

  // remove the data from cache
  auto tile_key = olp::geo::TileKey::FromHereTile(partition_id);
  ASSERT_TRUE(client.RemoveFromCache(tile_key));

  // check the data is not available in cache
  request.WithFetchOption(read::FetchOptions::CacheOnly);
  future = client.GetData(request);
  data_response = future.GetFuture().get();
  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
}

TEST_F(DataserviceReadVolatileLayerClientTest, PrefetchTilesWithCache) {
  olp::client::HRN catalog(GetTestCatalog());
  constexpr auto kLayerId = "hype-test-prefetch";
  read::VolatileLayerClient client(catalog, kLayerId, settings_);
  {
    SCOPED_TRACE("Prefetch tiles online and store them in memory cache");
    std::vector<olp::geo::TileKey> tile_keys = {
        olp::geo::TileKey::FromHereTile("5904591")};
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
    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    ASSERT_FALSE(response.GetResult().empty());

    const auto& result = response.GetResult();
    for (auto const& tile_result : result) {
      ASSERT_TRUE(tile_result->IsSuccessful());
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
    }
  }

  {
    SCOPED_TRACE("Read cached data from pre-fetched sub-partition #1");
    auto promise = std::make_shared<std::promise<read::DataResponse>>();
    std::future<read::DataResponse> future = promise->get_future();
    auto token =
        client.GetData(read::DataRequest()
                           .WithPartitionId("23618365")
                           .WithFetchOption(read::FetchOptions::CacheOnly),
                       [promise](read::DataResponse response) {
                         promise->set_value(std::move(response));
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
    auto promise = std::make_shared<std::promise<read::DataResponse>>();
    std::future<read::DataResponse> future = promise->get_future();
    auto token =
        client.GetData(read::DataRequest()
                           .WithPartitionId("23618366")
                           .WithFetchOption(read::FetchOptions::CacheOnly),
                       [promise](read::DataResponse response) {
                         promise->set_value(std::move(response));
                       });

    ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful())
        << ApiErrorToString(response.GetError());
    ASSERT_TRUE(response.GetResult() != nullptr);
    ASSERT_NE(response.GetResult()->size(), 0u);
  }
}

TEST_F(DataserviceReadVolatileLayerClientTest,
       PrefetchSibilingTilesDefaultLevels) {
  olp::client::HRN catalog(GetTestCatalog());
  constexpr auto kLayerId = "hype-test-prefetch";
  read::VolatileLayerClient client(catalog, kLayerId, settings_);
  {
    SCOPED_TRACE("Prefetch tiles online, ");
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_VOLATILE_92259), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_QUADKEYS_92259));
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_VOLATILE_23618364), _, _, _, _))
        .Times(0);
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_VOLATILE_1476147), _, _, _, _))
        .Times(0);
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_VOLATILE_5904591), _, _, _, _))
        .Times(0);
    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_QUADKEYS_VOLATILE_369036), _, _, _, _))
        .Times(0);

    std::vector<olp::geo::TileKey> tile_keys = {
        olp::geo::TileKey::FromHereTile("23618366"),
        olp::geo::TileKey::FromHereTile("23618365")};
    auto request = read::PrefetchTilesRequest().WithTileKeys(tile_keys);
    auto promise =
        std::make_shared<std::promise<read::PrefetchTilesResponse>>();
    std::future<read::PrefetchTilesResponse> future = promise->get_future();
    auto token = client.PrefetchTiles(
        request, [promise](read::PrefetchTilesResponse response) {
          promise->set_value(std::move(response));
        });

    ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
    auto response = future.get();
    ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
    ASSERT_FALSE(response.GetResult().empty());

    const auto& result = response.GetResult();
    for (auto const& tile_result : result) {
      ASSERT_TRUE(tile_result->IsSuccessful());
      ASSERT_TRUE(tile_result->tile_key_.IsValid());
    }
  }
}

TEST_F(DataserviceReadVolatileLayerClientTest, PrefetchTilesWrongLevels) {
  olp::client::HRN catalog(GetTestCatalog());
  constexpr auto kLayerId = "hype-test-prefetch";
  std::vector<olp::geo::TileKey> tile_keys = {
      olp::geo::TileKey::FromHereTile("5904591")};

  ON_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                 olp::http::HttpStatusCode::FORBIDDEN),
                             HTTP_RESPONSE_403));

  read::VolatileLayerClient client(catalog, kLayerId, settings_);

  auto request = read::PrefetchTilesRequest().WithTileKeys(tile_keys);
  auto cancel_future = client.PrefetchTiles(request);
  auto raw_future = cancel_future.GetFuture();

  ASSERT_NE(raw_future.wait_for(kTimeout), std::future_status::timeout);
  auto response = raw_future.get();
  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::AccessDenied,
            response.GetError().GetErrorCode());
  ASSERT_TRUE(response.GetResult().empty());
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
      {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  std::promise<read::PrefetchTilesResponse> promise;
  std::future<read::PrefetchTilesResponse> future = promise.get_future();

  const olp::client::HRN catalog(GetTestCatalog());
  constexpr auto kLayerId = "prefetch-catalog";

  auto client =
      std::make_unique<read::VolatileLayerClient>(catalog, kLayerId, settings_);
  ASSERT_TRUE(client);

  std::vector<olp::geo::TileKey> tile_keys = {
      olp::geo::TileKey::FromHereTile("23618365")};
  auto request = read::PrefetchTilesRequest()
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(11)
                     .WithMaxLevel(12);
  auto token = client->PrefetchTiles(
      request, [&promise](read::PrefetchTilesResponse response) {
        promise.set_value(std::move(response));
      });

  wait_for_cancel->get_future().get();
  client.reset();
  pause_for_cancel->set_value();

  ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
  auto response = future.get();
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
      {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  std::promise<read::PrefetchTilesResponse> promise;
  std::future<read::PrefetchTilesResponse> future = promise.get_future();

  const olp::client::HRN catalog(GetTestCatalog());
  constexpr auto kLayerId = "prefetch-catalog";
  read::VolatileLayerClient client(catalog, kLayerId, settings_);
  std::vector<olp::geo::TileKey> tile_keys = {
      olp::geo::TileKey::FromHereTile("23618365")};
  auto request = read::PrefetchTilesRequest()
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(10)
                     .WithMaxLevel(12);
  auto token = client.PrefetchTiles(
      request, [&promise](read::PrefetchTilesResponse response) {
        promise.set_value(response);
      });

  wait_for_cancel->get_future().get();
  token.Cancel();
  pause_for_cancel->set_value();

  ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
  auto response = future.get();
  ASSERT_FALSE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_EQ(response.GetError().GetErrorCode(),
            olp::client::ErrorCode::Cancelled);
}

TEST_F(DataserviceReadVolatileLayerClientTest,
       PrefetchTilesWithCancellableFuture) {
  olp::client::HRN catalog(GetTestCatalog());
  constexpr auto kLayerId = "hype-test-prefetch";
  read::VolatileLayerClient client(catalog, kLayerId, settings_);

  std::vector<olp::geo::TileKey> tile_keys = {
      olp::geo::TileKey::FromHereTile("5904591")};
  auto request = read::PrefetchTilesRequest()
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(10)
                     .WithMaxLevel(12);
  auto cancel_future = client.PrefetchTiles(request);
  auto raw_future = cancel_future.GetFuture();

  ASSERT_NE(raw_future.wait_for(kTimeout), std::future_status::timeout);
  auto response = raw_future.get();
  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_FALSE(response.GetResult().empty());

  const auto& result = response.GetResult();
  for (auto const& tile_result : result) {
    ASSERT_TRUE(tile_result->IsSuccessful())
        << tile_result->GetError().GetMessage();
    ASSERT_TRUE(tile_result->tile_key_.IsValid());
  }
}

TEST_F(DataserviceReadVolatileLayerClientTest, PrefetchPriority) {
  olp::client::HRN catalog(GetTestCatalog());
  constexpr auto kLayerId = "hype-test-prefetch";
  auto scheduler = settings_.task_scheduler;
  std::promise<void> block_promise;
  std::promise<void> finish_promise;
  auto block_future = block_promise.get_future();
  auto finish_future = finish_promise.get_future();

  scheduler->ScheduleTask([&]() { block_future.wait_for(kTimeout); },
                          std::numeric_limits<uint32_t>::max());

  auto priority = 300u;
  // this priority should be less than priority, but greater than LOW
  auto finish_task_priority = 200u;

  read::VolatileLayerClient client(catalog, kLayerId, settings_);
  std::vector<olp::geo::TileKey> tile_keys = {
      olp::geo::TileKey::FromHereTile("5904591")};
  auto request = read::PrefetchTilesRequest()
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(8)
                     .WithMaxLevel(12)
                     .WithPriority(priority);
  auto future = client.PrefetchTiles(request).GetFuture();
  scheduler->ScheduleTask(
      [&]() {
        EXPECT_EQ(future.wait_for(std::chrono::milliseconds(0)),
                  std::future_status::ready);
        finish_promise.set_value();
      },
      finish_task_priority);

  // unblock queue
  block_promise.set_value();

  ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
  ASSERT_NE(finish_future.wait_for(kTimeout), std::future_status::timeout);

  auto response = future.get();
  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_FALSE(response.GetResult().empty());

  const auto& result = response.GetResult();
  for (auto const& tile_result : result) {
    ASSERT_TRUE(tile_result->IsSuccessful());
    ASSERT_TRUE(tile_result->tile_key_.IsValid());
  }
}

TEST_F(DataserviceReadVolatileLayerClientTest,
       CancelPrefetchTilesWithCancellableFuture) {
  olp::client::HRN catalog(GetTestCatalog());
  constexpr auto kLayerId = "hype-test-prefetch";
  read::VolatileLayerClient client(catalog, kLayerId, settings_);
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;
  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel,
      {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP});

  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _))
      .WillOnce(testing::Invoke(std::move(send_mock)));

  EXPECT_CALL(*network_mock_, Cancel(_))
      .WillOnce(testing::Invoke(std::move(cancel_mock)));

  std::vector<olp::geo::TileKey> tile_keys = {
      olp::geo::TileKey::FromHereTile("5904591")};
  auto request = read::PrefetchTilesRequest()
                     .WithTileKeys(tile_keys)
                     .WithMinLevel(10)
                     .WithMaxLevel(12);
  auto cancel_future = client.PrefetchTiles(request);

  wait_for_cancel->get_future().get();
  cancel_future.GetCancellationToken().Cancel();
  pause_for_cancel->set_value();

  auto raw_future = cancel_future.GetFuture();
  ASSERT_NE(raw_future.wait_for(kTimeout), std::future_status::timeout);
  auto response = raw_future.get();
  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult().empty());
}

TEST_F(DataserviceReadVolatileLayerClientTest, CancelPendingRequestsPrefetch) {
  olp::client::HRN hrn(GetTestCatalog());
  constexpr auto kLayerId = "testlayer_volatile";
  read::VolatileLayerClient client(hrn, kLayerId, settings_);
  auto request_started = std::make_shared<std::promise<void>>();
  auto continue_request = std::make_shared<std::promise<void>>();
  auto prefetch_request = read::PrefetchTilesRequest();
  auto data_request =
      read::DataRequest().WithPartitionId("269").WithFetchOption(
          read::FetchOptions::OnlineOnly);
  {
    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
        request_started, continue_request,
        {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_BLOB_DATA_269});

    EXPECT_CALL(
        *network_mock_,
        Send(IsGetRequest(URL_QUERY_VOLATILE_PARTITION_269), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_PARTITIONS_V2));

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_VOLATILE_BLOB_DATA), _, _, _, _))
        .Times(1)
        .WillOnce(testing::Invoke(std::move(send_mock)));

    EXPECT_CALL(*network_mock_, Cancel(request_id))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));
  }

  auto data_future = client.GetData(data_request).GetFuture();
  auto prefetch_future = client.PrefetchTiles(prefetch_request).GetFuture();

  request_started->get_future().get();
  client.CancelPendingRequests();
  continue_request->set_value();

  ASSERT_EQ(prefetch_future.wait_for(kTimeout), std::future_status::ready);
  auto prefetch_response = prefetch_future.get();
  ASSERT_FALSE(prefetch_response.IsSuccessful())
      << ApiErrorToString(prefetch_response.GetError());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            prefetch_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            prefetch_response.GetError().GetErrorCode());

  ASSERT_EQ(data_future.wait_for(kTimeout), std::future_status::ready);
  auto data_response = data_future.get();
  ASSERT_FALSE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            data_response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            data_response.GetError().GetErrorCode());
}

TEST_F(DataserviceReadVolatileLayerClientTest, DataRequestPriority) {
  olp::client::HRN hrn(GetTestCatalog());
  constexpr auto partition_id = "269";
  constexpr auto layer_id = "testlayer_volatile";

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(0);
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_QUERY_VOLATILE_PARTITION_269), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_PARTITIONS_V2));
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_VOLATILE_BLOB_DATA), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   "someData"));

  read::VolatileLayerClient client(hrn, layer_id, settings_);

  auto scheduler = settings_.task_scheduler;
  std::promise<void> block_promise;
  std::promise<void> finish_promise;
  auto block_future = block_promise.get_future();
  auto finish_future = finish_promise.get_future();

  scheduler->ScheduleTask([&]() { block_future.wait_for(kTimeout); },
                          std::numeric_limits<uint32_t>::max());

  auto priority = 700u;
  // this priority should be less than `priority`, but greater than NORMAL
  auto finish_task_priority = 600u;

  auto request =
      read::DataRequest().WithPartitionId(partition_id).WithPriority(priority);
  auto future = client.GetData(request).GetFuture();
  scheduler->ScheduleTask(
      [&]() {
        EXPECT_EQ(future.wait_for(std::chrono::milliseconds(0)),
                  std::future_status::ready);
        finish_promise.set_value();
      },
      finish_task_priority);

  // unblock queue
  block_promise.set_value();

  ASSERT_NE(future.wait_for(kTimeout), std::future_status::timeout);
  ASSERT_NE(finish_future.wait_for(kTimeout), std::future_status::timeout);

  auto response = future.get();

  ASSERT_TRUE(response.IsSuccessful()) << response.GetError().GetMessage();
  ASSERT_NE(response.GetResult(), nullptr);
  ASSERT_NE(response.GetResult()->size(), 0u);
}

}  // namespace
