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

#include <thread>

#include <gtest/gtest.h>
#include <matchers/NetworkUrlMatchers.h>
#include <mocks/CacheMock.h>
#include <mocks/NetworkMock.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include "StreamLayerClientImpl.h"

namespace {
using ::testing::_;
using ::testing::AllOf;
using ::testing::ElementsAreArray;
using ::testing::Mock;
namespace http = olp::http;
namespace client = olp::client;
namespace read = olp::dataservice::read;
namespace model = olp::dataservice::read::model;

constexpr auto kUrlLookup =
    R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data::olp-here-test:hereos-internal-test-v2/apis)";

constexpr auto kUrlStreamSubscribe =
    R"(https://some.stream.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/testlayer/subscribe?mode=serial)";

constexpr auto kUrlStreamConsume =
    R"(https://stream.node.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/testlayer/partitions?mode=serial&subscriptionId=12345)";

constexpr auto kUrlStreamCommitOffsets =
    R"(https://stream.node.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/testlayer/offsets?mode=serial&subscriptionId=12345)";

constexpr auto kUrlStreamSeekToOffsets =
    R"(https://stream.node.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/testlayer/seek?mode=serial&subscriptionId=12345)";

constexpr auto kUrlStreamUnsubscribe =
    R"(https://stream.node.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/testlayer/subscribe?mode=serial&subscriptionId=12345)";

constexpr auto kUrlBlobGetBlob =
    R"(https://some.blob.url/blobstore/v1/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/testlayer/data/123-some-data-handle-456)";

constexpr auto kHttpRequestBodyOffsetsTwoPartitions =
    R"jsonString({"offsets":[{"partition":1,"offset":4},{"partition":2,"offset":8}]})jsonString";

constexpr auto kHttpRequestBodyOffsetsOnePartition =
    R"jsonString({"offsets":[{"partition":1,"offset":4}]})jsonString";

constexpr auto kHttpRequestBodyWithStreamOffsets =
    R"jsonString({"offsets":[{"partition":7,"offset":38562},{"partition":8,"offset":27458}]})jsonString";

constexpr auto kHttpResponseEmpty = R"jsonString()jsonString";

constexpr auto kHttpResponseLookup =
    R"jsonString([{"api":"stream","version":"v2","baseURL":"https://some.stream.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2","parameters":{}},
    {"api":"blob","version":"v1","baseURL":"https://some.blob.url/blobstore/v1/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2","parameters":{}}])jsonString";

constexpr auto kHttpResponseSubscribe =
    R"jsonString({"nodeBaseURL":"https://stream.node.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2","subscriptionId":"12345"})jsonString";

constexpr auto kHttpResponseSubscribeForbidden =
    R"jsonString({"error":"Forbidden Error","error_description":"Error description"})jsonString";

constexpr auto kHttpResponsePollNoMessages =
    R"jsonString({"messages":[]})jsonString";

constexpr auto kHttpResponsePollOneMessage =
    R"jsonString({"messages":[{"metaData":{"partition":"1","data":"data111","timestamp":4},"offset":{"partition":1,"offset":4}}]})jsonString";

constexpr auto kHttpResponsePollTwoMessagesOnePartition =
    R"jsonString({"messages":[{"metaData":{"partition":"0","data":"data000","timestamp":2},"offset":{"partition":1,"offset":2}}, {"metaData":{"partition":"1","data":"data111","timestamp":4},"offset":{"partition":1,"offset":4}}]})jsonString";

constexpr auto kHttpResponsePollTwoMessagesTwoPartitions =
    R"jsonString({"messages":[{"metaData":{"partition":"1","data":"data111","timestamp":4},"offset":{"partition":1,"offset":4}},{"metaData":{"partition":"2","data":"data222","timestamp":8},"offset":{"partition":2,"offset":8}}]})jsonString";

constexpr auto kHttpResponsePollConsumeBadRequest =
    R"jsonString({"title":"Invalid subscriptionId","status":400,"code":"E213014","cause":"Invalid subscriptionId","action":"Retry with valid subscriptionId","correlationId":"4199533b-6290-41db-8d79-edf4f4019a74"})jsonString";

