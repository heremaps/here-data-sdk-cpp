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
#include <mocks/CacheMock.h>
#include <mocks/NetworkMock.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include "StreamLayerClientImpl.h"

namespace {

using namespace testing;
using namespace olp;
using namespace olp::dataservice::write;
using namespace olp::tests::common;

const olp::client::HRN kHRN{"hrn:here:data:::catalog"};
constexpr auto kLayerName = "layer";

const std::string kConfigBaseUrl = "https://some.config.url/config/v1";
const std::string kConfigRequestUrl =
    "https://api-lookup.data.api.platform.here.com/lookup/v1/platform/apis/"
    "config/v1";
const std::string kConfigHttpResponse =
    R"jsonString([{"api":"config","version":"v1","baseURL":")jsonString" +
    kConfigBaseUrl + R"jsonString(","parameters":{}}])jsonString";

const std::string kIngestRequestUrl =
    "https://api-lookup.data.api.platform.here.com/lookup/v1/resources/" +
    kHRN.ToString() + "/apis/ingest/v1";
const std::string kIngestBaseUrl =
    "https://some.ingest.url/ingest/v1/catalogs/" + kHRN.ToString();
const std::string kIngestHttpResponse =
    R"jsonString([{"api":"ingest","version":"v1","baseURL":")jsonString" +
    kIngestBaseUrl + R"jsonString(","parameters":{}}])jsonString";

const std::string kGetCatalogRequest =
    kConfigBaseUrl + "/catalogs/" + kHRN.ToString();
const std::string kGetCatalogResponse =
    R"jsonString({"id":"catalog","hrn":")jsonString" + kConfigBaseUrl +
    R"jsonString(","layers":[{"id":"layer","hrn":"hrn:here:data:::catalog:layer",
     "contentType":"text/plain","layerType":"stream"}],"version":42})jsonString";

const std::string kPostIngestDataRequest =
    kIngestBaseUrl + "/layers/" + kLayerName;
const std::string kPostIngestDataTraceID = "aaaaa-bbb-ccc-dddd";
const std::string kPostIngestDataHttpResponse =
    R"jsonString({"TraceID":")jsonString" + kPostIngestDataTraceID +
    R"jsonString("})jsonString";

class MockStreamLayerClientImpl : public StreamLayerClientImpl {
 public:
  using StreamLayerClientImpl::PublishDataLessThanTwentyMibSync;
  using StreamLayerClientImpl::StreamLayerClientImpl;

  MOCK_METHOD(PublishSdiiResponse, IngestSdii,
              (model::PublishSdiiRequest request,
               client::CancellationContext context),
              (override));
};

class StreamLayerClientImplTest : public ::testing::Test {
 protected:
  void SetUp() override {
    cache_ = std::make_shared<CacheMock>();
    network_ = std::make_shared<NetworkMock>();

    settings_.network_request_handler = network_;
    settings_.cache = cache_;
    settings_.task_scheduler =
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);
  }

  void TearDown() override {
    settings_.network_request_handler.reset();
    settings_.cache.reset();

    network_.reset();
    cache_.reset();
  }

  std::shared_ptr<CacheMock> cache_;
  std::shared_ptr<NetworkMock> network_;
  olp::client::OlpClientSettings settings_;
};

TEST_F(StreamLayerClientImplTest, PublishSdii) {
  const auto trace_id = "123";

  model::PublishSdiiRequest request;
  request.WithSdiiMessageList(std::make_shared<std::vector<unsigned char>>());
  request.WithLayerId(kLayerName);
  request.WithTraceId(trace_id);

  auto client = std::make_shared<MockStreamLayerClientImpl>(
      kHRN, StreamLayerClientSettings{}, settings_);

  EXPECT_CALL(*client, IngestSdii).WillOnce(Return(model::ResponseOk{}));

  {
    auto good_request = request;
    auto result = client->PublishSdii(good_request).GetFuture().get();
    EXPECT_TRUE(result.IsSuccessful());
  }

  {
    auto bad_request = request;
    auto result = client->PublishSdii(bad_request.WithSdiiMessageList(nullptr))
                      .GetFuture()
                      .get();
    EXPECT_FALSE(result.IsSuccessful());
    EXPECT_EQ(result.GetError().GetErrorCode(),
              client::ErrorCode::InvalidArgument);
  }

  {
    auto bad_request = request;
    auto result =
        client->PublishSdii(bad_request.WithLayerId("")).GetFuture().get();
    EXPECT_FALSE(result.IsSuccessful());
    EXPECT_EQ(result.GetError().GetErrorCode(),
              client::ErrorCode::InvalidArgument);
  }
}

