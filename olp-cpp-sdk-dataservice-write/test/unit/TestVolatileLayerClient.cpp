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
#include <olp/authentication/TokenProvider.h>
#include <olp/core/client/ApiError.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/network/HttpResponse.h>

#include <olp/dataservice/write/VolatileLayerClient.h>
#include <olp/dataservice/write/model/PublishPartitionDataRequest.h>

#include "testutils/CustomParameters.hpp"

#include "HttpResponses.h"

using namespace olp::dataservice::write;
using namespace olp::dataservice::write::model;

const std::string kEndpoint = "endpoint";
const std::string kAppid = "appid";
const std::string kSecret = "secret";
const std::string kCatalog = "catalog";
const std::string kVolatileLayer = "volatile_layer";

// TODO: Move duplicate test code to common header
namespace {

void PublishDataSuccessAssertions(
    const olp::client::ApiResponse<
        olp::dataservice::write::model::ResponseOkSingle,
        olp::client::ApiError>& result) {
  EXPECT_TRUE(result.IsSuccessful());
  EXPECT_FALSE(result.GetResult().GetTraceID().empty());
  EXPECT_EQ("", result.GetError().GetMessage());
}

template <typename T>
void PublishFailureAssertions(
    const olp::client::ApiResponse<T, olp::client::ApiError>& result) {
  EXPECT_FALSE(result.IsSuccessful());
  EXPECT_NE(result.GetError().GetHttpStatusCode(), 200);
  EXPECT_FALSE(result.GetError().GetMessage().empty());
}
}  // namespace

class VolatileLayerClientTestBase : public ::testing::TestWithParam<bool> {
 protected:
  virtual void SetUp() override {
    client_ = CreateVolatileLayerClient();
    data_ = GenerateData();
  }

  virtual void TearDown() override { client_ = nullptr; }

  virtual bool IsOnlineTest() { return GetParam(); }

  std::string GetTestCatalog() {
    return IsOnlineTest()
               ? CustomParameters::getArgument(kCatalog)
               : "hrn:here:data:::olp-cpp-sdk-ingestion-test-catalog";
  }

  std::string GetTestLayer() {
    return IsOnlineTest() ? CustomParameters::getArgument(kVolatileLayer)
                          : "olp-cpp-sdk-ingestion-test-volatile-layer";
  }

  virtual std::shared_ptr<VolatileLayerClient> CreateVolatileLayerClient() = 0;

 private:
  std::shared_ptr<std::vector<unsigned char>> GenerateData() {
    std::string test_suite_name(testing::UnitTest::GetInstance()
                                    ->current_test_info()
                                    ->test_suite_name());
    std::string test_name(
        testing::UnitTest::GetInstance()->current_test_info()->name());
    std::string data_string(test_suite_name + " " + test_name + " Payload");
    return std::make_shared<std::vector<unsigned char>>(data_string.begin(),
                                                        data_string.end());
  }

 protected:
  std::shared_ptr<VolatileLayerClient> client_;
  std::shared_ptr<std::vector<unsigned char>> data_;
};

class VolatileLayerClientOnlineTest : public VolatileLayerClientTestBase {
 protected:
  virtual std::shared_ptr<VolatileLayerClient> CreateVolatileLayerClient()
      override {
    olp::authentication::Settings settings;
    settings.token_endpoint_url = CustomParameters::getArgument(kEndpoint);

    olp::client::OlpClientSettings client_settings;
    client_settings.authentication_settings =
        (olp::client::AuthenticationSettings{
            olp::authentication::TokenProviderDefault{
                CustomParameters::getArgument(kAppid),
                CustomParameters::getArgument(kSecret), settings}});

    return std::make_shared<VolatileLayerClient>(
        olp::client::HRN{GetTestCatalog()}, client_settings);
  }
};

INSTANTIATE_TEST_SUITE_P(TestOnline, VolatileLayerClientOnlineTest,
                         ::testing::Values(true));