constexpr auto kHttpResponsePollCommitConflict =
    R"jsonString({"title":"Unable to commit offset","status":409,"code":"E213028","cause":"Unable to commit offset","action":"Commit cannot be completed. Continue with reading and committing new messages","correlationId":"4199533b-6290-41db-8d79-edf4f4019a74"})jsonString";

constexpr auto kHttpResponseSeekFails =
    R"jsonString({ "title": "Realm not found", "status": 400, "code": "E213017", "cause": "App / user is not associated with a realm", "action": "Update access token and retry", "correlationId": "4199533b-6290-41db-8d79-edf4f4019a74" })jsonString";

constexpr auto kHttpResponseUnsubscribeNotFound =
    R"jsonString({"title":"Subscription not found","status":404,"code":"E213003","cause":"SubscriptionId 12345 not found","action":"Subscribe again","correlationId":"123"})jsonString";

const std::string kConsumerID = "consumer_id_1234";
const read::ConsumerProperties kConsumerProperties = {
    {"key1", "value1"}, {"key2", 10}, {"key3", true}};
const std::string kLayerId = "testlayer";
const auto kTimeout = std::chrono::seconds(5);
const std::string kSubscriptionId = "12345";
const std::string kDataHandle = "123-some-data-handle-456";
const std::string kBlobData =
    "iVBORw0KGgoAAAANSUhEUgAAADAAAAAwBAMAAAClLOS0AAAABGdBTUEAALGPC/"
    "xhBQAAABhQTFRFvb29AACEAP8AhIKEPb5x2m9E5413aFQirhRuvAMqCw+"
    "6kE2BVsa8miQaYSKyshxFvhqdzKx8UsPYk9gDEcY1ghZXcPbENtax8g5T+"
    "3zHYufF1Lf9HdIZBfNEiKAAAAAElFTkSuQmCC";

model::Message PrepareMessage(const std::string& metadata_partition,
                              int32_t offset_partition, int64_t offset) {
  model::Message message;

  model::Metadata metadata;
  metadata.SetPartition(metadata_partition);
  message.SetMetaData(metadata);

  model::StreamOffset stream_offset;
  stream_offset.SetPartition(offset_partition);
  stream_offset.SetOffset(offset);
  message.SetOffset(stream_offset);

  return message;
}

model::StreamOffsets GetStreamOffsets() {
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

MATCHER_P(EqMessage, message, "Equality matcher for the Messages") {
  return arg.GetMetaData().GetPartition() ==
             message.GetMetaData().GetPartition() &&
         arg.GetOffset().GetPartition() == message.GetOffset().GetPartition() &&
         arg.GetOffset().GetOffset() == message.GetOffset().GetOffset();
}

class StreamLayerClientImplTest : public testing::Test {
 protected:
  enum class RequestMethod { GET, POST, DELETE, PUT };

  void SetUp() override;

  template <class T>
  void SetupNetworkExpectation(T url, T response, int status,
                               RequestMethod method = RequestMethod::GET,
                               T body = T());

  void SimulateSubscription(read::StreamLayerClientImpl& client);

 protected:
  const client::HRN kHrn{client::HRN::FromString(
      "hrn:here:data::olp-here-test:hereos-internal-test-v2")};
  std::shared_ptr<NetworkMock> network_mock_;
  std::shared_ptr<CacheMock> cache_mock_;
  client::OlpClientSettings settings_;
};

void StreamLayerClientImplTest::SetUp() {
  network_mock_ = std::make_shared<NetworkMock>();
  cache_mock_ = std::make_shared<CacheMock>();
  settings_.network_request_handler = network_mock_;
  settings_.cache = cache_mock_;
}

void StreamLayerClientImplTest::SimulateSubscription(
    read::StreamLayerClientImpl& client) {
  SetupNetworkExpectation(kUrlLookup, kHttpResponseLookup,
                          http::HttpStatusCode::OK);

  SetupNetworkExpectation(kUrlStreamSubscribe, kHttpResponseSubscribe,
                          http::HttpStatusCode::CREATED, RequestMethod::POST);

  auto future = client.Subscribe(read::SubscribeRequest()).GetFuture();
  ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);
  EXPECT_TRUE(future.get().IsSuccessful());
}

