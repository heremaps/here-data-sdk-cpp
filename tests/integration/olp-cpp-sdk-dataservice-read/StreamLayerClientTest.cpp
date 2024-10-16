/*
 * Copyright (C) 2020-2024 HERE Europe B.V.
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
namespace client = olp::client;
namespace read = olp::dataservice::read;
namespace model = olp::dataservice::read::model;
using testing::_;

const std::string kCatalog =
    "hrn:here:data::olp-here-test:hereos-internal-test-v2";
const std::string kConsumerID = "consumer_id_1234";
const std::string kDataHandle = "4eed6ed1-0d32-43b9-ae79-043cb4256432";
const std::string kLayerId = "testlayer";
const std::string kSubscriptionId = "subscribe_id_12345";
const auto kTimeout = std::chrono::seconds(5);

class ReadStreamLayerClientTest : public testing::Test {
 protected:
  ReadStreamLayerClientTest() = default;
  ~ReadStreamLayerClientTest() = default;
  std::string GetTestCatalog() { return kCatalog; }
  static std::string ApiErrorToString(const client::ApiError& error);
  static model::StreamOffsets GetStreamOffsets();

  void SetUp() override;
  void TearDown() override;

 private:
  void SetUpCommonNetworkMockCalls();

 protected:
  client::OlpClientSettings settings_;
  std::shared_ptr<NetworkMock> network_mock_;
};

std::string ReadStreamLayerClientTest::ApiErrorToString(
    const client::ApiError& error) {
  std::ostringstream result_stream;
  result_stream << "ERROR: code: " << static_cast<int>(error.GetErrorCode())
                << ", status: " << error.GetHttpStatusCode()
                << ", message: " << error.GetMessage();
  return result_stream.str();
}

model::StreamOffsets ReadStreamLayerClientTest::GetStreamOffsets() {
  model::StreamOffset offset1;
  offset1.SetPartition(7);
  offset1.SetOffset(38562);
  model::StreamOffset offset2;
  offset2.SetPartition(8);
  offset2.SetOffset(27458);
  model::StreamOffsets offsets;
  offsets.SetOffsets({offset1, offset2});
  return offsets;
}

void ReadStreamLayerClientTest::SetUp() {
  network_mock_ = std::make_shared<NetworkMock>();
  settings_ = client::OlpClientSettings();
  settings_.network_request_handler = network_mock_;
  settings_.task_scheduler =
      client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1u);

  SetUpCommonNetworkMockCalls();
}

void ReadStreamLayerClientTest::TearDown() {
  testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  network_mock_.reset();
}

void ReadStreamLayerClientTest::SetUpCommonNetworkMockCalls() {
  ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_LOOKUP));

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

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .WillByDefault(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                            olp::http::HttpStatusCode::OK),
                                        HTTP_RESPONSE_LOOKUP));

  ON_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .WillByDefault(
          ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                 olp::http::HttpStatusCode::OK),
                             HTTP_RESPONSE_BLOB_DATA_STREAM_MESSAGE));

  // Catch any non-interesting network calls that don't need to be verified
  EXPECT_CALL(*network_mock_, Send(_, _, _, _, _)).Times(testing::AtLeast(0));
}

TEST_F(ReadStreamLayerClientTest, Subscribe) {
  client::HRN hrn(GetTestCatalog());

  {
    SCOPED_TRACE("Subscribe succeeds, serial");

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_,
                Send(IsPostRequest(URL_STREAM_SUBSCRIBE_SERIAL), _, _, _, _))
        .Times(1);

    read::StreamLayerClient client(hrn, kLayerId, settings_);

    read::SubscribeResponse subscribe_response;
    client::Condition condition;
    client.Subscribe(read::SubscribeRequest(),
                     [&](read::SubscribeResponse response) {
                       subscribe_response = std::move(response);
                       condition.Notify();
                     });

    ASSERT_TRUE(condition.Wait(kTimeout));

    ASSERT_TRUE(subscribe_response.IsSuccessful());

    EXPECT_EQ(kSubscriptionId, subscribe_response.GetResult().c_str());

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Subscribe succeeds, parallel");

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_,
                Send(IsPostRequest(URL_STREAM_SUBSCRIBE_PARALLEL), _, _, _, _))
        .Times(1);

    read::StreamLayerClient client(hrn, kLayerId, settings_);

    read::SubscribeResponse subscribe_response;
    client::Condition condition;
    client.Subscribe(read::SubscribeRequest().WithSubscriptionMode(
                         read::SubscribeRequest::SubscriptionMode::kParallel),
                     [&](read::SubscribeResponse response) {
                       subscribe_response = std::move(response);
                       condition.Notify();
                     });

    ASSERT_TRUE(condition.Wait(kTimeout));

    ASSERT_TRUE(subscribe_response.IsSuccessful())
        << ApiErrorToString(subscribe_response.GetError());

    EXPECT_EQ(kSubscriptionId, subscribe_response.GetResult().c_str());

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Subscribe succeeds, with subscriptionID");

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(1);

    EXPECT_CALL(
        *network_mock_,
        Send(IsPostRequest(URL_STREAM_SUBSCRIBE_SUBSCRIPTION_ID), _, _, _, _))
        .Times(1);

    read::StreamLayerClient client(hrn, kLayerId, settings_);

    read::SubscribeResponse subscribe_response;
    client::Condition condition;
    client.Subscribe(
        read::SubscribeRequest().WithSubscriptionId(kSubscriptionId),
        [&](read::SubscribeResponse response) {
          subscribe_response = std::move(response);
          condition.Notify();
        });

    ASSERT_TRUE(condition.Wait(kTimeout));

    ASSERT_TRUE(subscribe_response.IsSuccessful())
        << ApiErrorToString(subscribe_response.GetError());

    EXPECT_EQ(kSubscriptionId, subscribe_response.GetResult().c_str());

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Subscribe succeeds, with consumerID");

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(1);

    EXPECT_CALL(
        *network_mock_,
        Send(IsPostRequest(URL_STREAM_SUBSCRIBE_CONSUMER_ID), _, _, _, _))
        .Times(1);

    read::StreamLayerClient client(hrn, kLayerId, settings_);

    read::SubscribeResponse subscribe_response;
    client::Condition condition;
    client.Subscribe(read::SubscribeRequest().WithConsumerId(kConsumerID),
                     [&](read::SubscribeResponse response) {
                       subscribe_response = std::move(response);
                       condition.Notify();
                     });

    ASSERT_TRUE(condition.Wait(kTimeout));

    ASSERT_TRUE(subscribe_response.IsSuccessful())
        << ApiErrorToString(subscribe_response.GetError());

    EXPECT_EQ(kSubscriptionId, subscribe_response.GetResult().c_str());

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Subscribe succeeds, multiple query parameters");

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(1);

    EXPECT_CALL(
        *network_mock_,
        Send(IsPostRequest(URL_STREAM_SUBSCRIBE_ALL_PARAMETERS), _, _, _, _))
        .Times(1);

    read::StreamLayerClient client(hrn, kLayerId, settings_);

    read::SubscribeResponse subscribe_response;
    client::Condition condition;
    client.Subscribe(
        read::SubscribeRequest()
            .WithConsumerId(kConsumerID)
            .WithSubscriptionId(kSubscriptionId)
            .WithSubscriptionMode(
                read::SubscribeRequest::SubscriptionMode::kParallel),
        [&](read::SubscribeResponse response) {
          subscribe_response = std::move(response);
          condition.Notify();
        });

    ASSERT_TRUE(condition.Wait(kTimeout));

    ASSERT_TRUE(subscribe_response.IsSuccessful())
        << ApiErrorToString(subscribe_response.GetError());

    EXPECT_EQ(kSubscriptionId, subscribe_response.GetResult().c_str());

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Subscribe fails, incorrect request");

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_,
                Send(IsPostRequest(URL_STREAM_SUBSCRIBE_SERIAL), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::FORBIDDEN),
                                     HTTP_RESPONSE_SUBSCRIBE_403));

    read::StreamLayerClient client(hrn, kLayerId, settings_);

    read::SubscribeResponse subscribe_response;
    client::Condition condition;
    client.Subscribe(read::SubscribeRequest(),
                     [&](read::SubscribeResponse response) {
                       subscribe_response = std::move(response);
                       condition.Notify();
                     });

    ASSERT_TRUE(condition.Wait(kTimeout));

    EXPECT_FALSE(subscribe_response.IsSuccessful());

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Subscribe fails, incorrect hrn");

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::FORBIDDEN),
                                     HTTP_RESPONSE_403));

    read::StreamLayerClient client(hrn, kLayerId, settings_);

    read::SubscribeResponse subscribe_response;
    client::Condition condition;
    client.Subscribe(read::SubscribeRequest(),
                     [&](read::SubscribeResponse response) {
                       subscribe_response = std::move(response);
                       condition.Notify();
                     });

    ASSERT_TRUE(condition.Wait(kTimeout));

    EXPECT_FALSE(subscribe_response.IsSuccessful());

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Subscribe fails, incorrect layer");

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_,
                Send(IsPostRequest(URL_STREAM_SUBSCRIBE_SERIAL), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::NOT_FOUND),
                                     HTTP_RESPONSE_SUBSCRIBE_404));

    read::StreamLayerClient client(hrn, kLayerId, settings_);

    read::SubscribeResponse subscribe_response;
    client::Condition condition;
    client.Subscribe(read::SubscribeRequest(),
                     [&](read::SubscribeResponse response) {
                       subscribe_response = std::move(response);
                       condition.Notify();
                     });

    ASSERT_TRUE(condition.Wait(kTimeout));

    EXPECT_FALSE(subscribe_response.IsSuccessful());

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(ReadStreamLayerClientTest, SubscribeCancellableFuture) {
  client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_,
              Send(IsPostRequest(URL_STREAM_SUBSCRIBE_SERIAL), _, _, _, _))
      .Times(1);

  read::StreamLayerClient client(hrn, kLayerId, settings_);

  auto future = client.Subscribe(read::SubscribeRequest()).GetFuture();

  ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

  const auto& response = future.get();
  ASSERT_TRUE(response.IsSuccessful());

  EXPECT_EQ(response.GetResult(), kSubscriptionId);
}

TEST_F(ReadStreamLayerClientTest, SubscribeApiLookup429) {
  client::HRN hrn(GetTestCatalog());
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

  client::RetrySettings retry_settings;
  retry_settings.retry_condition = [](const client::HttpResponse& response) {
    return olp::http::HttpStatusCode::TOO_MANY_REQUESTS == response.GetStatus();
  };
  settings_.retry_settings = retry_settings;
  read::StreamLayerClient client(hrn, kLayerId, settings_);

  read::SubscribeResponse subscribe_response;
  client::Condition condition;
  client.Subscribe(read::SubscribeRequest(),
                   [&](read::SubscribeResponse response) {
                     subscribe_response = std::move(response);
                     condition.Notify();
                   });

  ASSERT_TRUE(condition.Wait(kTimeout));

  ASSERT_TRUE(subscribe_response.IsSuccessful())
      << ApiErrorToString(subscribe_response.GetError());

  EXPECT_EQ(kSubscriptionId, subscribe_response.GetResult().c_str());
}

TEST_F(ReadStreamLayerClientTest, SubscribeCancelFuture) {
  client::HRN hrn(GetTestCatalog());

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

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(1)
        .WillOnce(testing::Invoke(std::move(send_mock)));

    EXPECT_CALL(*network_mock_, Cancel(request_id))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));
  }

  read::StreamLayerClient client(hrn, kLayerId, settings_);

  auto future = client.Subscribe(read::SubscribeRequest());

  request_started->get_future().get();
  future.GetCancellationToken().Cancel();
  continue_request->set_value();

  auto response = future.GetFuture().get();

  ASSERT_FALSE(response.IsSuccessful());

  EXPECT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            response.GetError().GetHttpStatusCode());
  EXPECT_EQ(client::ErrorCode::Cancelled, response.GetError().GetErrorCode());
}

TEST_F(ReadStreamLayerClientTest, Unsubscribe) {
  client::HRN hrn(GetTestCatalog());

  {
    SCOPED_TRACE("Unsubscribe succeeds, serial subscription");

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_,
                Send(IsPostRequest(URL_STREAM_SUBSCRIBE_SERIAL), _, _, _, _))
        .Times(1);

    EXPECT_CALL(
        *network_mock_,
        Send(IsDeleteRequest(URL_STREAM_UNSUBSCRIBE_SERIAL), _, _, _, _))
        .Times(1);

    read::StreamLayerClient client(hrn, kLayerId, settings_);

    auto subscribe_future =
        client.Subscribe(read::SubscribeRequest()).GetFuture();
    ASSERT_EQ(subscribe_future.wait_for(kTimeout), std::future_status::ready);
    ASSERT_TRUE(subscribe_future.get().IsSuccessful());

    // Unsubscribe part
    read::UnsubscribeResponse unsubscribe_response;
    client::Condition condition;
    client.Unsubscribe([&](read::UnsubscribeResponse response) {
      unsubscribe_response = std::move(response);
      condition.Notify();
    });

    ASSERT_TRUE(condition.Wait(kTimeout));
    ASSERT_TRUE(unsubscribe_response.IsSuccessful());
    EXPECT_EQ(kSubscriptionId, unsubscribe_response.GetResult());

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Unsubscribe succeeds, parallel subscription");

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_,
                Send(IsPostRequest(URL_STREAM_SUBSCRIBE_PARALLEL), _, _, _, _))
        .Times(1);

    EXPECT_CALL(
        *network_mock_,
        Send(IsDeleteRequest(URL_STREAM_UNSUBSCRIBE_PARALLEL), _, _, _, _))
        .Times(1);

    read::StreamLayerClient client(hrn, kLayerId, settings_);

    auto subscribe_future =
        client
            .Subscribe(read::SubscribeRequest().WithSubscriptionMode(
                read::SubscribeRequest::SubscriptionMode::kParallel))
            .GetFuture();
    ASSERT_EQ(subscribe_future.wait_for(kTimeout), std::future_status::ready);
    ASSERT_TRUE(subscribe_future.get().IsSuccessful());

    // Unsubscribe part
    read::UnsubscribeResponse unsubscribe_response;
    client::Condition condition;
    client.Unsubscribe([&](read::UnsubscribeResponse response) {
      unsubscribe_response = std::move(response);
      condition.Notify();
    });

    ASSERT_TRUE(condition.Wait(kTimeout));
    ASSERT_TRUE(unsubscribe_response.IsSuccessful());
    EXPECT_EQ(kSubscriptionId, unsubscribe_response.GetResult());

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE(
        "Unsubscribe succeeds, parallel subscription with provided ConsumerID "
        "and SubscriptionID");

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(1);

    EXPECT_CALL(
        *network_mock_,
        Send(IsPostRequest(URL_STREAM_SUBSCRIBE_ALL_PARAMETERS), _, _, _, _))
        .Times(1);

    EXPECT_CALL(
        *network_mock_,
        Send(IsDeleteRequest(URL_STREAM_UNSUBSCRIBE_PARALLEL), _, _, _, _))
        .Times(1);

    read::StreamLayerClient client(hrn, kLayerId, settings_);

    auto subscribe_future =
        client
            .Subscribe(
                read::SubscribeRequest()
                    .WithConsumerId(kConsumerID)
                    .WithSubscriptionId(kSubscriptionId)
                    .WithSubscriptionMode(
                        read::SubscribeRequest::SubscriptionMode::kParallel))
            .GetFuture();
    ASSERT_EQ(subscribe_future.wait_for(kTimeout), std::future_status::ready);
    ASSERT_TRUE(subscribe_future.get().IsSuccessful());

    read::UnsubscribeResponse unsubscribe_response;
    client::Condition condition;
    client.Unsubscribe([&](read::UnsubscribeResponse response) {
      unsubscribe_response = std::move(response);
      condition.Notify();
    });

    ASSERT_TRUE(condition.Wait(kTimeout));
    ASSERT_TRUE(unsubscribe_response.IsSuccessful());
    EXPECT_EQ(kSubscriptionId, unsubscribe_response.GetResult());

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Unsubscribe fails, subscription missing");

    read::StreamLayerClient client(hrn, kLayerId, settings_);

    read::UnsubscribeResponse unsubscribe_response;
    client::Condition condition;
    client.Unsubscribe([&](read::UnsubscribeResponse response) {
      unsubscribe_response = std::move(response);
      condition.Notify();
    });

    ASSERT_TRUE(condition.Wait(kTimeout));
    ASSERT_FALSE(unsubscribe_response.IsSuccessful());

    EXPECT_EQ(unsubscribe_response.GetError().GetErrorCode(),
              client::ErrorCode::PreconditionFailed);

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Unsubscribe fails, server error");

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
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

    read::StreamLayerClient client(hrn, kLayerId, settings_);

    auto subscribe_future =
        client.Subscribe(read::SubscribeRequest()).GetFuture();
    ASSERT_EQ(subscribe_future.wait_for(kTimeout), std::future_status::ready);
    ASSERT_TRUE(subscribe_future.get().IsSuccessful());

    // Unsubscribe part
    read::UnsubscribeResponse unsubscribe_response;
    client::Condition condition;
    client.Unsubscribe([&](read::UnsubscribeResponse response) {
      unsubscribe_response = std::move(response);
      condition.Notify();
    });

    ASSERT_TRUE(condition.Wait(kTimeout));
    ASSERT_FALSE(unsubscribe_response.IsSuccessful());
    EXPECT_EQ(unsubscribe_response.GetError().GetErrorCode(),
              client::ErrorCode::NotFound);

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(ReadStreamLayerClientTest, UnsubscribeCancellableFuture) {
  client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_,
              Send(IsPostRequest(URL_STREAM_SUBSCRIBE_SERIAL), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_,
              Send(IsDeleteRequest(URL_STREAM_UNSUBSCRIBE_SERIAL), _, _, _, _))
      .Times(1);

  read::StreamLayerClient client(hrn, kLayerId, settings_);

  auto subscribe_future =
      client.Subscribe(read::SubscribeRequest()).GetFuture();
  ASSERT_EQ(subscribe_future.wait_for(kTimeout), std::future_status::ready);
  ASSERT_TRUE(subscribe_future.get().IsSuccessful());

  // Unsubscribe part
  read::UnsubscribeResponse unsubscribe_response;
  auto unsubscribe_future = client.Unsubscribe().GetFuture();

  ASSERT_EQ(unsubscribe_future.wait_for(kTimeout), std::future_status::ready);

  const auto& response = unsubscribe_future.get();
  ASSERT_TRUE(response.IsSuccessful());

  EXPECT_EQ(response.GetResult(), kSubscriptionId);
}

TEST_F(ReadStreamLayerClientTest, UnsubscribeCancelFuture) {
  client::HRN hrn(GetTestCatalog());

  auto request_started = std::make_shared<std::promise<void>>();
  auto continue_request = std::make_shared<std::promise<void>>();
  {
    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
        request_started, continue_request,
        {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_EMPTY});

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_,
                Send(IsPostRequest(URL_STREAM_SUBSCRIBE_SERIAL), _, _, _, _))
        .Times(1);

    EXPECT_CALL(
        *network_mock_,
        Send(IsDeleteRequest(URL_STREAM_UNSUBSCRIBE_SERIAL), _, _, _, _))
        .Times(1)
        .WillOnce(testing::Invoke(std::move(send_mock)));

    EXPECT_CALL(*network_mock_, Cancel(request_id))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));
  }

  read::StreamLayerClient client(hrn, kLayerId, settings_);

  auto subscribe_future =
      client.Subscribe(read::SubscribeRequest()).GetFuture();
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
  EXPECT_EQ(client::ErrorCode::Cancelled,
            unsubscribe_response.GetError().GetErrorCode());
}

TEST_F(ReadStreamLayerClientTest, GetData) {
  client::HRN hrn(GetTestCatalog());

  {
    SCOPED_TRACE("GetData success");

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .Times(1);

    read::StreamLayerClient client(hrn, kLayerId, settings_);

    std::promise<read::DataResponse> promise;
    auto future = promise.get_future();

    model::Metadata metadata;
    metadata.SetDataHandle(kDataHandle);
    model::Message message;
    message.SetMetaData(metadata);

    client.GetData(message, [&](read::DataResponse response) {
      promise.set_value(response);
    });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_TRUE(response.IsSuccessful());
    ASSERT_TRUE(response.GetResult());

    const std::string blob_data = HTTP_RESPONSE_BLOB_DATA_STREAM_MESSAGE;
    EXPECT_THAT(*response.GetResult(),
                testing::ElementsAreArray(blob_data.begin(), blob_data.end()));

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("GetData fails, no data handle");

    read::StreamLayerClient client(hrn, kLayerId, settings_);

    std::promise<read::DataResponse> promise;
    auto future = promise.get_future();

    client.GetData(model::Message{}, [&](read::DataResponse response) {
      promise.set_value(response);
    });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    ASSERT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              client::ErrorCode::InvalidArgument);

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("GetData fails, lookup server error");

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(
                olp::http::HttpStatusCode::AUTHENTICATION_TIMEOUT),
            HTTP_RESPONSE_EMPTY));

    read::StreamLayerClient client(hrn, kLayerId, settings_);

    std::promise<read::DataResponse> promise;
    auto future = promise.get_future();

    model::Metadata metadata;
    metadata.SetDataHandle(kDataHandle);
    model::Message message;
    message.SetMetaData(metadata);

    client.GetData(message, [&](read::DataResponse response) {
      promise.set_value(response);
    });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetHttpStatusCode(),
              olp::http::HttpStatusCode::AUTHENTICATION_TIMEOUT);

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("GetData fails, blob server error");

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(1);

    EXPECT_CALL(*network_mock_,
                Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::NOT_FOUND),
                                     HTTP_RESPONSE_EMPTY));

    read::StreamLayerClient client(hrn, kLayerId, settings_);

    std::promise<read::DataResponse> promise;
    auto future = promise.get_future();

    model::Metadata metadata;
    metadata.SetDataHandle(kDataHandle);
    model::Message message;
    message.SetMetaData(metadata);

    client.GetData(message, [&](read::DataResponse response) {
      promise.set_value(response);
    });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    ASSERT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetHttpStatusCode(),
              olp::http::HttpStatusCode::NOT_FOUND);

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(ReadStreamLayerClientTest, GetDataCancellableFuture) {
  client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .Times(1);

  read::StreamLayerClient client(hrn, kLayerId, settings_);

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
              testing::ElementsAreArray(blob_data.begin(), blob_data.end()));
}

TEST_F(ReadStreamLayerClientTest, GetDataCancel) {
  client::HRN hrn(GetTestCatalog());

  auto request_started = std::make_shared<std::promise<void>>();
  auto continue_request = std::make_shared<std::promise<void>>();

  {
    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
        request_started, continue_request,
        {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_LOOKUP});

    EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
        .Times(1)
        .WillOnce(testing::Invoke(std::move(send_mock)));

    EXPECT_CALL(*network_mock_, Cancel(request_id))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));
  }

  read::StreamLayerClient client(hrn, kLayerId, settings_);

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
  EXPECT_EQ(client::ErrorCode::Cancelled, response.GetError().GetErrorCode());
}

TEST_F(ReadStreamLayerClientTest, CancelPendingRequests) {
  client::HRN hrn(GetTestCatalog());

  // Simulate a loaded queue
  std::promise<void> promise;
  auto future = promise.get_future();
  settings_.task_scheduler->ScheduleTask([&future]() { future.get(); });

  read::StreamLayerClient client(hrn, kLayerId, settings_);

  auto subscribe_future = client.Subscribe(read::SubscribeRequest());
  auto get_data_future = client.GetData(model::Message());
  auto unsubscribe_future = client.Unsubscribe();

  client.CancelPendingRequests();

  promise.set_value();

  auto subscribe_response = subscribe_future.GetFuture().get();
  ASSERT_FALSE(subscribe_response.IsSuccessful());
  EXPECT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            subscribe_response.GetError().GetHttpStatusCode());
  EXPECT_EQ(client::ErrorCode::Cancelled,
            subscribe_response.GetError().GetErrorCode());

  auto get_data_response = get_data_future.GetFuture().get();
  ASSERT_FALSE(get_data_response.IsSuccessful());
  EXPECT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            get_data_response.GetError().GetHttpStatusCode());
  EXPECT_EQ(client::ErrorCode::Cancelled,
            get_data_response.GetError().GetErrorCode());

  auto unsubscribe_response = unsubscribe_future.GetFuture().get();
  ASSERT_FALSE(unsubscribe_response.IsSuccessful());
  EXPECT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            unsubscribe_response.GetError().GetHttpStatusCode());
  EXPECT_EQ(client::ErrorCode::Cancelled,
            unsubscribe_response.GetError().GetErrorCode());
}

TEST_F(ReadStreamLayerClientTest, Seek) {
  client::HRN hrn(GetTestCatalog());
  auto stream_offsets = GetStreamOffsets();

  {
    SCOPED_TRACE("Seek success");

    read::StreamLayerClient client(hrn, kLayerId, settings_);
    auto subscribe_future =
        client.Subscribe(read::SubscribeRequest()).GetFuture();
    ASSERT_EQ(subscribe_future.wait_for(kTimeout), std::future_status::ready);
    ASSERT_TRUE(subscribe_future.get().IsSuccessful());

    EXPECT_CALL(*network_mock_, Send(IsPutRequest(URL_SEEK_STREAM), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     HTTP_RESPONSE_EMPTY));

    std::promise<read::SeekResponse> promise;
    auto future = promise.get_future();
    read::SeekRequest request;
    client.Seek(
        request.WithOffsets(stream_offsets),
        [&](read::SeekResponse response) { promise.set_value(response); });

    EXPECT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult(), olp::http::HttpStatusCode::OK);

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Seek fails, subscription is missing");

    read::StreamLayerClient client(hrn, kLayerId, settings_);

    std::promise<read::SeekResponse> promise;
    auto future = promise.get_future();
    read::SeekRequest request;
    client.Seek(
        request.WithOffsets(stream_offsets),
        [&](read::SeekResponse response) { promise.set_value(response); });

    EXPECT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::PreconditionFailed);

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Seek fails, StreamOffsets is empty");

    read::StreamLayerClient client(hrn, kLayerId, settings_);
    auto subscribe_future =
        client.Subscribe(read::SubscribeRequest()).GetFuture();
    ASSERT_EQ(subscribe_future.wait_for(kTimeout), std::future_status::ready);
    ASSERT_TRUE(subscribe_future.get().IsSuccessful());

    std::promise<read::SeekResponse> promise;
    auto future = promise.get_future();
    read::SeekRequest request;
    client.Seek(request, [&](read::SeekResponse response) {
      promise.set_value(response);
    });

    EXPECT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::PreconditionFailed);

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Seek fails, server error on SeekToOffset");

    read::StreamLayerClient client(hrn, kLayerId, settings_);
    auto subscribe_future =
        client.Subscribe(read::SubscribeRequest()).GetFuture();
    ASSERT_EQ(subscribe_future.wait_for(kTimeout), std::future_status::ready);
    ASSERT_TRUE(subscribe_future.get().IsSuccessful());

    EXPECT_CALL(*network_mock_, Send(IsPutRequest(URL_SEEK_STREAM), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::BAD_REQUEST),
                               HTTP_RESPONSE_EMPTY));

    std::promise<read::SeekResponse> promise;
    auto future = promise.get_future();
    read::SeekRequest request;
    client.Seek(
        request.WithOffsets(stream_offsets),
        [&](read::SeekResponse response) { promise.set_value(response); });

    EXPECT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetHttpStatusCode(),
              olp::http::HttpStatusCode::BAD_REQUEST);

    testing::Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(ReadStreamLayerClientTest, SeekCancellableFuture) {
  client::HRN hrn(GetTestCatalog());

  read::StreamLayerClient client(hrn, kLayerId, settings_);
  auto subscribe_future =
      client.Subscribe(read::SubscribeRequest()).GetFuture();
  ASSERT_EQ(subscribe_future.wait_for(kTimeout), std::future_status::ready);
  ASSERT_TRUE(subscribe_future.get().IsSuccessful());

  EXPECT_CALL(*network_mock_, Send(IsPutRequest(URL_SEEK_STREAM), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   HTTP_RESPONSE_EMPTY));

  auto stream_offsets = GetStreamOffsets();
  read::SeekRequest request;
  auto cancellable = client.Seek(request.WithOffsets(stream_offsets));
  auto future = cancellable.GetFuture();

  EXPECT_EQ(future.wait_for(kTimeout), std::future_status::ready);

  const auto& response = future.get();
  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_EQ(response.GetResult(), olp::http::HttpStatusCode::OK);

  testing::Mock::VerifyAndClearExpectations(network_mock_.get());
}

TEST_F(ReadStreamLayerClientTest, SeekCancel) {
  client::HRN hrn(GetTestCatalog());

  auto request_started = std::make_shared<std::promise<void>>();
  auto continue_request = std::make_shared<std::promise<void>>();
  {
    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
        request_started, continue_request,
        {olp::http::HttpStatusCode::OK, HTTP_RESPONSE_EMPTY});

    EXPECT_CALL(*network_mock_, Send(IsPutRequest(URL_SEEK_STREAM), _, _, _, _))
        .Times(1)
        .WillOnce(testing::Invoke(std::move(send_mock)));

    EXPECT_CALL(*network_mock_, Cancel(request_id))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));
  }

  read::StreamLayerClient client(hrn, kLayerId, settings_);
  auto subscribe_future =
      client.Subscribe(read::SubscribeRequest()).GetFuture();
  ASSERT_EQ(subscribe_future.wait_for(kTimeout), std::future_status::ready);
  ASSERT_TRUE(subscribe_future.get().IsSuccessful());

  auto stream_offsets = GetStreamOffsets();
  read::SeekRequest request;
  auto cancellable = client.Seek(request.WithOffsets(stream_offsets));
  auto future = cancellable.GetFuture();
  auto token = cancellable.GetCancellationToken();

  request_started->get_future().get();
  token.Cancel();
  continue_request->set_value();

  EXPECT_EQ(future.wait_for(kTimeout), std::future_status::ready);

  const auto& response = future.get();
  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(static_cast<int>(olp::http::ErrorCode::CANCELLED_ERROR),
            response.GetError().GetHttpStatusCode());
  EXPECT_EQ(client::ErrorCode::Cancelled, response.GetError().GetErrorCode());

  testing::Mock::VerifyAndClearExpectations(network_mock_.get());
}

}  // namespace