TEST_P(VolatileLayerClientOnlineTest, GetBaseVersionTest) {
  auto volatileClient = CreateVolatileLayerClient();
  auto response = volatileClient->GetBaseVersion().GetFuture().get();

  ASSERT_TRUE(response.IsSuccessful());
  auto versionResponse = response.GetResult();
  ASSERT_GE(versionResponse.GetVersion(), 0);
}

TEST_P(VolatileLayerClientOnlineTest, StartBatchInvalidTest) {
  auto volatileClient = CreateVolatileLayerClient();
  auto response =
      volatileClient->StartBatch(StartBatchRequest()).GetFuture().get();

  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_FALSE(response.GetResult().GetId());
  ASSERT_EQ(olp::client::ErrorCode::BadRequest,
            response.GetError().GetErrorCode());

  auto getBatchResponse =
      volatileClient->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_FALSE(getBatchResponse.IsSuccessful());

  auto completeBatchResponse =
      volatileClient->CompleteBatch(getBatchResponse.GetResult())
          .GetFuture()
          .get();
  ASSERT_FALSE(completeBatchResponse.IsSuccessful());
}

TEST_P(VolatileLayerClientOnlineTest, StartBatchTest) {
  auto volatileClient = CreateVolatileLayerClient();
  auto response =
      volatileClient
          ->StartBatch(StartBatchRequest().WithLayers({GetTestLayer()}))
          .GetFuture()
          .get();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult().GetId());
  ASSERT_NE("", response.GetResult().GetId().value());

  auto getBatchResponse =
      volatileClient->GetBatch(response.GetResult()).GetFuture().get();

  ASSERT_TRUE(getBatchResponse.IsSuccessful());
  ASSERT_EQ(response.GetResult().GetId().value(),
            getBatchResponse.GetResult().GetId().value());
  ASSERT_EQ("initialized",
            getBatchResponse.GetResult().GetDetails()->GetState());

  auto completeBatchResponse =
      volatileClient->CompleteBatch(getBatchResponse.GetResult())
          .GetFuture()
          .get();
  ASSERT_TRUE(completeBatchResponse.IsSuccessful());

  for (int i = 0; i < 100; ++i) {
    getBatchResponse =
        volatileClient->GetBatch(response.GetResult()).GetFuture().get();

    ASSERT_TRUE(getBatchResponse.IsSuccessful());
    ASSERT_EQ(response.GetResult().GetId().value(),
              getBatchResponse.GetResult().GetId().value());
    if (getBatchResponse.GetResult().GetDetails()->GetState() != "succeeded") {
      ASSERT_EQ("submitted",
                getBatchResponse.GetResult().GetDetails()->GetState());
    } else {
      break;
    }
  }
  ASSERT_EQ("succeeded", getBatchResponse.GetResult().GetDetails()->GetState());
}

TEST_P(VolatileLayerClientOnlineTest, PublishToBatchTest) {
  auto volatileClient = CreateVolatileLayerClient();
  auto response =
      volatileClient
          ->StartBatch(StartBatchRequest().WithLayers({GetTestLayer()}))
          .GetFuture()
          .get();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult().GetId());
  ASSERT_NE("", response.GetResult().GetId().value());

  std::vector<PublishPartitionDataRequest> partition_requests;
  PublishPartitionDataRequest partition_request;
  partition_requests.push_back(
      partition_request.WithLayerId(GetTestLayer()).WithPartitionId("123"));
  partition_requests.push_back(partition_request.WithPartitionId("456"));

  auto publishToBatchResponse =
      volatileClient->PublishToBatch(response.GetResult(), partition_requests)
          .GetFuture()
          .get();
  ASSERT_TRUE(publishToBatchResponse.IsSuccessful());

  auto completeBatchResponse =
      volatileClient->CompleteBatch(response.GetResult()).GetFuture().get();
  ASSERT_TRUE(completeBatchResponse.IsSuccessful());

  GetBatchResponse getBatchResponse;
  for (int i = 0; i < 100; ++i) {
    getBatchResponse =
        volatileClient->GetBatch(response.GetResult()).GetFuture().get();

    ASSERT_TRUE(getBatchResponse.IsSuccessful());
    ASSERT_EQ(response.GetResult().GetId().value(),
              getBatchResponse.GetResult().GetId().value());
    if (getBatchResponse.GetResult().GetDetails()->GetState() != "succeeded") {
      ASSERT_EQ("submitted",
                getBatchResponse.GetResult().GetDetails()->GetState());
    } else {
      break;
    }
  }
  ASSERT_EQ("succeeded", getBatchResponse.GetResult().GetDetails()->GetState());
}