template <class T>
void StreamLayerClientImplTest::SetupNetworkExpectation(T url, T response,
                                                        int status,
                                                        RequestMethod method,
                                                        T body) {
  switch (method) {
    case RequestMethod::GET: {
      EXPECT_CALL(*network_mock_, Send(IsGetRequest(url), _, _, _, _))
          .WillOnce(ReturnHttpResponse(
              http::NetworkResponse().WithStatus(status), response));
      return;
    }
    case RequestMethod::POST: {
      EXPECT_CALL(*network_mock_, Send(IsPostRequest(url), _, _, _, _))
          .WillOnce(ReturnHttpResponse(
              http::NetworkResponse().WithStatus(status), response));
      return;
    }
    case RequestMethod::DELETE: {
      EXPECT_CALL(*network_mock_, Send(IsDeleteRequest(url), _, _, _, _))
          .WillOnce(ReturnHttpResponse(
              http::NetworkResponse().WithStatus(status), response));
      return;
    }
    case RequestMethod::PUT: {
      EXPECT_CALL(*network_mock_,
                  Send(AllOf(IsPutRequest(url), BodyEq(body)), _, _, _, _))
          .WillOnce(ReturnHttpResponse(
              http::NetworkResponse().WithStatus(status), response));
      return;
    }
  }
}

TEST_F(StreamLayerClientImplTest, DISABLED_Subscribe) {
  {
    SCOPED_TRACE("Subscribe success");

    read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);

    SetupNetworkExpectation(kUrlLookup, kHttpResponseLookup,
                            http::HttpStatusCode::OK);

    SetupNetworkExpectation(kUrlStreamSubscribe, kHttpResponseSubscribe,
                            http::HttpStatusCode::CREATED, RequestMethod::POST);

    std::promise<read::SubscribeResponse> promise;
    auto future = promise.get_future();

    client.Subscribe(
        read::SubscribeRequest(),
        [&](read::SubscribeResponse response) { promise.set_value(response); });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_TRUE(response.IsSuccessful());

    EXPECT_EQ(response.GetResult(), kSubscriptionId);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Subscribe failed");

    read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);

    SetupNetworkExpectation(kUrlLookup, kHttpResponseLookup,
                            http::HttpStatusCode::OK);

    SetupNetworkExpectation(
        kUrlStreamSubscribe, kHttpResponseSubscribeForbidden,
        http::HttpStatusCode::FORBIDDEN, RequestMethod::POST);

    std::promise<read::SubscribeResponse> promise;
    auto future = promise.get_future();

    client.Subscribe(
        read::SubscribeRequest(),
        [&](read::SubscribeResponse response) { promise.set_value(response); });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_FALSE(response.IsSuccessful());

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(StreamLayerClientImplTest, DISABLED_SubscribeCancellableFuture) {
  {
    SCOPED_TRACE("Subscribe success");

    read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);

    SetupNetworkExpectation(kUrlLookup, kHttpResponseLookup,
                            http::HttpStatusCode::OK);

    SetupNetworkExpectation(kUrlStreamSubscribe, kHttpResponseSubscribe,
                            http::HttpStatusCode::CREATED, RequestMethod::POST);

    auto future = client.Subscribe(read::SubscribeRequest()).GetFuture();

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_TRUE(response.IsSuccessful());

    EXPECT_EQ(response.GetResult(), kSubscriptionId);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("The second subscribe");

    read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);

    SetupNetworkExpectation(kUrlLookup, kHttpResponseLookup,
                            http::HttpStatusCode::OK);

    SetupNetworkExpectation(kUrlStreamSubscribe, kHttpResponseSubscribe,
                            http::HttpStatusCode::CREATED, RequestMethod::POST);

    {
      auto future = client.Subscribe(read::SubscribeRequest()).GetFuture();

      ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

      const auto& response = future.get();
      EXPECT_TRUE(response.IsSuccessful());

      EXPECT_EQ(response.GetResult(), kSubscriptionId);
    }
    {
      auto future = client.Subscribe(read::SubscribeRequest()).GetFuture();

      ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

      const auto& response = future.get();
      EXPECT_FALSE(response.IsSuccessful());

      EXPECT_EQ(response.GetError().GetErrorCode(),
                client::ErrorCode::InvalidArgument);
    }

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(StreamLayerClientImplTest, DISABLED_SubscribeCancel) {
  settings_.task_scheduler =
      client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);

  // Simulate a loaded queue
  std::promise<void> promise;
  auto future = promise.get_future();
  settings_.task_scheduler->ScheduleTask([&future]() { future.get(); });

  read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);

  auto cancellable = client.Subscribe(read::SubscribeRequest());

  auto subscribe_future = cancellable.GetFuture();
  cancellable.GetCancellationToken().Cancel();

  promise.set_value();

  ASSERT_EQ(subscribe_future.wait_for(kTimeout), std::future_status::ready);

  auto response = subscribe_future.get();

  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);
}

