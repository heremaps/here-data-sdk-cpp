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

#include <gtest/gtest.h>
#include <matchers/NetworkUrlMatchers.h>
#include <mocks/CacheMock.h>
#include <mocks/NetworkMock.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include "StreamLayerClientImpl.h"

namespace {
using namespace ::testing;
using namespace olp;
using namespace olp::client;
using namespace olp::dataservice::read;
using namespace olp::tests::common;

constexpr auto kUrlLookupStream =
    R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data::olp-here-test:hereos-internal-test-v2/apis/stream/v2)";

constexpr auto kUrlLookupBlob =
    R"(https://api-lookup.data.api.platform.here.com/lookup/v1/resources/hrn:here:data::olp-here-test:hereos-internal-test-v2/apis/blob/v1)";

constexpr auto kUrlStreamSubscribe =
    R"(https://some.stream.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/testlayer/subscribe?mode=serial)";

constexpr auto kUrlStreamUnsubscribe =
    R"(https://some.stream.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/testlayer/subscribe?mode=serial&subscriptionId=12345)";

constexpr auto kUrlBlobGetBlob =
    R"(https://some.stream.url/blobstore/v1/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2/layers/testlayer/data/123-some-data-handle-456)";

constexpr auto kHttpResponseEmpty = R"jsonString()jsonString";

constexpr auto kHttpResponseLookupStream =
    R"jsonString([{"api":"stream","version":"v2","baseURL":"https://some.stream.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2","parameters":{}}])jsonString";

constexpr auto kHttpResponseLookupBlob =
    R"jsonString([{"api":"blob","version":"v1","baseURL":"https://some.stream.url/blobstore/v1/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2","parameters":{}}])jsonString";

constexpr auto kHttpResponseSubscribe =
    R"jsonString({ "nodeBaseURL": "https://some.stream.url/stream/v2/catalogs/hrn:here:data::olp-here-test:hereos-internal-test-v2", "subscriptionId": "12345" })jsonString";

constexpr auto kHttpResponseSubscribeForbidden =
    R"jsonString({ "error": "Forbidden Error", "error_description": "Error description" })jsonString";

constexpr auto kHttpResponseUnsubscribeNotFound =
    R"jsonString({"title": "Subscription not found","status": 404,"code": "E213003","cause": "SubscriptionId 12345 not found","action": "Subscribe again","correlationId": "123"})jsonString";

const std::string kCatalog =
    "hrn:here:data::olp-here-test:hereos-internal-test-v2";
const std::string kConsumerID = "consumer_id_1234";
const ConsumerProperties kConsumerProperties = {
    {"key1", "value1"}, {"key2", 10}, {"key3", true}};
const auto kHrn = HRN::FromString(kCatalog);
const std::string kLayerId = "testlayer";
const auto kTimeout = std::chrono::seconds(5);
const std::string kSubscriptionId = "12345";
const std::string kDataHandle = "123-some-data-handle-456";
const std::string kBlobData =
    "iVBORw0KGgoAAAANSUhEUgAAADAAAAAwBAMAAAClLOS0AAAABGdBTUEAALGPC/"
    "xhBQAAABhQTFRFvb29AACEAP8AhIKEPb5x2m9E5413aFQirhRuvAMqCw+"
    "6kE2BVsa8miQaYSKyshxFvhqdzKx8UsPYk9gDEcY1ghZXcPbENtax8g5T+"
    "3zHYufF1Lf9HdIZBfNEiKAAAAAElFTkSuQmCC";

enum class RequestMethod { GET, POST, DELETE };