TEST_P(VolatileLayerClientOnlineTest, PublishToBatchInvalidTest) {
  auto volatileClient = CreateVolatileLayerClient();
  auto response =
      volatileClient
          ->StartBatch(StartBatchRequest().WithLayers({GetTestLayer()}))
          .GetFuture()
          .get();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult().GetId());
  ASSERT_NE("", response.GetResult().GetId().value());

  auto publishToBatchResponse =
      volatileClient->PublishToBatch(response.GetResult(), {})
          .GetFuture()
          .get();
  ASSERT_FALSE(publishToBatchResponse.IsSuccessful());

  std::vector<PublishPartitionDataRequest> partition_requests{
      PublishPartitionDataRequest{}, PublishPartitionDataRequest{}};

  publishToBatchResponse =
      volatileClient->PublishToBatch(response.GetResult(), partition_requests)
          .GetFuture()
          .get();
  ASSERT_FALSE(publishToBatchResponse.IsSuccessful());

  partition_requests.clear();
  PublishPartitionDataRequest partition_request;
  partition_requests.push_back(
      partition_request.WithLayerId("foo").WithPartitionId("123"));
  partition_requests.push_back(
      partition_request.WithLayerId("bar").WithPartitionId("456"));

  publishToBatchResponse =
      volatileClient->PublishToBatch(response.GetResult(), partition_requests)
          .GetFuture()
          .get();
  ASSERT_FALSE(publishToBatchResponse.IsSuccessful());
}

TEST_P(VolatileLayerClientOnlineTest, StartBatchDeleteClientTest) {
  auto volatileClient = CreateVolatileLayerClient();
  auto response =
      volatileClient
          ->StartBatch(StartBatchRequest().WithLayers({GetTestLayer()}))
          .GetFuture()
          .get();

  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult().GetId());
  ASSERT_NE("", response.GetResult().GetId().value());

  auto getBatchFuture =
      volatileClient->GetBatch(response.GetResult()).GetFuture();

  volatileClient = nullptr;

  auto getBatchResponse = getBatchFuture.get();
  ASSERT_TRUE(getBatchResponse.IsSuccessful());
  ASSERT_EQ(response.GetResult().GetId().value(),
            getBatchResponse.GetResult().GetId().value());
  ASSERT_EQ("initialized",
            getBatchResponse.GetResult().GetDetails()->GetState());

  volatileClient = CreateVolatileLayerClient();

  auto completeBatchResponse =
      volatileClient->CompleteBatch(getBatchResponse.GetResult())
          .GetFuture()
          .get();
  ASSERT_TRUE(completeBatchResponse.IsSuccessful());

  for (int i = 0; i < 100; ++i) {
    getBatchResponse =
        volatileClient->GetBatch(response.GetResult()).GetFuture().get();

    ASSERT_TRUE(getBatchResponse.IsSuccessful());
    ASSERT_EQ(response.GetResult().GetId().value(),
              getBatchResponse.GetResult().GetId().value());
    if (getBatchResponse.GetResult().GetDetails()->GetState() != "succeeded") {
      ASSERT_EQ("submitted",
                getBatchResponse.GetResult().GetDetails()->GetState());
    } else {
      break;
    }
  }
  ASSERT_EQ("succeeded", getBatchResponse.GetResult().GetDetails()->GetState());
}