TEST_F(StreamLayerClientImplTest, DISABLED_SubscribeCancelOnClientDestroy) {
  settings_.task_scheduler =
      client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);

  // Simulate a loaded queue
  settings_.task_scheduler->ScheduleTask(
      []() { std::this_thread::sleep_for(std::chrono::seconds(1)); });

  std::future<read::SubscribeResponse> subscribe_future;
  {
    read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);
    subscribe_future = client.Subscribe(read::SubscribeRequest()).GetFuture();
  }

  ASSERT_EQ(subscribe_future.wait_for(kTimeout), std::future_status::ready);

  auto response = subscribe_future.get();
  // Callback must be called during client destructor.
  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);
}

TEST_F(StreamLayerClientImplTest, Unsubscribe) {
  {
    SCOPED_TRACE("Unsubscribe success");

    read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);
    SimulateSubscription(client);

    SetupNetworkExpectation(kUrlStreamUnsubscribe, kHttpResponseEmpty,
                            http::HttpStatusCode::OK, RequestMethod::DELETE);

    std::promise<read::UnsubscribeResponse> promise;
    auto future = promise.get_future();
    client.Unsubscribe([&](read::UnsubscribeResponse response) {
      promise.set_value(response);
    });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_TRUE(response.IsSuccessful());

    EXPECT_EQ(response.GetResult(), kSubscriptionId);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Unsubscribe fails, subscription missing");

    read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);

    std::promise<read::UnsubscribeResponse> promise;
    auto future = promise.get_future();

    client.Unsubscribe([&](read::UnsubscribeResponse response) {
      promise.set_value(response);
    });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_FALSE(response.IsSuccessful());

    EXPECT_EQ(response.GetError().GetErrorCode(),
              client::ErrorCode::PreconditionFailed);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Unsubscribe fails, server error");

    read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);
    SimulateSubscription(client);

    SetupNetworkExpectation(
        kUrlStreamUnsubscribe, kHttpResponseUnsubscribeNotFound,
        http::HttpStatusCode::NOT_FOUND, RequestMethod::DELETE);

    std::promise<read::UnsubscribeResponse> promise;
    auto future = promise.get_future();

    client.Unsubscribe([&](read::UnsubscribeResponse response) {
      promise.set_value(response);
    });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_FALSE(response.IsSuccessful());

    EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::NotFound);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(StreamLayerClientImplTest, UnsubscribeCancellableFuture) {
  read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);
  SimulateSubscription(client);

  SetupNetworkExpectation(kUrlStreamUnsubscribe, kHttpResponseEmpty,
                          http::HttpStatusCode::OK, RequestMethod::DELETE);

  auto future = client.Unsubscribe().GetFuture();

  ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

  const auto& response = future.get();
  EXPECT_TRUE(response.IsSuccessful());

  EXPECT_EQ(response.GetResult(), kSubscriptionId);

  Mock::VerifyAndClearExpectations(network_mock_.get());
}