template <class T>
void SetupNetworkExpectation(NetworkMock& network_mock, T url, T response,
                             int status,
                             RequestMethod method = RequestMethod::GET) {
  switch (method) {
    case RequestMethod::GET: {
      EXPECT_CALL(network_mock, Send(IsGetRequest(url), _, _, _, _))
          .WillOnce(ReturnHttpResponse(
              http::NetworkResponse().WithStatus(status), response));
      return;
    }
    case RequestMethod::POST: {
      EXPECT_CALL(network_mock, Send(IsPostRequest(url), _, _, _, _))
          .WillOnce(ReturnHttpResponse(
              http::NetworkResponse().WithStatus(status), response));
      return;
    }
    case RequestMethod::DELETE: {
      EXPECT_CALL(network_mock, Send(IsDeleteRequest(url), _, _, _, _))
          .WillOnce(ReturnHttpResponse(
              http::NetworkResponse().WithStatus(status), response));
      return;
    }
    default:
      return;
  }
}

TEST(StreamLayerClientImplTest, Subscribe) {
  auto network_mock = std::make_shared<NetworkMock>();
  auto cache_mock = std::make_shared<CacheMock>();
  OlpClientSettings settings;
  settings.network_request_handler = network_mock;
  settings.cache = cache_mock;

  {
    SCOPED_TRACE("Subscribe success");

    StreamLayerClientImpl client(kHrn, kLayerId, settings);

    SetupNetworkExpectation(*network_mock, kUrlLookupStream,
                            kHttpResponseLookupStream,
                            http::HttpStatusCode::OK);

    SetupNetworkExpectation(*network_mock, kUrlStreamSubscribe,
                            kHttpResponseSubscribe,
                            http::HttpStatusCode::CREATED, RequestMethod::POST);

    std::promise<SubscribeResponse> promise;
    auto future = promise.get_future();

    client.Subscribe(SubscribeRequest(), [&](SubscribeResponse response) {
      promise.set_value(response);
    });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    ASSERT_TRUE(response.IsSuccessful());

    ASSERT_EQ(response.GetResult(), kSubscriptionId);

    Mock::VerifyAndClearExpectations(network_mock.get());
  }

  {
    SCOPED_TRACE("Subscribe failed");

    StreamLayerClientImpl client(kHrn, kLayerId, settings);

    SetupNetworkExpectation(*network_mock, kUrlLookupStream,
                            kHttpResponseLookupStream,
                            http::HttpStatusCode::OK);

    SetupNetworkExpectation(
        *network_mock, kUrlStreamSubscribe, kHttpResponseSubscribeForbidden,
        http::HttpStatusCode::FORBIDDEN, RequestMethod::POST);

    std::promise<SubscribeResponse> promise;
    auto future = promise.get_future();

    client.Subscribe(SubscribeRequest(), [&](SubscribeResponse response) {
      promise.set_value(response);
    });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    ASSERT_FALSE(response.IsSuccessful());

    Mock::VerifyAndClearExpectations(network_mock.get());
  }
}

TEST(StreamLayerClientImplTest, SubscribeCancellableFuture) {
  auto network_mock = std::make_shared<NetworkMock>();
  auto cache_mock = std::make_shared<CacheMock>();
  OlpClientSettings settings;
  settings.network_request_handler = network_mock;
  settings.cache = cache_mock;

  {
    SCOPED_TRACE("Subscribe success");

    StreamLayerClientImpl client(kHrn, kLayerId, settings);

    SetupNetworkExpectation(*network_mock, kUrlLookupStream,
                            kHttpResponseLookupStream,
                            http::HttpStatusCode::OK);

    SetupNetworkExpectation(*network_mock, kUrlStreamSubscribe,
                            kHttpResponseSubscribe,
                            http::HttpStatusCode::CREATED, RequestMethod::POST);

    auto future = client.Subscribe(SubscribeRequest()).GetFuture();

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    ASSERT_TRUE(response.IsSuccessful());

    ASSERT_EQ(response.GetResult(), kSubscriptionId);

    Mock::VerifyAndClearExpectations(network_mock.get());
  }

  {
    SCOPED_TRACE("The second subscribe");

    StreamLayerClientImpl client(kHrn, kLayerId, settings);

    SetupNetworkExpectation(*network_mock, kUrlLookupStream,
                            kHttpResponseLookupStream,
                            http::HttpStatusCode::OK);

    SetupNetworkExpectation(*network_mock, kUrlStreamSubscribe,
                            kHttpResponseSubscribe,
                            http::HttpStatusCode::CREATED, RequestMethod::POST);

    {
      auto future = client.Subscribe(SubscribeRequest()).GetFuture();

      ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

      const auto& response = future.get();
      ASSERT_TRUE(response.IsSuccessful());

      ASSERT_EQ(response.GetResult(), kSubscriptionId);
    }
    {
      auto future = client.Subscribe(SubscribeRequest()).GetFuture();

      ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

      const auto& response = future.get();
      ASSERT_FALSE(response.IsSuccessful());

      ASSERT_EQ(response.GetError().GetErrorCode(), ErrorCode::InvalidArgument);
    }
    Mock::VerifyAndClearExpectations(network_mock.get());
  }
}