TEST_P(VolatileLayerClientOnlineTest, cancellAllRequestsTest) {
  auto volatileClient = CreateVolatileLayerClient();
  auto future = volatileClient->GetBaseVersion().GetFuture();

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  volatileClient->cancellAll();

  auto response = future.get();
  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_P(VolatileLayerClientOnlineTest, PublishData) {
  auto response = client_
                      ->PublishPartitionData(PublishPartitionDataRequest()
                                                 .WithData(data_)
                                                 .WithLayerId(GetTestLayer())
                                                 .WithPartitionId("123"))
                      .GetFuture()
                      .get();
  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
}

TEST_P(VolatileLayerClientOnlineTest, PublishDataAsync) {
  std::promise<PublishPartitionDataResponse> response_promise;
  bool call_is_async = true;

  auto cancel_token = client_->PublishPartitionData(
      PublishPartitionDataRequest()
          .WithData(data_)
          .WithLayerId(GetTestLayer())
          .WithPartitionId("456"),
      [&](const PublishPartitionDataResponse& response) {
        call_is_async = false;
        response_promise.set_value(response);
      });

  EXPECT_TRUE(call_is_async);
  auto response_future = response_promise.get_future();
  auto status = response_future.wait_for(std::chrono::seconds(30));
  if (status != std::future_status::ready) {
    cancel_token.cancel();
  }
  auto response = response_future.get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
}

class MockHandler {
 public:
  MOCK_METHOD3(CallOperator,
               olp::client::CancellationToken(
                   const olp::network::NetworkRequest& request,
                   const olp::network::NetworkConfig& config,
                   const olp::client::NetworkAsyncCallback& callback));

  olp::client::CancellationToken operator()(
      const olp::network::NetworkRequest& request,
      const olp::network::NetworkConfig& config,
      const olp::client::NetworkAsyncCallback& callback) {
    return CallOperator(request, config, callback);
  }
};

MATCHER_P(IsGetRequest, url, "") {
  return olp::network::NetworkRequest::HttpVerb::GET == arg.Verb() &&
         url == arg.Url() && (!arg.Content() || arg.Content()->empty());
}

MATCHER_P(IsPostRequest, url, "") {
  return olp::network::NetworkRequest::HttpVerb::POST == arg.Verb() &&
         url == arg.Url();
}

MATCHER_P(IsPutRequest, url, "") {
  return olp::network::NetworkRequest::HttpVerb::PUT == arg.Verb() &&
         url == arg.Url();
}

MATCHER_P(IsPutRequestPrefix, url, "") {
  if (olp::network::NetworkRequest::HttpVerb::PUT != arg.Verb()) {
    return false;
  }

  std::string url_string(url);
  auto res =
      std::mismatch(url_string.begin(), url_string.end(), arg.Url().begin());

  return (res.first == url_string.end());
}

olp::client::NetworkAsyncHandler volatileReturnsResponse(
    olp::network::HttpResponse response) {
  return [=](const olp::network::NetworkRequest& request,
             const olp::network::NetworkConfig& config,
             const olp::client::NetworkAsyncCallback& callback)
             -> olp::client::CancellationToken {
    std::thread([=]() { callback(response); }).detach();
    return olp::client::CancellationToken();
  };
}

olp::client::NetworkAsyncHandler volatileSetsPromiseWaitsAndReturns(
    std::shared_ptr<std::promise<void>> preSignal,
    std::shared_ptr<std::promise<void>> waitForSignal,
    olp::network::HttpResponse response) {
  return [preSignal, waitForSignal, response](
             const olp::network::NetworkRequest& request,
             const olp::network::NetworkConfig& /*config*/,
             const olp::client::NetworkAsyncCallback& callback)
             -> olp::client::CancellationToken {
    auto completed = std::make_shared<std::atomic_bool>(false);

    std::thread(
        [request, preSignal, waitForSignal, completed, callback, response]() {
          preSignal->set_value();
          waitForSignal->get_future().get();

          if (!completed->exchange(true)) {
            callback(response);
          }
        })
        .detach();

    return olp::client::CancellationToken([request, completed, callback]() {
      if (!completed->exchange(true)) {
        callback({olp::network::Network::ErrorCode::Cancelled, "Cancelled"});
      }
    });
  };
}

class VolatileLayerClientMockTest : public VolatileLayerClientTestBase {
 protected:
  MockHandler handler_;

  virtual std::shared_ptr<VolatileLayerClient> CreateVolatileLayerClient()
      override {
    olp::client::OlpClientSettings client_settings;
    auto handle = [&](const olp::network::NetworkRequest& request,
                      const olp::network::NetworkConfig& config,
                      const olp::client::NetworkAsyncCallback& callback)
        -> olp::client::CancellationToken {
      return handler_(request, config, callback);
    };
    client_settings.network_async_handler = handle;
    SetUpCommonNetworkMockCalls();

    return std::make_shared<VolatileLayerClient>(
        olp::client::HRN{GetTestCatalog()}, client_settings);
  }

  void SetUpCommonNetworkMockCalls() {
    // Catch unexpected calls and fail immediatley
    ON_CALL(handler_, CallOperator(testing::_, testing::_, testing::_))
        .WillByDefault(
            testing::DoAll(testing::InvokeWithoutArgs([]() { FAIL(); }),
                           testing::Invoke(volatileReturnsResponse({-1, ""}))));

    ON_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG), testing::_,
                                   testing::_))
        .WillByDefault(testing::Invoke(
            volatileReturnsResponse({200, HTTP_RESPONSE_LOOKUP_CONFIG})));

    ON_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_METADATA),
                                   testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            volatileReturnsResponse({200, HTTP_RESPONSE_LOOKUP_METADATA})));

    ON_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_VOLATILE_BLOB),
                                   testing::_, testing::_))
        .WillByDefault(testing::Invoke(volatileReturnsResponse(
            {200, HTTP_RESPONSE_LOOKUP_VOLATILE_BLOB})));

    ON_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_QUERY), testing::_,
                                   testing::_))
        .WillByDefault(testing::Invoke(
            volatileReturnsResponse({200, HTTP_RESPONSE_LOOKUP_QUERY})));

    ON_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_PUBLISH_V2),
                                   testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            volatileReturnsResponse({200, HTTP_RESPONSE_LOOKUP_PUBLISH_V2})));

    ON_CALL(handler_,
            CallOperator(IsGetRequest(URL_GET_CATALOG), testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            volatileReturnsResponse({200, HTTP_RESPONSE_GET_CATALOG})));

    ON_CALL(handler_, CallOperator(IsGetRequest(URL_QUERY_PARTITION_1111),
                                   testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            volatileReturnsResponse({200, HTTP_RESPONSE_QUERY_DATA_HANDLE})));

    ON_CALL(handler_,
            CallOperator(IsPutRequestPrefix(URL_PUT_VOLATILE_BLOB_PREFIX),
                         testing::_, testing::_))
        .WillByDefault(testing::Invoke(volatileReturnsResponse({200, ""})));
  }
};