TEST_F(StreamLayerClientImplTest, UnsubscribeCancel) {
  settings_.task_scheduler =
      client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);

  read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);
  SimulateSubscription(client);

  // Simulate a loaded queue
  std::promise<void> promise;
  auto future = promise.get_future();
  settings_.task_scheduler->ScheduleTask([&future]() { future.get(); });

  auto cancellable = client.Unsubscribe();

  auto unsubscribe_future = cancellable.GetFuture();
  cancellable.GetCancellationToken().Cancel();

  promise.set_value();

  ASSERT_EQ(unsubscribe_future.wait_for(kTimeout), std::future_status::ready);

  auto response = unsubscribe_future.get();

  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);

  Mock::VerifyAndClearExpectations(network_mock_.get());
}

TEST_F(StreamLayerClientImplTest, GetData) {
  {
    SCOPED_TRACE("GetData success");

    SetupNetworkExpectation(kUrlLookup, kHttpResponseLookup,
                            http::HttpStatusCode::OK);

    SetupNetworkExpectation(kUrlBlobGetBlob, kBlobData.c_str(),
                            http::HttpStatusCode::OK);

    read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);

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

    EXPECT_THAT(*response.GetResult(),
                ElementsAreArray(kBlobData.begin(), kBlobData.end()));

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("GetData fails, no data handle");

    EXPECT_CALL(*network_mock_, Send(_, _, _, _, _)).Times(0);

    read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);

    std::promise<read::DataResponse> promise;
    auto future = promise.get_future();

    client.GetData(model::Message{}, [&](read::DataResponse response) {
      promise.set_value(response);
    });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              client::ErrorCode::InvalidArgument);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("GetData fails, lookup server error");

    SetupNetworkExpectation(kUrlLookup, kHttpResponseEmpty,
                            http::HttpStatusCode::AUTHENTICATION_TIMEOUT);

    read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);

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
              http::HttpStatusCode::AUTHENTICATION_TIMEOUT);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("GetData fails, blob server error");

    SetupNetworkExpectation(kUrlLookup, kHttpResponseLookup,
                            http::HttpStatusCode::OK);

    SetupNetworkExpectation(kUrlBlobGetBlob, kHttpResponseEmpty,
                            http::HttpStatusCode::NOT_FOUND);

    read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);

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
              http::HttpStatusCode::NOT_FOUND);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(StreamLayerClientImplTest, GetDataCancellableFuture) {
  SetupNetworkExpectation(kUrlLookup, kHttpResponseLookup,
                          http::HttpStatusCode::OK);

  SetupNetworkExpectation(kUrlBlobGetBlob, kBlobData.c_str(),
                          http::HttpStatusCode::OK);

  read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);

  model::Metadata metadata;
  metadata.SetDataHandle(kDataHandle);
  model::Message message;
  message.SetMetaData(metadata);

  auto future = client.GetData(message).GetFuture();

  ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

  const auto& response = future.get();
  EXPECT_TRUE(response.IsSuccessful());
  ASSERT_TRUE(response.GetResult());

  EXPECT_THAT(*response.GetResult(),
              ElementsAreArray(kBlobData.begin(), kBlobData.end()));

  Mock::VerifyAndClearExpectations(network_mock_.get());
}

TEST_F(StreamLayerClientImplTest, GetDataCancel) {
  settings_.task_scheduler =
      client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);

  // Simulate a loaded queue
  std::promise<void> promise;
  auto future = promise.get_future();
  settings_.task_scheduler->ScheduleTask([&future]() { future.get(); });

  read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);

  auto cancellable = client.GetData(model::Message{});

  auto get_data_future = cancellable.GetFuture();
  cancellable.GetCancellationToken().Cancel();

  promise.set_value();

  ASSERT_EQ(get_data_future.wait_for(kTimeout), std::future_status::ready);

  auto response = get_data_future.get();

  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);

  Mock::VerifyAndClearExpectations(network_mock_.get());
}

