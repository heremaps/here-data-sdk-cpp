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
