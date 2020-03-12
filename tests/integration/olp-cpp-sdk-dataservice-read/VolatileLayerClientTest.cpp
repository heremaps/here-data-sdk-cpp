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

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_LOOKUP_METADATA));

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

  ON_CALL(*network_mock_,
          Send(IsGetRequest(URL_LOOKUP_VOLATILE_BLOB), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_LOOKUP_VOLATILE_BLOB));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_QUERY), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_LOOKUP_QUERY));

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

TEST_F(DataserviceReadVolatileLayerClientTest, GetPartitionsVersionIsIgnored) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1);

  olp::dataservice::read::VolatileLayerClient client(hrn, "testlayer",
                                                     settings_);

  {
    SCOPED_TRACE(
        "Online request with version in request. Version should be ignored.");
    auto request = olp::dataservice::read::PartitionsRequest();
    request.WithVersion(4);
    request.WithFetchOption(FetchOptions::OnlineIfNotFound);

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
    SCOPED_TRACE("Cache have data without version");
    auto request = olp::dataservice::read::PartitionsRequest();
    request.WithFetchOption(FetchOptions::CacheOnly);

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
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
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
        .WillRepeatedly(ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(
                olp::http::HttpStatusCode::TOO_MANY_REQUESTS),
            "Server busy at the moment."));

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
        .Times(1);
  }

  olp::client::RetrySettings retry_settings;
  retry_settings.retry_condition =
      [](const olp::client::HttpResponse& response) {
        return olp::http::HttpStatusCode::TOO_MANY_REQUESTS == response.status;
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

  olp::dataservice::read::VolatileLayerClient client(hrn, "somewhat_not_okay",
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
  ASSERT_EQ(olp::client::ErrorCode::NotFound,
            partitions_response.GetError().GetErrorCode());
}

TEST_F(DataserviceReadVolatileLayerClientTest, GetPartitionsGarbageResponse) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
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
      wait_for_cancel, pause_for_cancel,
      {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP_METADATA});

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
  cancel_token.Cancel();
  pause_for_cancel->set_value();  // unblock the handler
  PartitionsResponse partitions_response = promise.get_future().get();

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
        .WillOnce(ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(
                olp::http::HttpStatusCode::TOO_MANY_REQUESTS),
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
  ASSERT_EQ(olp::http::HttpStatusCode::FORBIDDEN,
            partitions_response.GetError().GetHttpStatusCode());

  // Check for cached response
  request.WithFetchOption(CacheOnly);
  client.GetPartitions(request, [&](PartitionsResponse response) {
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

  auto client = std::make_unique<olp::dataservice::read::VolatileLayerClient>(
      hrn, "testlayer", settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithDataHandle("volatileHandle");

  auto future = client->GetData(request);

  auto data_response = future.GetFuture().get();

  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_LT(0, data_response.GetResult()->size());
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

  auto client = std::make_unique<olp::dataservice::read::VolatileLayerClient>(
      hrn, "testlayer_volatile", settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithPartitionId("269");

  auto future = client->GetData(request);

  auto data_response = future.GetFuture().get();

  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_LT(0, data_response.GetResult()->size());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ("someData", data_string);
}

TEST_F(DataserviceReadVolatileLayerClientTest,
       CancelPendingRequestsPartitions) {
  olp::client::HRN hrn(GetTestCatalog());

  auto client = std::make_unique<VolatileLayerClient>(hrn, "testlayer_volatile",
                                                      settings_);
  auto partitions_request = PartitionsRequest().WithFetchOption(OnlineOnly);
  auto data_request =
      DataRequest().WithPartitionId("269").WithFetchOption(OnlineOnly);

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

}  // namespace