TEST_F(StreamLayerClientImplTest, Poll) {
  const auto message1 = PrepareMessage("1", 1, 4);

  {
    SCOPED_TRACE("Poll success, no messages");

    read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);
    SimulateSubscription(client);

    SetupNetworkExpectation(kUrlStreamConsume, kHttpResponsePollNoMessages,
                            http::HttpStatusCode::OK, RequestMethod::GET);

    std::promise<read::PollResponse> promise;
    auto future = promise.get_future();
    client.Poll(
        [&](read::PollResponse response) { promise.set_value(response); });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_TRUE(response.IsSuccessful());

    const auto messages = response.GetResult().GetMessages();
    ASSERT_TRUE(messages.empty());

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Poll success, one message");

    read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);
    SimulateSubscription(client);

    SetupNetworkExpectation(kUrlStreamConsume, kHttpResponsePollOneMessage,
                            http::HttpStatusCode::OK, RequestMethod::GET);

    SetupNetworkExpectation(kUrlStreamCommitOffsets, kHttpResponseEmpty,
                            http::HttpStatusCode::OK, RequestMethod::PUT,
                            kHttpRequestBodyOffsetsOnePartition);

    std::promise<read::PollResponse> promise;
    auto future = promise.get_future();
    client.Poll(
        [&](read::PollResponse response) { promise.set_value(response); });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_TRUE(response.IsSuccessful());

    const auto messages = response.GetResult().GetMessages();
    ASSERT_EQ(messages.size(), 1u);
    ASSERT_THAT(messages[0], EqMessage(message1));

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Poll success, two messages, two partitions");

    read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);
    SimulateSubscription(client);

    SetupNetworkExpectation(kUrlStreamConsume,
                            kHttpResponsePollTwoMessagesTwoPartitions,
                            http::HttpStatusCode::OK, RequestMethod::GET);

    SetupNetworkExpectation(kUrlStreamCommitOffsets, kHttpResponseEmpty,
                            http::HttpStatusCode::OK, RequestMethod::PUT,
                            kHttpRequestBodyOffsetsTwoPartitions);

    std::promise<read::PollResponse> promise;
    auto future = promise.get_future();
    client.Poll(
        [&](read::PollResponse response) { promise.set_value(response); });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_TRUE(response.IsSuccessful());

    const auto messages = response.GetResult().GetMessages();
    ASSERT_EQ(messages.size(), 2u);
    EXPECT_THAT(messages[0], EqMessage(message1));
    EXPECT_THAT(messages[1], EqMessage(PrepareMessage("2", 2, 8)));

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE(
        "Poll success, two messages, one partition, the latest offset "
        "commited");

    read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);
    SimulateSubscription(client);

    SetupNetworkExpectation(kUrlStreamConsume,
                            kHttpResponsePollTwoMessagesOnePartition,
                            http::HttpStatusCode::OK, RequestMethod::GET);

    SetupNetworkExpectation(kUrlStreamCommitOffsets, kHttpResponseEmpty,
                            http::HttpStatusCode::OK, RequestMethod::PUT,
                            kHttpRequestBodyOffsetsOnePartition);

    std::promise<read::PollResponse> promise;
    auto future = promise.get_future();
    client.Poll(
        [&](read::PollResponse response) { promise.set_value(response); });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_TRUE(response.IsSuccessful());

    const auto messages = response.GetResult().GetMessages();
    ASSERT_EQ(messages.size(), 2u);
    EXPECT_THAT(messages[0], EqMessage(PrepareMessage("0", 1, 2)));
    EXPECT_THAT(messages[1], EqMessage(message1));

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Poll fails, subscription missing");

    read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);

    std::promise<read::PollResponse> promise;
    auto future = promise.get_future();
    client.Poll(
        [&](read::PollResponse response) { promise.set_value(response); });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              client::ErrorCode::PreconditionFailed);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Poll fails, server error on consume");

    read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);
    SimulateSubscription(client);

    SetupNetworkExpectation(
        kUrlStreamConsume, kHttpResponsePollConsumeBadRequest,
        http::HttpStatusCode::BAD_REQUEST, RequestMethod::GET);

    std::promise<read::PollResponse> promise;
    auto future = promise.get_future();
    client.Poll(
        [&](read::PollResponse response) { promise.set_value(response); });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetHttpStatusCode(),
              http::HttpStatusCode::BAD_REQUEST);
    EXPECT_EQ(response.GetError().GetMessage(),
              kHttpResponsePollConsumeBadRequest);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Poll fails, server error on commit");

    read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);
    SimulateSubscription(client);

    SetupNetworkExpectation(kUrlStreamConsume, kHttpResponsePollOneMessage,
                            http::HttpStatusCode::OK, RequestMethod::GET);

    SetupNetworkExpectation(kUrlStreamCommitOffsets,
                            kHttpResponsePollCommitConflict,
                            http::HttpStatusCode::CONFLICT, RequestMethod::PUT,
                            kHttpRequestBodyOffsetsOnePartition);

    std::promise<read::PollResponse> promise;
    auto future = promise.get_future();
    client.Poll(
        [&](read::PollResponse response) { promise.set_value(response); });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetHttpStatusCode(),
              http::HttpStatusCode::CONFLICT);
    EXPECT_EQ(response.GetError().GetMessage(),
              kHttpResponsePollCommitConflict);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(StreamLayerClientImplTest, PollCancellableFuture) {
  read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);
  SimulateSubscription(client);

  SetupNetworkExpectation(kUrlStreamConsume, kHttpResponsePollOneMessage,
                          http::HttpStatusCode::OK, RequestMethod::GET);

  SetupNetworkExpectation(kUrlStreamCommitOffsets, kHttpResponseEmpty,
                          http::HttpStatusCode::OK, RequestMethod::PUT,
                          kHttpRequestBodyOffsetsOnePartition);

  auto future = client.Poll().GetFuture();

  ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

  const auto& response = future.get();
  EXPECT_TRUE(response.IsSuccessful());

  const auto messages = response.GetResult().GetMessages();
  ASSERT_EQ(messages.size(), 1u);

  EXPECT_THAT(messages[0], EqMessage(PrepareMessage("1", 1, 4)));

  Mock::VerifyAndClearExpectations(network_mock_.get());
}

