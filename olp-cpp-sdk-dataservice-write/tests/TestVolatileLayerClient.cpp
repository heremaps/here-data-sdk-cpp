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
#include <olp/core/client/HttpResponse.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientSettingsFactory.h>

#include <olp/dataservice/write/VolatileLayerClient.h>
#include <olp/dataservice/write/model/PublishPartitionDataRequest.h>

#include "testutils/CustomParameters.hpp"

#include "HttpResponses.h"

#include <matchers/NetworkUrlMatchers.h>
#include <mocks/NetworkMock.h>

using namespace olp::dataservice::write;
using namespace olp::dataservice::write::model;

const std::string kEndpoint = "endpoint";
const std::string kAppid = "dataservice_write_test_appid";
const std::string kSecret = "dataservice_write_test_secret";
const std::string kCatalog = "dataservice_write_test_catalog";
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

  virtual void TearDown() override {
    data_.reset();
    client_.reset();
  }

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
  static std::shared_ptr<olp::http::Network> s_network;

  static void SetUpTestSuite() {
    s_network = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();
  }

  virtual std::shared_ptr<VolatileLayerClient> CreateVolatileLayerClient()
      override {
    auto network = s_network;

    olp::authentication::Settings authentication_settings;
    authentication_settings.token_endpoint_url =
        CustomParameters::getArgument(kEndpoint);
    authentication_settings.network_request_handler = network;

    olp::authentication::TokenProviderDefault provider(
        CustomParameters::getArgument(kAppid),
        CustomParameters::getArgument(kSecret), authentication_settings);

    olp::client::AuthenticationSettings auth_client_settings;
    auth_client_settings.provider = provider;

    olp::client::OlpClientSettings settings;
    settings.authentication_settings = auth_client_settings;
    settings.network_request_handler = network;

    return std::make_shared<VolatileLayerClient>(
        olp::client::HRN{GetTestCatalog()}, settings);
  }
};

// Static network instance is necessary as it needs to outlive any created
// clients. This is a known limitation as triggered send requests capture the
// network instance inside the callbacks.
std::shared_ptr<olp::http::Network> VolatileLayerClientOnlineTest::s_network;

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
  // There are can be a case that GetBatch() is not "succeeded", but in
  // "submitted" state even after 100 iterations,
  // which actually means that there are might be a problem on the server side,
  // (or just long delay). Thus, better to rewrite this test, or do not rely on
  // the real server, but use mocked server.
  // ASSERT_EQ("succeeded",
  // getBatchResponse.GetResult().GetDetails()->GetState());
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
  // There are can be a case that GetBatch() is not "succeeded", but in
  // "submitted" state even after 100 iterations,
  // which actually means that there are might be a problem on the server side,
  // (or just long delay). Thus, better to rewrite this test, or do not rely on
  // the real server, but use mocked server.
  // ASSERT_EQ("succeeded",
  // getBatchResponse.GetResult().GetDetails()->GetState());
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

// Sometimes we receive the 500 internal server error,
// thus looks loke the problem is on the server side.
// Please, re-enable this test when switched to mocked server or
// when the server will be more steady for testing
TEST_P(VolatileLayerClientOnlineTest, DISABLED_StartBatchDeleteClientTest) {
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
  // There are can be a case that GetBatch() is not "succeeded", but in
  // "submitted" state even after 100 iterations,
  // which actually means that there are might be a problem on the server side,
  // (or just long delay). Thus, better to rewrite this test, or do not rely on
  // the real server, but use mocked server.
  // ASSERT_EQ("succeeded",
  // getBatchResponse.GetResult().GetDetails()->GetState());
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

#if 0
olp::client::NetworkAsyncHandler volatileSetsPromiseWaitsAndReturns(
    std::shared_ptr<std::promise<void>> preSignal,
    std::shared_ptr<std::promise<void>> waitForSignal,
    olp::client::HttpResponse response) {
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
#endif

using testing::_;

class VolatileLayerClientMockTest : public VolatileLayerClientTestBase {
 protected:
  std::shared_ptr<NetworkMock> network_;

  void TearDown() override {
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  virtual std::shared_ptr<VolatileLayerClient> CreateVolatileLayerClient()
      override {
    olp::client::OlpClientSettings client_settings;
    network_ = std::make_shared<NetworkMock>();
    client_settings.network_request_handler = network_;
    SetUpCommonNetworkMockCalls(*network_);

    return std::make_shared<VolatileLayerClient>(
        olp::client::HRN{GetTestCatalog()}, client_settings);
  }

  void SetUpCommonNetworkMockCalls(NetworkMock& network) {
    // Catch unexpected calls and fail immediatley
    ON_CALL(network, Send(_, _, _, _, _))
        .WillByDefault(testing::DoAll(
            NetworkMock::ReturnHttpResponse(
                olp::http::NetworkResponse().WithStatus(-1), ""),
            [](olp::http::NetworkRequest request,
               olp::http::Network::Payload payload,
               olp::http::Network::Callback callback,
               olp::http::Network::HeaderCallback header_callback,
               olp::http::Network::DataCallback data_callback) {
              auto fail_helper = []() { FAIL(); };
              fail_helper();
              return olp::http::SendOutcome(5);
            }));

    ON_CALL(network, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
        .WillByDefault(NetworkMock::ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(200),
            HTTP_RESPONSE_LOOKUP_CONFIG));

    ON_CALL(network, Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
        .WillByDefault(NetworkMock::ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(200),
            HTTP_RESPONSE_LOOKUP_METADATA));

    ON_CALL(network, Send(IsGetRequest(URL_LOOKUP_VOLATILE_BLOB), _, _, _, _))
        .WillByDefault(NetworkMock::ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(200),
            HTTP_RESPONSE_LOOKUP_VOLATILE_BLOB));

    ON_CALL(network, Send(IsGetRequest(URL_LOOKUP_QUERY), _, _, _, _))
        .WillByDefault(NetworkMock::ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(200),
            HTTP_RESPONSE_LOOKUP_QUERY));

    ON_CALL(network, Send(IsGetRequest(URL_LOOKUP_PUBLISH_V2), _, _, _, _))
        .WillByDefault(NetworkMock::ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(200),
            HTTP_RESPONSE_LOOKUP_PUBLISH_V2));

    ON_CALL(network, Send(IsGetRequest(URL_GET_CATALOG), _, _, _, _))
        .WillByDefault(NetworkMock::ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(200),
            HTTP_RESPONSE_GET_CATALOG));

    ON_CALL(network, Send(IsGetRequest(URL_QUERY_PARTITION_1111), _, _, _, _))
        .WillByDefault(NetworkMock::ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(200),
            HTTP_RESPONSE_QUERY_DATA_HANDLE));

    ON_CALL(network,
            Send(IsPutRequestPrefix(URL_PUT_VOLATILE_BLOB_PREFIX), _, _, _, _))
        .WillByDefault(NetworkMock::ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(200), ""));
  }
};