TEST_F(StreamLayerClientImplTest,
       SuccessfullyPublishDataLessThanTwentyMibSync) {
  auto data = std::make_shared<std::vector<unsigned char>>(1, 'a');
  auto request =
      model::PublishDataRequest().WithData(data).WithLayerId(kLayerName);

  auto settings = settings_;
  settings.cache = nullptr;

  MockStreamLayerClientImpl client{kHRN, StreamLayerClientSettings{}, settings};

  EXPECT_CALL(*network_, Send(IsGetRequest(kConfigRequestUrl), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kConfigHttpResponse));

  EXPECT_CALL(*network_, Send(IsGetRequest(kGetCatalogRequest), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kGetCatalogResponse));

  EXPECT_CALL(*network_, Send(IsGetRequest(kIngestRequestUrl), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kIngestHttpResponse));

  EXPECT_CALL(*network_,
              Send(IsPostRequest(kPostIngestDataRequest), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kPostIngestDataHttpResponse));

  auto response = client.PublishDataLessThanTwentyMibSync(
      request, client::CancellationContext{});
  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_EQ(kPostIngestDataTraceID, response.GetResult().GetTraceID());
}

TEST_F(StreamLayerClientImplTest, FaliedPublishDataLessThanTwentyMibSync) {
  auto data = std::make_shared<std::vector<unsigned char>>(1, 'a');
  auto request =
      model::PublishDataRequest().WithData(data).WithLayerId(kLayerName);

  auto settings = settings_;
  settings.cache = nullptr;

  MockStreamLayerClientImpl client{kHRN, StreamLayerClientSettings{}, settings};

  // Current expectations on NetworkMock will first return a failing response
  // and after each subsequent request with same URL will return correct
  // response. So, no need to clear mock expectations after each scoped trace,
  // because we testing a method step-by-step with this approach.
  {
    SCOPED_TRACE("Failed on getting a config");

    EXPECT_CALL(*network_, Send(IsGetRequest(kConfigRequestUrl), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::BAD_REQUEST),
                               std::string{}))
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kConfigHttpResponse));

    auto response = client.PublishDataLessThanTwentyMibSync(
        request, client::CancellationContext{});

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
              response.GetError().GetHttpStatusCode());
  }

  {
    SCOPED_TRACE("Failed on retrieving a catalog");

    EXPECT_CALL(*network_, Send(IsGetRequest(kGetCatalogRequest), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::BAD_REQUEST),
                               std::string{}))
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kGetCatalogResponse));

    auto response = client.PublishDataLessThanTwentyMibSync(
        request, client::CancellationContext{});

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
              response.GetError().GetHttpStatusCode());
    EXPECT_TRUE(response.GetError().GetMessage().empty());
  }

  {
    SCOPED_TRACE("Failed on retrieving catalog with invalid layer");

    auto request2 = request;
    request2.WithLayerId("invalid_layer_id");
    auto response = client.PublishDataLessThanTwentyMibSync(
        request2, client::CancellationContext{});

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(olp::client::ErrorCode::InvalidArgument,
              response.GetError().GetErrorCode());
  }

  {
    SCOPED_TRACE("Failed on retrieving an ingest API");

    EXPECT_CALL(*network_, Send(IsGetRequest(kIngestRequestUrl), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::BAD_REQUEST),
                               std::string{}))
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kIngestHttpResponse));

    auto response = client.PublishDataLessThanTwentyMibSync(
        request, client::CancellationContext{});

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
              response.GetError().GetHttpStatusCode());
    // For some reason the OlpClient return error string as:
    // 'Error occured. Please check HTTP status code.'
    // Maybe we should reconsider this...
    EXPECT_EQ("Error occured. Please check HTTP status code.",
              response.GetError().GetMessage());
  }

  {
    SCOPED_TRACE("Failed on publishing via ingest API");

    EXPECT_CALL(*network_,
                Send(IsPostRequest(kPostIngestDataRequest), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::BAD_REQUEST),
                               std::string{}));

    auto response = client.PublishDataLessThanTwentyMibSync(
        request, client::CancellationContext{});

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
              response.GetError().GetHttpStatusCode());
    EXPECT_TRUE(response.GetError().GetMessage().empty());
  }
}