TEST_F(StreamLayerClientImplTest, PollCancel) {
  settings_.task_scheduler =
      client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);

  read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);
  SimulateSubscription(client);

  // Simulate a loaded queue
  std::promise<void> promise;
  auto future = promise.get_future();
  settings_.task_scheduler->ScheduleTask([&future]() { future.get(); });

  auto cancellable = client.Poll();

  auto poll_future = cancellable.GetFuture();
  cancellable.GetCancellationToken().Cancel();

  promise.set_value();

  ASSERT_EQ(poll_future.wait_for(kTimeout), std::future_status::ready);

  const auto& response = poll_future.get();
  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);

  Mock::VerifyAndClearExpectations(network_mock_.get());
}

TEST_F(StreamLayerClientImplTest, Seek) {
  model::StreamOffsets offsets = GetStreamOffsets();
  {
    SCOPED_TRACE("Seek success");

    read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);
    SimulateSubscription(client);

    SetupNetworkExpectation(kUrlStreamSeekToOffsets, kHttpResponseEmpty,
                            http::HttpStatusCode::OK, RequestMethod::PUT,
                            kHttpRequestBodyWithStreamOffsets);

    std::promise<read::SeekResponse> promise;
    auto future = promise.get_future();
    read::SeekRequest seek_request;
    seek_request.WithOffsets(offsets);
    client.Seek(seek_request, [&](read::SeekResponse response) {
      promise.set_value(response);
    });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult(), http::HttpStatusCode::OK);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Seek fails, subscription is missing");

    read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);

    std::promise<read::SeekResponse> promise;
    auto future = promise.get_future();
    read::SeekRequest seek_request;
    seek_request.WithOffsets(offsets);
    client.Seek(seek_request, [&](read::SeekResponse response) {
      promise.set_value(response);
    });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              client::ErrorCode::PreconditionFailed);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Seek fails, StreamOffsets is empty");

    read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);
    SimulateSubscription(client);

    std::promise<read::SeekResponse> promise;
    auto future = promise.get_future();
    read::SeekRequest seek_request;
    client.Seek(seek_request, [&](read::SeekResponse response) {
      promise.set_value(response);
    });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              client::ErrorCode::PreconditionFailed);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
  {
    SCOPED_TRACE("Seek fails, server error on SeekToOffset");

    read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);
    SimulateSubscription(client);

    SetupNetworkExpectation(kUrlStreamSeekToOffsets, kHttpResponseSeekFails,
                            http::HttpStatusCode::BAD_REQUEST,
                            RequestMethod::PUT,
                            kHttpRequestBodyWithStreamOffsets);

    std::promise<read::SeekResponse> promise;
    auto future = promise.get_future();
    read::SeekRequest seek_request;
    seek_request.WithOffsets(offsets);
    client.Seek(seek_request, [&](read::SeekResponse response) {
      promise.set_value(response);
    });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetHttpStatusCode(),
              http::HttpStatusCode::BAD_REQUEST);
    EXPECT_EQ(response.GetError().GetMessage(), kHttpResponseSeekFails);

    Mock::VerifyAndClearExpectations(network_mock_.get());
  }
}

