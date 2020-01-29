/*
 * Copyright (C) 2020 HERE Europe B.V.
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
#include <olp/core/client/Condition.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/dataservice/read/StreamLayerClient.h>

#include "HttpResponses.h"

namespace {

using namespace olp::client;
using namespace olp::dataservice::read;
using namespace olp::tests::common;
using namespace testing;

const std::string kCatalog =
    "hrn:here:data::olp-here-test:hereos-internal-test-v2";
const std::string kConsumerID = "consumer_id_1234";
const std::string kDataHandle = "4eed6ed1-0d32-43b9-ae79-043cb4256432";
const std::string kLayerId = "testlayer";
const std::string kSubscriptionId = "subscribe_id_12345";
const auto kTimeout = std::chrono::seconds(5);

class ReadStreamLayerClientTest : public Test {
 protected:
  ReadStreamLayerClientTest() = default;
  ~ReadStreamLayerClientTest() = default;
  std::string GetTestCatalog() { return kCatalog; }
  static std::string ApiErrorToString(const ApiError& error);

  void SetUp() override;
  void TearDown() override;

 private:
  void SetUpCommonNetworkMockCalls();

 protected:
  OlpClientSettings settings_;
  std::shared_ptr<NetworkMock> network_mock_;
};

std::string ReadStreamLayerClientTest::ApiErrorToString(const ApiError& error) {
  std::ostringstream result_stream;
  result_stream << "ERROR: code: " << static_cast<int>(error.GetErrorCode())
                << ", status: " << error.GetHttpStatusCode()
                << ", message: " << error.GetMessage();
  return result_stream.str();
}

void ReadStreamLayerClientTest::SetUp() {
  network_mock_ = std::make_shared<NetworkMock>();
  settings_ = OlpClientSettings();
  settings_.network_request_handler = network_mock_;
  settings_.task_scheduler =
      OlpClientSettingsFactory::CreateDefaultTaskScheduler(1u);

  SetUpCommonNetworkMockCalls();
}

void ReadStreamLayerClientTest::TearDown() {
  Mock::VerifyAndClearExpectations(network_mock_.get());
  network_mock_.reset();
}

void ReadStreamLayerClientTest::SetUpCommonNetworkMockCalls() {
  ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_STREAM), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_LOOKUP_STREAM));

  ON_CALL(*network_mock_,
          Send(IsPostRequest(URL_STREAM_SUBSCRIBE_SERIAL), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                 olp::http::HttpStatusCode::CREATED),
                             HTTP_RESPONSE_STREAM_LAYER_SUBSCRIPTION));

  ON_CALL(*network_mock_,
          Send(IsPostRequest(URL_STREAM_SUBSCRIBE_PARALLEL), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                 olp::http::HttpStatusCode::CREATED),
                             HTTP_RESPONSE_STREAM_LAYER_SUBSCRIPTION));

  ON_CALL(*network_mock_,
          Send(IsPostRequest(URL_STREAM_SUBSCRIBE_SUBSCRIPTION_ID), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                 olp::http::HttpStatusCode::CREATED),
                             HTTP_RESPONSE_STREAM_LAYER_SUBSCRIPTION));

  ON_CALL(*network_mock_,
          Send(IsPostRequest(URL_STREAM_SUBSCRIBE_CONSUMER_ID), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                 olp::http::HttpStatusCode::CREATED),
                             HTTP_RESPONSE_STREAM_LAYER_SUBSCRIPTION));

  ON_CALL(*network_mock_,
          Send(IsPostRequest(URL_STREAM_SUBSCRIBE_ALL_PARAMETERS), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                 olp::http::HttpStatusCode::CREATED),
                             HTTP_RESPONSE_STREAM_LAYER_SUBSCRIPTION));

  ON_CALL(*network_mock_,
          Send(IsDeleteRequest(URL_STREAM_UNSUBSCRIBE_SERIAL), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_EMPTY));

  ON_CALL(*network_mock_,
          Send(IsDeleteRequest(URL_STREAM_UNSUBSCRIBE_PARALLEL), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_EMPTY));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_LOOKUP_BLOB));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                             HTTP_RESPONSE_BLOB_DATA_STREAM_MESSAGE));

  // Catch any non-interesting network calls that don't need to be verified
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _)).Times(AtLeast(0));
}

TEST_F(ReadStreamLayerClientTest, Subscribe) {
  HRN hrn(GetTestCatalog());

  {
    SCOPED_TRACE("Subscribe succeeds, serial");

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_STREAM), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_,
                Send(IsPostRequest(URL_STREAM_SUBSCRIBE_SERIAL), _, _, _, _))
        .Times(1);

    StreamLayerClient client(hrn, kLayerId, settings_);

    SubscribeResponse subscribe_response;
    Condition condition;
    client.Subscribe(SubscribeRequest(), [&](SubscribeResponse response) {
      subscribe_response = std::move(response);
      condition.Notify();
    });

    ASSERT_TRUE(condition.Wait(kTimeout));

    ASSERT_TRUE(subscribe_response.IsSuccessful());

    EXPECT_EQ(kSubscriptionId, subscribe_response.GetResult().c_str());

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Subscribe succeeds, parallel");

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_STREAM), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_,
                Send(IsPostRequest(URL_STREAM_SUBSCRIBE_PARALLEL), _, _, _, _))
        .Times(1);

    StreamLayerClient client(hrn, kLayerId, settings_);

    SubscribeResponse subscribe_response;
    Condition condition;
    client.Subscribe(SubscribeRequest().WithSubscriptionMode(
                         SubscribeRequest::SubscriptionMode::kParallel),
                     [&](SubscribeResponse response) {
                       subscribe_response = std::move(response);
                       condition.Notify();
                     });

    ASSERT_TRUE(condition.Wait(kTimeout));

    ASSERT_TRUE(subscribe_response.IsSuccessful())
        << ApiErrorToString(subscribe_response.GetError());

    EXPECT_EQ(kSubscriptionId, subscribe_response.GetResult().c_str());

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Subscribe succeeds, with subscriptionID");

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_STREAM), _, _, _, _))
        .Times(1);

    EXPECT_CALL(
        *network_mock_,
        Send(IsPostRequest(URL_STREAM_SUBSCRIBE_SUBSCRIPTION_ID), _, _, _, _))
        .Times(1);

    StreamLayerClient client(hrn, kLayerId, settings_);

    SubscribeResponse subscribe_response;
    Condition condition;
    client.Subscribe(SubscribeRequest().WithSubscriptionId(kSubscriptionId),
                     [&](SubscribeResponse response) {
                       subscribe_response = std::move(response);
                       condition.Notify();
                     });

    ASSERT_TRUE(condition.Wait(kTimeout));

    ASSERT_TRUE(subscribe_response.IsSuccessful())
        << ApiErrorToString(subscribe_response.GetError());

    EXPECT_EQ(kSubscriptionId, subscribe_response.GetResult().c_str());

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Subscribe succeeds, with consumerID");

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_STREAM), _, _, _, _))
        .Times(1);

    EXPECT_CALL(
        *network_mock_,
        Send(IsPostRequest(URL_STREAM_SUBSCRIBE_CONSUMER_ID), _, _, _, _))
        .Times(1);

    StreamLayerClient client(hrn, kLayerId, settings_);

    SubscribeResponse subscribe_response;
    Condition condition;
    client.Subscribe(SubscribeRequest().WithConsumerId(kConsumerID),
                     [&](SubscribeResponse response) {
                       subscribe_response = std::move(response);
                       condition.Notify();
                     });

    ASSERT_TRUE(condition.Wait(kTimeout));

    ASSERT_TRUE(subscribe_response.IsSuccessful())
        << ApiErrorToString(subscribe_response.GetError());

    EXPECT_EQ(kSubscriptionId, subscribe_response.GetResult().c_str());

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Subscribe succeeds, multiple query parameters");

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_STREAM), _, _, _, _))
        .Times(1);

    EXPECT_CALL(
        *network_mock_,
        Send(IsPostRequest(URL_STREAM_SUBSCRIBE_ALL_PARAMETERS), _, _, _, _))
        .Times(1);

    StreamLayerClient client(hrn, kLayerId, settings_);

    SubscribeResponse subscribe_response;
    Condition condition;
    client.Subscribe(SubscribeRequest()
                         .WithConsumerId(kConsumerID)
                         .WithSubscriptionId(kSubscriptionId)
                         .WithSubscriptionMode(
                             SubscribeRequest::SubscriptionMode::kParallel),
                     [&](SubscribeResponse response) {
                       subscribe_response = std::move(response);
                       condition.Notify();
                     });

    ASSERT_TRUE(condition.Wait(kTimeout));

    ASSERT_TRUE(subscribe_response.IsSuccessful())
        << ApiErrorToString(subscribe_response.GetError());

    EXPECT_EQ(kSubscriptionId, subscribe_response.GetResult().c_str());

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Subscribe fails, incorrect request");

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_STREAM), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_,
                Send(IsPostRequest(URL_STREAM_SUBSCRIBE_SERIAL), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::FORBIDDEN),
                                     HTTP_RESPONSE_SUBSCRIBE_403));

    StreamLayerClient client(hrn, kLayerId, settings_);

    SubscribeResponse subscribe_response;
    Condition condition;
    client.Subscribe(SubscribeRequest(), [&](SubscribeResponse response) {
      subscribe_response = std::move(response);
      condition.Notify();
    });

    ASSERT_TRUE(condition.Wait(kTimeout));

    EXPECT_FALSE(subscribe_response.IsSuccessful());

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Subscribe fails, incorrect hrn");

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_STREAM), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::FORBIDDEN),
                                     HTTP_RESPONSE_403));

    StreamLayerClient client(hrn, kLayerId, settings_);

    SubscribeResponse subscribe_response;
    Condition condition;
    client.Subscribe(SubscribeRequest(), [&](SubscribeResponse response) {
      subscribe_response = std::move(response);
      condition.Notify();
    });

    ASSERT_TRUE(condition.Wait(kTimeout));

    EXPECT_FALSE(subscribe_response.IsSuccessful());

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Subscribe fails, incorrect layer");

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_STREAM), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_,
                Send(IsPostRequest(URL_STREAM_SUBSCRIBE_SERIAL), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::NOT_FOUND),
                                     HTTP_RESPONSE_SUBSCRIBE_404));

    StreamLayerClient client(hrn, kLayerId, settings_);

    SubscribeResponse subscribe_response;
    Condition condition;
    client.Subscribe(SubscribeRequest(), [&](SubscribeResponse response) {
      subscribe_response = std::move(response);
      condition.Notify();
    });

    ASSERT_TRUE(condition.Wait(kTimeout));

    EXPECT_FALSE(subscribe_response.IsSuccessful());

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(ReadStreamLayerClientTest, SubscribeCancellableFuture) {
  HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_STREAM), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_,
              Send(IsPostRequest(URL_STREAM_SUBSCRIBE_SERIAL), _, _, _, _))
      .Times(1);

  StreamLayerClient client(hrn, kLayerId, settings_);

  auto future = client.Subscribe(SubscribeRequest()).GetFuture();

  ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

  const auto& response = future.get();
  ASSERT_TRUE(response.IsSuccessful());

  EXPECT_EQ(response.GetResult(), kSubscriptionId);
}

TEST_F(ReadStreamLayerClientTest, SubscribeApiLookup429) {
  HRN hrn(GetTestCatalog());
  {
    InSequence s;

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_STREAM), _, _, _, _))
        .Times(2)
        .WillRepeatedly(ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(
                olp::http::HttpStatusCode::TOO_MANY_REQUESTS),
            "Server busy at the moment."));

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_STREAM), _, _, _, _))
        .Times(1);
  }

  RetrySettings retry_settings;
  retry_settings.retry_condition = [](const HttpResponse& response) {
    return olp::http::HttpStatusCode::TOO_MANY_REQUESTS == response.status;
  };
  settings_.retry_settings = retry_settings;
  StreamLayerClient client(hrn, kLayerId, settings_);

  SubscribeResponse subscribe_response;
  Condition condition;
  client.Subscribe(SubscribeRequest(), [&](SubscribeResponse response) {
    subscribe_response = std::move(response);
    condition.Notify();
  });

  ASSERT_TRUE(condition.Wait(kTimeout));

  ASSERT_TRUE(subscribe_response.IsSuccessful())
      << ApiErrorToString(subscribe_response.GetError());

  EXPECT_EQ(kSubscriptionId, subscribe_response.GetResult().c_str());
}

TEST_F(ReadStreamLayerClientTest, SubscribeCancelFuture) {
  HRN hrn(GetTestCatalog());

  auto request_started = std::make_shared<std::promise<void>>();
  auto continue_request = std::make_shared<std::promise<void>>();

  {
    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    std::tie(request_id, send_mock, cancel_mock) =
        GenerateNetworkMockActions(request_started, continue_request,
                                   {olp::http::HttpStatusCode::OK,
                                    HTTP_RESPONSE_STREAM_LAYER_SUBSCRIPTION});

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_STREAM), _, _, _, _))
        .Times(1)
        .WillOnce(Invoke(std::move(send_mock)));

    EXPECT_CALL(*network_mock_, Cancel(request_id))
        .WillOnce(Invoke(std::move(cancel_mock)));
  }

  StreamLayerClient client(hrn, kLayerId, settings_);

  auto future = client.Subscribe(SubscribeRequest());

  request_started->get_future().get();
  future.GetCancellationToken().Cancel();
  continue_request->set_value();

  auto response = future.GetFuture().get();

  ASSERT_FALSE(response.IsSuccessful());

  EXPECT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            response.GetError().GetHttpStatusCode());
  EXPECT_EQ(ErrorCode::Cancelled, response.GetError().GetErrorCode());
}

TEST_F(ReadStreamLayerClientTest, Unsubscribe) {
  HRN hrn(GetTestCatalog());

  {
    SCOPED_TRACE("Unsubscribe succeeds, serial subscription");

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_STREAM), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_,
                Send(IsPostRequest(URL_STREAM_SUBSCRIBE_SERIAL), _, _, _, _))
        .Times(1);

    EXPECT_CALL(
        *network_mock_,
        Send(IsDeleteRequest(URL_STREAM_UNSUBSCRIBE_SERIAL), _, _, _, _))
        .Times(1);

    StreamLayerClient client(hrn, kLayerId, settings_);

    auto subscribe_future = client.Subscribe(SubscribeRequest()).GetFuture();
    ASSERT_EQ(subscribe_future.wait_for(kTimeout), std::future_status::ready);
    ASSERT_TRUE(subscribe_future.get().IsSuccessful());

    // Unsubscribe part
    UnsubscribeResponse unsubscribe_response;
    Condition condition;
    client.Unsubscribe([&](UnsubscribeResponse response) {
      unsubscribe_response = std::move(response);
      condition.Notify();
    });

    ASSERT_TRUE(condition.Wait(kTimeout));
    ASSERT_TRUE(unsubscribe_response.IsSuccessful());
    EXPECT_EQ(kSubscriptionId, unsubscribe_response.GetResult());

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Unsubscribe succeeds, parallel subscription");

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_STREAM), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_,
                Send(IsPostRequest(URL_STREAM_SUBSCRIBE_PARALLEL), _, _, _, _))
        .Times(1);

    EXPECT_CALL(
        *network_mock_,
        Send(IsDeleteRequest(URL_STREAM_UNSUBSCRIBE_PARALLEL), _, _, _, _))
        .Times(1);

    StreamLayerClient client(hrn, kLayerId, settings_);

    auto subscribe_future =
        client
            .Subscribe(SubscribeRequest().WithSubscriptionMode(
                SubscribeRequest::SubscriptionMode::kParallel))
            .GetFuture();
    ASSERT_EQ(subscribe_future.wait_for(kTimeout), std::future_status::ready);
    ASSERT_TRUE(subscribe_future.get().IsSuccessful());

    // Unsubscribe part
    UnsubscribeResponse unsubscribe_response;
    Condition condition;
    client.Unsubscribe([&](UnsubscribeResponse response) {
      unsubscribe_response = std::move(response);
      condition.Notify();
    });

    ASSERT_TRUE(condition.Wait(kTimeout));
    ASSERT_TRUE(unsubscribe_response.IsSuccessful());
    EXPECT_EQ(kSubscriptionId, unsubscribe_response.GetResult());

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE(
        "Unsubscribe succeeds, parallel subscription with provided ConsumerID "
        "and SubscriptionID");

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_STREAM), _, _, _, _))
        .Times(1);

    EXPECT_CALL(
        *network_mock_,
        Send(IsPostRequest(URL_STREAM_SUBSCRIBE_ALL_PARAMETERS), _, _, _, _))
        .Times(1);

    EXPECT_CALL(
        *network_mock_,
        Send(IsDeleteRequest(URL_STREAM_UNSUBSCRIBE_PARALLEL), _, _, _, _))
        .Times(1);

    StreamLayerClient client(hrn, kLayerId, settings_);

    auto subscribe_future =
        client
            .Subscribe(SubscribeRequest()
                           .WithConsumerId(kConsumerID)
                           .WithSubscriptionId(kSubscriptionId)
                           .WithSubscriptionMode(
                               SubscribeRequest::SubscriptionMode::kParallel))
            .GetFuture();
    ASSERT_EQ(subscribe_future.wait_for(kTimeout), std::future_status::ready);
    ASSERT_TRUE(subscribe_future.get().IsSuccessful());

    UnsubscribeResponse unsubscribe_response;
    Condition condition;
    client.Unsubscribe([&](UnsubscribeResponse response) {
      unsubscribe_response = std::move(response);
      condition.Notify();
    });

    ASSERT_TRUE(condition.Wait(kTimeout));
    ASSERT_TRUE(unsubscribe_response.IsSuccessful());
    EXPECT_EQ(kSubscriptionId, unsubscribe_response.GetResult());

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Unsubscribe fails, subscription missing");

    StreamLayerClient client(hrn, kLayerId, settings_);

    UnsubscribeResponse unsubscribe_response;
    Condition condition;
    client.Unsubscribe([&](UnsubscribeResponse response) {
      unsubscribe_response = std::move(response);
      condition.Notify();
    });

    ASSERT_TRUE(condition.Wait(kTimeout));
    ASSERT_FALSE(unsubscribe_response.IsSuccessful());

    EXPECT_EQ(unsubscribe_response.GetError().GetErrorCode(),
              ErrorCode::PreconditionFailed);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Unsubscribe fails, server error");

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_STREAM), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_,
                Send(IsPostRequest(URL_STREAM_SUBSCRIBE_SERIAL), _, _, _, _))
        .Times(1);

    EXPECT_CALL(
        *network_mock_,
        Send(IsDeleteRequest(URL_STREAM_UNSUBSCRIBE_SERIAL), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::NOT_FOUND),
                                     HTTP_RESPONSE_UNSUBSCRIBE_404));

    StreamLayerClient client(hrn, kLayerId, settings_);

    auto subscribe_future = client.Subscribe(SubscribeRequest()).GetFuture();
    ASSERT_EQ(subscribe_future.wait_for(kTimeout), std::future_status::ready);
    ASSERT_TRUE(subscribe_future.get().IsSuccessful());

    // Unsubscribe part
    UnsubscribeResponse unsubscribe_response;
    Condition condition;
    client.Unsubscribe([&](UnsubscribeResponse response) {
      unsubscribe_response = std::move(response);
      condition.Notify();
    });

    ASSERT_TRUE(condition.Wait(kTimeout));
    ASSERT_FALSE(unsubscribe_response.IsSuccessful());
    EXPECT_EQ(unsubscribe_response.GetError().GetErrorCode(),
              ErrorCode::NotFound);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(ReadStreamLayerClientTest, UnsubscribeCancellableFuture) {
  HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_STREAM), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_,
              Send(IsPostRequest(URL_STREAM_SUBSCRIBE_SERIAL), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_,
              Send(IsDeleteRequest(URL_STREAM_UNSUBSCRIBE_SERIAL), _, _, _, _))
      .Times(1);

  StreamLayerClient client(hrn, kLayerId, settings_);

  auto subscribe_future = client.Subscribe(SubscribeRequest()).GetFuture();
  ASSERT_EQ(subscribe_future.wait_for(kTimeout), std::future_status::ready);
  ASSERT_TRUE(subscribe_future.get().IsSuccessful());

  // Unsubscribe part
  UnsubscribeResponse unsubscribe_response;
  auto unsubscribe_future = client.Unsubscribe().GetFuture();

  ASSERT_EQ(unsubscribe_future.wait_for(kTimeout), std::future_status::ready);

  const auto& response = unsubscribe_future.get();
  ASSERT_TRUE(response.IsSuccessful());

  EXPECT_EQ(response.GetResult(), kSubscriptionId);
}

TEST_F(ReadStreamLayerClientTest, UnsubscribeCancelFuture) {
  HRN hrn(GetTestCatalog());

  auto request_started = std::make_shared<std::promise<void>>();
  auto continue_request = std::make_shared<std::promise<void>>();
  {
    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
        request_started, continue_request,
        {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_EMPTY});

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_LOOKUP_STREAM), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_,
                Send(IsPostRequest(URL_STREAM_SUBSCRIBE_SERIAL), _, _, _, _))
        .Times(1);

    EXPECT_CALL(
        *network_mock_,
        Send(IsDeleteRequest(URL_STREAM_UNSUBSCRIBE_SERIAL), _, _, _, _))
        .Times(1)
        .WillOnce(Invoke(std::move(send_mock)));

    EXPECT_CALL(*network_mock_, Cancel(request_id))
        .WillOnce(Invoke(std::move(cancel_mock)));
  }

  StreamLayerClient client(hrn, kLayerId, settings_);

  auto subscribe_future = client.Subscribe(SubscribeRequest()).GetFuture();
  ASSERT_EQ(subscribe_future.wait_for(kTimeout), std::future_status::ready);
  ASSERT_TRUE(subscribe_future.get().IsSuccessful());

  auto cancellable_future = client.Unsubscribe();

  request_started->get_future().get();
  cancellable_future.GetCancellationToken().Cancel();
  continue_request->set_value();

  auto unsubscribe_response = cancellable_future.GetFuture().get();

  ASSERT_FALSE(unsubscribe_response.IsSuccessful());

  EXPECT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            unsubscribe_response.GetError().GetHttpStatusCode());
  EXPECT_EQ(ErrorCode::Cancelled,
            unsubscribe_response.GetError().GetErrorCode());
}

TEST_F(ReadStreamLayerClientTest, GetData) {
  HRN hrn(GetTestCatalog());

  {
    SCOPED_TRACE("GetData success");

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .Times(1);

    StreamLayerClient client(hrn, kLayerId, settings_);

    std::promise<DataResponse> promise;
    auto future = promise.get_future();

    model::Metadata metadata;
    metadata.SetDataHandle(kDataHandle);
    model::Message message;
    message.SetMetaData(metadata);

    client.GetData(message,
                   [&](DataResponse response) { promise.set_value(response); });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_TRUE(response.IsSuccessful());
    ASSERT_TRUE(response.GetResult());

    const std::string blob_data = HTTP_RESPONSE_BLOB_DATA_STREAM_MESSAGE;
    EXPECT_THAT(*response.GetResult(),
                ElementsAreArray(blob_data.begin(), blob_data.end()));

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("GetData fails, no data handle");

    StreamLayerClient client(hrn, kLayerId, settings_);

    std::promise<DataResponse> promise;
    auto future = promise.get_future();

    client.GetData(model::Message{},
                   [&](DataResponse response) { promise.set_value(response); });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    ASSERT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::InvalidArgument);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("GetData fails, lookup server error");

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(
                olp::http::HttpStatusCode::AUTHENTICATION_TIMEOUT),
            HTTP_RESPONSE_EMPTY));

    StreamLayerClient client(hrn, kLayerId, settings_);

    std::promise<DataResponse> promise;
    auto future = promise.get_future();

    model::Metadata metadata;
    metadata.SetDataHandle(kDataHandle);
    model::Message message;
    message.SetMetaData(metadata);

    client.GetData(message,
                   [&](DataResponse response) { promise.set_value(response); });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetHttpStatusCode(),
              olp::http::HttpStatusCode::AUTHENTICATION_TIMEOUT);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("GetData fails, blob server error");

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::NOT_FOUND),
                                     HTTP_RESPONSE_EMPTY));

    StreamLayerClient client(hrn, kLayerId, settings_);

    std::promise<DataResponse> promise;
    auto future = promise.get_future();

    model::Metadata metadata;
    metadata.SetDataHandle(kDataHandle);
    model::Message message;
    message.SetMetaData(metadata);

    client.GetData(message,
                   [&](DataResponse response) { promise.set_value(response); });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    ASSERT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetHttpStatusCode(),
              olp::http::HttpStatusCode::NOT_FOUND);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(ReadStreamLayerClientTest, GetDataCancellableFuture) {
  HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .Times(1);

  StreamLayerClient client(hrn, kLayerId, settings_);

  model::Metadata metadata;
  metadata.SetDataHandle(kDataHandle);
  model::Message message;
  message.SetMetaData(metadata);

  auto future = client.GetData(message).GetFuture();

  ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

  const auto& response = future.get();
  EXPECT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult());

  const std::string blob_data = HTTP_RESPONSE_BLOB_DATA_STREAM_MESSAGE;
  EXPECT_THAT(*response.GetResult(),
              ElementsAreArray(blob_data.begin(), blob_data.end()));
}

TEST_F(ReadStreamLayerClientTest, GetDataCancel) {
  HRN hrn(GetTestCatalog());

  auto request_started = std::make_shared<std::promise<void>>();
  auto continue_request = std::make_shared<std::promise<void>>();

  {
    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
        request_started, continue_request,
        {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP_BLOB});

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
        .Times(1)
        .WillOnce(Invoke(std::move(send_mock)));

    EXPECT_CALL(*network_mock_, Cancel(request_id))
        .WillOnce(Invoke(std::move(cancel_mock)));
  }

  StreamLayerClient client(hrn, kLayerId, settings_);

  model::Metadata metadata;
  metadata.SetDataHandle(kDataHandle);
  model::Message message;
  message.SetMetaData(metadata);

  auto future = client.GetData(message);

  request_started->get_future().get();
  future.GetCancellationToken().Cancel();
  continue_request->set_value();

  auto response = future.GetFuture().get();

  ASSERT_FALSE(response.IsSuccessful());

  EXPECT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            response.GetError().GetHttpStatusCode());
  EXPECT_EQ(ErrorCode::Cancelled, response.GetError().GetErrorCode());
}

TEST_F(ReadStreamLayerClientTest, CancelPendingRequests) {
  HRN hrn(GetTestCatalog());

  // Simulate a loaded queue
  std::promise<void> promise;
  auto future = promise.get_future();
  settings_.task_scheduler->ScheduleTask([&future]() { future.get(); });

  StreamLayerClient client(hrn, kLayerId, settings_);

  auto subscribe_future = client.Subscribe(SubscribeRequest());
  auto get_data_future = client.GetData(model::Message());
  auto unsubscribe_future = client.Unsubscribe();

  client.CancelPendingRequests();

  promise.set_value();

  auto subscribe_response = subscribe_future.GetFuture().get();
  ASSERT_FALSE(subscribe_response.IsSuccessful());
  EXPECT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            subscribe_response.GetError().GetHttpStatusCode());
  EXPECT_EQ(ErrorCode::Cancelled, subscribe_response.GetError().GetErrorCode());

  auto get_data_response = get_data_future.GetFuture().get();
  ASSERT_FALSE(get_data_response.IsSuccessful());
  EXPECT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            get_data_response.GetError().GetHttpStatusCode());
  EXPECT_EQ(ErrorCode::Cancelled, get_data_response.GetError().GetErrorCode());

  auto unsubscribe_response = unsubscribe_future.GetFuture().get();
  ASSERT_FALSE(unsubscribe_response.IsSuccessful());
  EXPECT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            unsubscribe_response.GetError().GetHttpStatusCode());
  EXPECT_EQ(ErrorCode::Cancelled,
            unsubscribe_response.GetError().GetErrorCode());
}

}  // namespace