TEST(StreamLayerClientImplTest, SubscribeCancel) {
  auto network_mock = std::make_shared<NetworkMock>();
  auto cache_mock = std::make_shared<CacheMock>();
  OlpClientSettings settings;
  settings.network_request_handler = network_mock;
  settings.cache = cache_mock;
  settings.task_scheduler =
      OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);

  {
    // Simulate a loaded queue
    std::promise<void> promise;
    auto future = promise.get_future();
    settings.task_scheduler->ScheduleTask([&future]() { future.get(); });

    StreamLayerClientImpl client(kHrn, kLayerId, settings);

    auto cancellable = client.Subscribe(SubscribeRequest());

    auto subscribe_future = cancellable.GetFuture();
    cancellable.GetCancellationToken().Cancel();

    promise.set_value();

    ASSERT_EQ(subscribe_future.wait_for(kTimeout), std::future_status::ready);

    auto response = subscribe_future.get();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::Cancelled);
  }
}

TEST(StreamLayerClientImplTest, SubscribeCancelOnClientDestroy) {
  auto network_mock = std::make_shared<NetworkMock>();
  auto cache_mock = std::make_shared<CacheMock>();
  OlpClientSettings settings;
  settings.network_request_handler = network_mock;
  settings.cache = cache_mock;
  settings.task_scheduler =
      OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);

  {
    // Simulate a loaded queue
    settings.task_scheduler->ScheduleTask(
        []() { std::this_thread::sleep_for(std::chrono::seconds(1)); });

    std::future<SubscribeResponse> subscribe_future;
    {
      StreamLayerClientImpl client(kHrn, kLayerId, settings);
      subscribe_future = client.Subscribe(SubscribeRequest()).GetFuture();
    }

    ASSERT_EQ(subscribe_future.wait_for(kTimeout), std::future_status::ready);

    auto response = subscribe_future.get();
    // Callback must be called during client destructor.
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::Cancelled);
  }
}