TEST_F(StreamLayerClientImplTest, SeekCancellableFuture) {
  read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);
  SimulateSubscription(client);

  SetupNetworkExpectation(kUrlStreamSeekToOffsets, kHttpResponseEmpty,
                          http::HttpStatusCode::OK, RequestMethod::PUT,
                          kHttpRequestBodyWithStreamOffsets);

  read::SeekRequest seek_request;
  model::StreamOffsets offsets = GetStreamOffsets();
  seek_request.WithOffsets(offsets);
  auto future = client.Seek(seek_request).GetFuture();

  ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

  const auto& response = future.get();
  EXPECT_TRUE(response.IsSuccessful());

  Mock::VerifyAndClearExpectations(network_mock_.get());
}

TEST_F(StreamLayerClientImplTest, SeekCancel) {
  settings_.task_scheduler =
      client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);

  read::StreamLayerClientImpl client(kHrn, kLayerId, settings_);
  SimulateSubscription(client);

  // Simulate a loaded queue
  std::promise<void> promise;
  auto future = promise.get_future();
  settings_.task_scheduler->ScheduleTask([&future]() { future.get(); });

  read::SeekRequest seek_request;
  model::StreamOffsets offsets = GetStreamOffsets();
  seek_request.WithOffsets(offsets);

  auto cancellable = client.Seek(seek_request);
  auto cancel_future = cancellable.GetFuture();
  auto token = cancellable.GetCancellationToken();
  token.Cancel();

  promise.set_value();

  ASSERT_EQ(cancel_future.wait_for(kTimeout), std::future_status::ready);

  const auto& response = cancel_future.get();
  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);

  Mock::VerifyAndClearExpectations(network_mock_.get());
}

TEST(SubscribeRequestTest, SubscribeRequest) {
  auto sub_req = read::SubscribeRequest();

  EXPECT_EQ(sub_req.GetSubscriptionMode(),
            read::SubscribeRequest::SubscriptionMode::kSerial);
  EXPECT_FALSE(sub_req.GetSubscriptionId());
  EXPECT_FALSE(sub_req.GetConsumerId());
  EXPECT_FALSE(sub_req.GetConsumerProperties());

  sub_req
      .WithSubscriptionMode(read::SubscribeRequest::SubscriptionMode::kParallel)
      .WithSubscriptionId(kSubscriptionId)
      .WithConsumerId(kConsumerID)
      .WithConsumerProperties(kConsumerProperties);

  EXPECT_TRUE(sub_req.GetSubscriptionId());
  EXPECT_TRUE(sub_req.GetConsumerId());
  EXPECT_TRUE(sub_req.GetConsumerProperties());

  EXPECT_EQ(sub_req.GetSubscriptionMode(),
            read::SubscribeRequest::SubscriptionMode::kParallel);

  EXPECT_EQ(sub_req.GetSubscriptionId().get(), kSubscriptionId);
  EXPECT_EQ(sub_req.GetConsumerId().get(), kConsumerID);

  const auto& consumer_properties =
      sub_req.GetConsumerProperties().get().GetProperties();
  EXPECT_EQ(consumer_properties.size(),
            kConsumerProperties.GetProperties().size());
  for (size_t idx = 0; idx < consumer_properties.size(); ++idx) {
    EXPECT_EQ(consumer_properties[idx].GetKey(),
              kConsumerProperties.GetProperties()[idx].GetKey());
    EXPECT_EQ(consumer_properties[idx].GetValue(),
              kConsumerProperties.GetProperties()[idx].GetValue());
  }
}

}  // namespace