INSTANTIATE_TEST_SUITE_P(TestMock, VolatileLayerClientMockTest,
                         ::testing::Values(false));

TEST_P(VolatileLayerClientMockTest, PublishData) {
  {
    testing::InSequence dummy;

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_METADATA),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_VOLATILE_BLOB),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_QUERY),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_PUBLISH_V2),
                                       testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG),
                                       testing::_, testing::_))
        .Times(1);

    EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_QUERY_PARTITION_1111),
                                       testing::_, testing::_))
        .Times(1);

    EXPECT_CALL(handler_,
                CallOperator(IsPutRequestPrefix(URL_PUT_VOLATILE_BLOB_PREFIX),
                             testing::_, testing::_))
        .Times(1);
  }

  auto new_client = CreateVolatileLayerClient();
  auto response = new_client
                      ->PublishPartitionData(PublishPartitionDataRequest()
                                                 .WithData(data_)
                                                 .WithLayerId(GetTestLayer())
                                                 .WithPartitionId("1111"))
                      .GetFuture()
                      .get();

  ASSERT_NO_FATAL_FAILURE(PublishDataSuccessAssertions(response));
}

TEST_P(VolatileLayerClientMockTest, PublishDataCancelConfig) {
  auto waitForCancel = std::make_shared<std::promise<void>>();
  auto pauseForCancel = std::make_shared<std::promise<void>>();

  // Setup the expected calls :
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                     testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(volatileSetsPromiseWaitsAndReturns(
          waitForCancel, pauseForCancel, {200, HTTP_RESPONSE_LOOKUP_CONFIG})));
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_VOLATILE_BLOB),
                                     testing::_, testing::_))
      .Times(0);
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG), testing::_,
                                     testing::_))
      .Times(0);

  auto promise = client_->PublishPartitionData(PublishPartitionDataRequest()
                                                   .WithData(data_)
                                                   .WithLayerId(GetTestLayer())
                                                   .WithPartitionId("1111"));
  waitForCancel->get_future().get();
  promise.GetCancellationToken().cancel();
  pauseForCancel->set_value();

  auto response = promise.GetFuture().get();

  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_EQ(olp::network::Network::ErrorCode::Cancelled,
            static_cast<int>(response.GetError().GetHttpStatusCode()));
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_P(VolatileLayerClientMockTest, PublishDataCancelBlob) {
  auto waitForCancel = std::make_shared<std::promise<void>>();
  auto pauseForCancel = std::make_shared<std::promise<void>>();

  // Setup the expected calls :
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                     testing::_, testing::_))
      .Times(1);

  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_METADATA),
                                     testing::_, testing::_))
      .Times(1);
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_VOLATILE_BLOB),
                                     testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(volatileSetsPromiseWaitsAndReturns(
          waitForCancel, pauseForCancel,
          {200, HTTP_RESPONSE_LOOKUP_VOLATILE_BLOB})));
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG), testing::_,
                                     testing::_))
      .Times(0);

  auto promise = client_->PublishPartitionData(PublishPartitionDataRequest()
                                                   .WithData(data_)
                                                   .WithLayerId(GetTestLayer())
                                                   .WithPartitionId("1111"));
  waitForCancel->get_future().get();
  promise.GetCancellationToken().cancel();
  pauseForCancel->set_value();

  auto response = promise.GetFuture().get();

  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_EQ(olp::network::Network::ErrorCode::Cancelled,
            static_cast<int>(response.GetError().GetHttpStatusCode()));
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_P(VolatileLayerClientMockTest, PublishDataCancelCatalog) {
  auto waitForCancel = std::make_shared<std::promise<void>>();
  auto pauseForCancel = std::make_shared<std::promise<void>>();

  // Setup the expected calls :
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_CONFIG),
                                     testing::_, testing::_))
      .Times(1);
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_METADATA),
                                     testing::_, testing::_))
      .Times(1);
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_VOLATILE_BLOB),
                                     testing::_, testing::_))
      .Times(1);
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_QUERY), testing::_,
                                     testing::_))
      .Times(1);
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_LOOKUP_PUBLISH_V2),
                                     testing::_, testing::_))
      .Times(1);
  EXPECT_CALL(handler_, CallOperator(IsGetRequest(URL_GET_CATALOG), testing::_,
                                     testing::_))
      .Times(1)
      .WillOnce(testing::Invoke(volatileSetsPromiseWaitsAndReturns(
          waitForCancel, pauseForCancel, {200, HTTP_RESPONSE_GET_CATALOG})));

  auto promise = client_->PublishPartitionData(PublishPartitionDataRequest()
                                                   .WithData(data_)
                                                   .WithLayerId(GetTestLayer())
                                                   .WithPartitionId("1111"));
  waitForCancel->get_future().get();
  promise.GetCancellationToken().cancel();
  pauseForCancel->set_value();

  auto response = promise.GetFuture().get();

  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_EQ(olp::network::Network::ErrorCode::Cancelled,
            static_cast<int>(response.GetError().GetHttpStatusCode()));
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}