TEST(StreamLayerClientImplTest, Unsubscribe) {
  auto network_mock = std::make_shared<NetworkMock>();
  auto cache_mock = std::make_shared<CacheMock>();
  OlpClientSettings settings;
  settings.network_request_handler = network_mock;
  settings.cache = cache_mock;

  {
    SCOPED_TRACE("Unsubscribe success");

    StreamLayerClientImpl client(kHrn, kLayerId, settings);

    {
      SetupNetworkExpectation(*network_mock, kUrlLookupStream,
                              kHttpResponseLookupStream,
                              http::HttpStatusCode::OK);

      SetupNetworkExpectation(
          *network_mock, kUrlStreamSubscribe, kHttpResponseSubscribe,
          http::HttpStatusCode::CREATED, RequestMethod::POST);

      std::promise<SubscribeResponse> promise;
      auto future = promise.get_future();
      client.Subscribe(SubscribeRequest(), [&](SubscribeResponse response) {
        promise.set_value(response);
      });

      ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

      const auto& response = future.get();
      ASSERT_TRUE(response.IsSuccessful());

      ASSERT_EQ(response.GetResult(), kSubscriptionId);
    }
    {
      SetupNetworkExpectation(*network_mock, kUrlStreamUnsubscribe,
                              kHttpResponseEmpty, http::HttpStatusCode::OK,
                              RequestMethod::DELETE);

      std::promise<UnsubscribeResponse> promise;
      auto future = promise.get_future();
      client.Unsubscribe(
          [&](UnsubscribeResponse response) { promise.set_value(response); });

      ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

      const auto& response = future.get();
      ASSERT_TRUE(response.IsSuccessful());

      ASSERT_EQ(response.GetResult(), kSubscriptionId);
    }

    Mock::VerifyAndClearExpectations(network_mock.get());
  }

  {
    SCOPED_TRACE("Unsubscribe fails, subscription missing");

    StreamLayerClientImpl client(kHrn, kLayerId, settings);

    {
      std::promise<UnsubscribeResponse> promise;
      auto future = promise.get_future();

      client.Unsubscribe(
          [&](UnsubscribeResponse response) { promise.set_value(response); });

      ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

      const auto& response = future.get();
      ASSERT_FALSE(response.IsSuccessful());

      ASSERT_EQ(response.GetError().GetErrorCode(),
                ErrorCode::PreconditionFailed);
    }

    Mock::VerifyAndClearExpectations(network_mock.get());
  }

  {
    SCOPED_TRACE("Unsubscribe fails, server error");

    StreamLayerClientImpl client(kHrn, kLayerId, settings);

    {
      SetupNetworkExpectation(*network_mock, kUrlLookupStream,
                              kHttpResponseLookupStream,
                              http::HttpStatusCode::OK);

      SetupNetworkExpectation(
          *network_mock, kUrlStreamSubscribe, kHttpResponseSubscribe,
          http::HttpStatusCode::CREATED, RequestMethod::POST);

      std::promise<SubscribeResponse> promise;
      auto future = promise.get_future();
      client.Subscribe(SubscribeRequest(), [&](SubscribeResponse response) {
        promise.set_value(response);
      });

      ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

      const auto& response = future.get();
      ASSERT_TRUE(response.IsSuccessful());

      ASSERT_EQ(response.GetResult(), kSubscriptionId);
    }
    {
      SetupNetworkExpectation(*network_mock, kUrlStreamUnsubscribe,
                              kHttpResponseUnsubscribeNotFound,
                              http::HttpStatusCode::NOT_FOUND,
                              RequestMethod::DELETE);

      std::promise<UnsubscribeResponse> promise;
      auto future = promise.get_future();

      client.Unsubscribe(
          [&](UnsubscribeResponse response) { promise.set_value(response); });

      ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

      const auto& response = future.get();
      ASSERT_FALSE(response.IsSuccessful());

      ASSERT_EQ(response.GetError().GetErrorCode(), ErrorCode::NotFound);
    }

    Mock::VerifyAndClearExpectations(network_mock.get());
  }
}

TEST(StreamLayerClientImplTest, UnsubscribeCancellableFuture) {
  auto network_mock = std::make_shared<NetworkMock>();
  auto cache_mock = std::make_shared<CacheMock>();
  OlpClientSettings settings;
  settings.network_request_handler = network_mock;
  settings.cache = cache_mock;

  StreamLayerClientImpl client(kHrn, kLayerId, settings);

  {
    SetupNetworkExpectation(*network_mock, kUrlLookupStream,
                            kHttpResponseLookupStream,
                            http::HttpStatusCode::OK);

    SetupNetworkExpectation(*network_mock, kUrlStreamSubscribe,
                            kHttpResponseSubscribe,
                            http::HttpStatusCode::CREATED, RequestMethod::POST);

    auto future = client.Subscribe(SubscribeRequest()).GetFuture();

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    ASSERT_TRUE(response.IsSuccessful());

    ASSERT_EQ(response.GetResult(), kSubscriptionId);
  }
  {
    SetupNetworkExpectation(*network_mock, kUrlStreamUnsubscribe,
                            kHttpResponseEmpty, http::HttpStatusCode::OK,
                            RequestMethod::DELETE);

    auto future = client.Unsubscribe().GetFuture();

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    ASSERT_TRUE(response.IsSuccessful());

    ASSERT_EQ(response.GetResult(), kSubscriptionId);
  }

  Mock::VerifyAndClearExpectations(network_mock.get());
}