INSTANTIATE_TEST_SUITE_P(TestMock, VolatileLayerClientMockTest,
                         ::testing::Values(false));

TEST_P(VolatileLayerClientMockTest, PublishData) {
  auto new_client = CreateVolatileLayerClient();
  {
    testing::InSequence dummy;

    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_,
                Send(IsGetRequest(URL_LOOKUP_VOLATILE_BLOB), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_QUERY), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_,
                Send(IsGetRequest(URL_LOOKUP_PUBLISH_V2), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_GET_CATALOG), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_,
                Send(IsGetRequest(URL_QUERY_PARTITION_1111), _, _, _, _))
        .Times(1);
    EXPECT_CALL(
        *network_,
        Send(IsPutRequestPrefix(URL_PUT_VOLATILE_BLOB_PREFIX), _, _, _, _))
        .Times(1);
  }
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
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_LOOKUP_CONFIG});

  {
    testing::InSequence s;

    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
        .Times(1)
        .WillOnce(testing::Invoke(std::move(send_mock)));
    EXPECT_CALL(*network_, Cancel(request_id))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));
    EXPECT_CALL(*network_,
                Send(IsGetRequest(URL_LOOKUP_VOLATILE_BLOB), _, _, _, _))
        .Times(0);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_GET_CATALOG), _, _, _, _))
        .Times(0);
  }
  auto promise = client_->PublishPartitionData(PublishPartitionDataRequest()
                                                   .WithData(data_)
                                                   .WithLayerId(GetTestLayer())
                                                   .WithPartitionId("1111"));
  wait_for_cancel->get_future().get();
  promise.GetCancellationToken().cancel();
  pause_for_cancel->set_value();

  auto response = promise.GetFuture().get();

  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_P(VolatileLayerClientMockTest, PublishDataCancelBlob) {
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) =
      GenerateNetworkMockActions(wait_for_cancel, pause_for_cancel,
                                 {200, HTTP_RESPONSE_LOOKUP_VOLATILE_BLOB});

  {
    testing::InSequence s;

    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_,
                Send(IsGetRequest(URL_LOOKUP_VOLATILE_BLOB), _, _, _, _))
        .Times(1)
        .WillOnce(testing::Invoke(std::move(send_mock)));
    EXPECT_CALL(*network_, Cancel(request_id))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_GET_CATALOG), _, _, _, _))
        .Times(0);
  }

  auto promise = client_->PublishPartitionData(PublishPartitionDataRequest()
                                                   .WithData(data_)
                                                   .WithLayerId(GetTestLayer())
                                                   .WithPartitionId("1111"));
  wait_for_cancel->get_future().get();
  promise.GetCancellationToken().cancel();
  pause_for_cancel->set_value();

  auto response = promise.GetFuture().get();

  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}

TEST_P(VolatileLayerClientMockTest, PublishDataCancelCatalog) {
  auto wait_for_cancel = std::make_shared<std::promise<void>>();
  auto pause_for_cancel = std::make_shared<std::promise<void>>();

  olp::http::RequestId request_id;
  NetworkCallback send_mock;
  CancelCallback cancel_mock;

  std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
      wait_for_cancel, pause_for_cancel, {200, HTTP_RESPONSE_GET_CATALOG});

  {
    testing::InSequence s;

    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_,
                Send(IsGetRequest(URL_LOOKUP_VOLATILE_BLOB), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_LOOKUP_QUERY), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_,
                Send(IsGetRequest(URL_LOOKUP_PUBLISH_V2), _, _, _, _))
        .Times(1);
    EXPECT_CALL(*network_, Send(IsGetRequest(URL_GET_CATALOG), _, _, _, _))
        .Times(1)
        .WillOnce(testing::Invoke(std::move(send_mock)));
    EXPECT_CALL(*network_, Cancel(request_id))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));
  }

  auto promise = client_->PublishPartitionData(PublishPartitionDataRequest()
                                                   .WithData(data_)
                                                   .WithLayerId(GetTestLayer())
                                                   .WithPartitionId("1111"));
  wait_for_cancel->get_future().get();
  promise.GetCancellationToken().cancel();
  pause_for_cancel->set_value();

  auto response = promise.GetFuture().get();

  ASSERT_FALSE(response.IsSuccessful());
  ASSERT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            response.GetError().GetHttpStatusCode());
  ASSERT_EQ(olp::client::ErrorCode::Cancelled,
            response.GetError().GetErrorCode());
}