TEST_F(StreamLayerClientImplTest, CancelPublishDataLessThanTwentyMibSync) {
  auto data = std::make_shared<std::vector<unsigned char>>(1, 'a');
  auto request =
      model::PublishDataRequest().WithData(data).WithLayerId(kLayerName);

  auto settings = settings_;
  settings.cache = nullptr;

  MockStreamLayerClientImpl client{kHRN, StreamLayerClientSettings{}, settings};
  {
    SCOPED_TRACE("Cancelled before publish call");

    client::CancellationContext cancel_context;
    cancel_context.CancelOperation();

    auto response =
        client.PublishDataLessThanTwentyMibSync(request, cancel_context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(olp::client::ErrorCode::Cancelled,
              response.GetError().GetErrorCode());
  }

  EXPECT_CALL(*network_, Cancel(_)).Times(4);

  auto cancel_context = std::make_shared<client::CancellationContext>();
  auto cancel_request = [cancel_context](olp::http::NetworkRequest,
                                         olp::http::Network::Payload,
                                         olp::http::Network::Callback,
                                         olp::http::Network::HeaderCallback,
                                         olp::http::Network::DataCallback) {
    std::thread([cancel_context]() {
      cancel_context->CancelOperation();
    }).detach();

    constexpr auto unused_request_id = 5;
    return olp::http::SendOutcome(unused_request_id);
  };

  // Current expectations on NetworkMock will first cancel a response
  // and after each subsequent request with same URL will return correct
  // response. So, no need to clear mock expectations after each scoped trace,
  // because we are testing a method step-by-step with this approach.
  {
    SCOPED_TRACE("Cancelled on getting a config");

    EXPECT_CALL(*network_, Send(IsGetRequest(kConfigRequestUrl), _, _, _, _))
        .WillOnce(cancel_request)
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kConfigHttpResponse));

    auto response =
        client.PublishDataLessThanTwentyMibSync(request, *cancel_context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(olp::client::ErrorCode::Cancelled,
              response.GetError().GetErrorCode());

    *cancel_context = client::CancellationContext{};
  }

  {
    SCOPED_TRACE("Cancelled on retrieving a catalog");

    EXPECT_CALL(*network_, Send(IsGetRequest(kGetCatalogRequest), _, _, _, _))
        .WillOnce(cancel_request)
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kGetCatalogResponse));

    auto response =
        client.PublishDataLessThanTwentyMibSync(request, *cancel_context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(olp::client::ErrorCode::Cancelled,
              response.GetError().GetErrorCode());

    *cancel_context = client::CancellationContext{};
  }

  {
    SCOPED_TRACE("Cancelled on retrieving the ingest API");

    EXPECT_CALL(*network_, Send(IsGetRequest(kIngestRequestUrl), _, _, _, _))
        .WillOnce(cancel_request)
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kIngestHttpResponse));

    auto response =
        client.PublishDataLessThanTwentyMibSync(request, *cancel_context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(olp::client::ErrorCode::Cancelled,
              response.GetError().GetErrorCode());

    *cancel_context = client::CancellationContext{};
  }

  {
    SCOPED_TRACE("Cancelled on posting data via ingest API");

    EXPECT_CALL(*network_,
                Send(IsPostRequest(kPostIngestDataRequest), _, _, _, _))
        .WillOnce(cancel_request);

    auto response =
        client.PublishDataLessThanTwentyMibSync(request, *cancel_context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(olp::client::ErrorCode::Cancelled,
              response.GetError().GetErrorCode());

    *cancel_context = client::CancellationContext{};
  }
}

}  // namespace