TEST(StreamLayerClientImplTest, UnsubscribeCancel) {
  auto network_mock = std::make_shared<NetworkMock>();
  auto cache_mock = std::make_shared<CacheMock>();
  OlpClientSettings settings;
  settings.network_request_handler = network_mock;
  settings.cache = cache_mock;
  settings.task_scheduler =
      OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);

  SetupNetworkExpectation(*network_mock, kUrlLookupStream,
                          kHttpResponseLookupStream, http::HttpStatusCode::OK);

  SetupNetworkExpectation(*network_mock, kUrlStreamSubscribe,
                          kHttpResponseSubscribe, http::HttpStatusCode::CREATED,
                          RequestMethod::POST);

  StreamLayerClientImpl client(kHrn, kLayerId, settings);

  {
    auto future = client.Subscribe(SubscribeRequest()).GetFuture();

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    ASSERT_TRUE(response.IsSuccessful());

    ASSERT_EQ(response.GetResult(), kSubscriptionId);
  }
  {
    // Simulate a loaded queue
    std::promise<void> promise;
    auto future = promise.get_future();
    settings.task_scheduler->ScheduleTask([&future]() { future.get(); });

    auto cancellable = client.Unsubscribe();

    auto unsubscribe_future = cancellable.GetFuture();
    cancellable.GetCancellationToken().Cancel();

    promise.set_value();

    ASSERT_EQ(unsubscribe_future.wait_for(kTimeout), std::future_status::ready);

    auto response = unsubscribe_future.get();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::Cancelled);
  }

  Mock::VerifyAndClearExpectations(network_mock.get());
}

TEST(StreamLayerClientImplTest, GetData) {
  auto network_mock = std::make_shared<NetworkMock>();
  auto cache_mock = std::make_shared<CacheMock>();
  OlpClientSettings settings;
  settings.network_request_handler = network_mock;
  settings.cache = cache_mock;

  {
    SCOPED_TRACE("GetData success");

    SetupNetworkExpectation(*network_mock, kUrlLookupBlob,
                            kHttpResponseLookupBlob, http::HttpStatusCode::OK);

    SetupNetworkExpectation(*network_mock, kUrlBlobGetBlob, kBlobData.c_str(),
                            http::HttpStatusCode::OK);

    StreamLayerClientImpl client(kHrn, kLayerId, settings);

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

    EXPECT_THAT(*response.GetResult(),
                ElementsAreArray(kBlobData.begin(), kBlobData.end()));

    Mock::VerifyAndClearExpectations(network_mock.get());
  }
  {
    SCOPED_TRACE("GetData fails, no data handle");

    EXPECT_CALL(*network_mock, Send(_, _, _, _, _)).Times(0);

    StreamLayerClientImpl client(kHrn, kLayerId, settings);

    std::promise<DataResponse> promise;
    auto future = promise.get_future();

    client.GetData(model::Message{},
                   [&](DataResponse response) { promise.set_value(response); });

    ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);

    const auto& response = future.get();
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::InvalidArgument);

    Mock::VerifyAndClearExpectations(network_mock.get());
  }
  {
    SCOPED_TRACE("GetData fails, lookup server error");

    SetupNetworkExpectation(*network_mock, kUrlLookupBlob, kHttpResponseEmpty,
                            http::HttpStatusCode::AUTHENTICATION_TIMEOUT);

    StreamLayerClientImpl client(kHrn, kLayerId, settings);

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
              http::HttpStatusCode::AUTHENTICATION_TIMEOUT);

    Mock::VerifyAndClearExpectations(network_mock.get());
  }
  {
    SCOPED_TRACE("GetData fails, blob server error");

    SetupNetworkExpectation(*network_mock, kUrlLookupBlob,
                            kHttpResponseLookupBlob, http::HttpStatusCode::OK);

    SetupNetworkExpectation(*network_mock, kUrlBlobGetBlob, kHttpResponseEmpty,
                            http::HttpStatusCode::NOT_FOUND);

    StreamLayerClientImpl client(kHrn, kLayerId, settings);

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
              http::HttpStatusCode::NOT_FOUND);

    Mock::VerifyAndClearExpectations(network_mock.get());
  }
}

TEST(StreamLayerClientImplTest, GetDataCancellableFuture) {
  auto network_mock = std::make_shared<NetworkMock>();
  auto cache_mock = std::make_shared<CacheMock>();
  OlpClientSettings settings;
  settings.network_request_handler = network_mock;
  settings.cache = cache_mock;

  SetupNetworkExpectation(*network_mock, kUrlLookupBlob,
                          kHttpResponseLookupBlob, http::HttpStatusCode::OK);

  SetupNetworkExpectation(*network_mock, kUrlBlobGetBlob, kBlobData.c_str(),
                          http::HttpStatusCode::OK);

  StreamLayerClientImpl client(kHrn, kLayerId, settings);

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
}

TEST(StreamLayerClientImplTest, GetDataCancel) {
  auto network_mock = std::make_shared<NetworkMock>();
  auto cache_mock = std::make_shared<CacheMock>();
  OlpClientSettings settings;
  settings.network_request_handler = network_mock;
  settings.cache = cache_mock;
  settings.task_scheduler =
      OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);

  // Simulate a loaded queue
  std::promise<void> promise;
  auto future = promise.get_future();
  settings.task_scheduler->ScheduleTask([&future]() { future.get(); });

  StreamLayerClientImpl client(kHrn, kLayerId, settings);

  auto cancellable = client.GetData(model::Message{});

  auto get_data_future = cancellable.GetFuture();
  cancellable.GetCancellationToken().Cancel();

  promise.set_value();

  ASSERT_EQ(get_data_future.wait_for(kTimeout), std::future_status::ready);

  auto response = get_data_future.get();

  EXPECT_FALSE(response.IsSuccessful());
  EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::Cancelled);
}

TEST(SubscribeRequestTest, SubscribeRequest) {
  ASSERT_EQ(SubscribeRequest().GetSubscriptionMode(),
            SubscribeRequest::SubscriptionMode::kSerial);
  ASSERT_FALSE(SubscribeRequest().GetSubscriptionId());
  ASSERT_FALSE(SubscribeRequest().GetConsumerId());
  ASSERT_FALSE(SubscribeRequest().GetConsumerProperties());

  auto sub_req =
      SubscribeRequest()
          .WithSubscriptionMode(SubscribeRequest::SubscriptionMode::kParallel)
          .WithSubscriptionId(kSubscriptionId)
          .WithConsumerId(kConsumerID)
          .WithConsumerProperties(kConsumerProperties);

  ASSERT_TRUE(sub_req.GetSubscriptionId());
  ASSERT_TRUE(sub_req.GetConsumerId());
  ASSERT_TRUE(sub_req.GetConsumerProperties());

  ASSERT_EQ(sub_req.GetSubscriptionMode(),
            SubscribeRequest::SubscriptionMode::kParallel);

  ASSERT_EQ(sub_req.GetSubscriptionId().get(), kSubscriptionId);
  ASSERT_EQ(sub_req.GetConsumerId().get(), kConsumerID);

  const auto& consumer_properties =
      sub_req.GetConsumerProperties().get().GetProperties();
  ASSERT_EQ(consumer_properties.size(),
            kConsumerProperties.GetProperties().size());
  for (size_t idx = 0; idx < consumer_properties.size(); ++idx) {
    ASSERT_EQ(consumer_properties[idx].GetKey(),
              kConsumerProperties.GetProperties()[idx].GetKey());
    ASSERT_EQ(consumer_properties[idx].GetValue(),
              kConsumerProperties.GetProperties()[idx].GetValue());
  }
}

}  // namespace
