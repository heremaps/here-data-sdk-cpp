/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
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
#include <unordered_set>
#include "StreamLayerClientImpl.h"

namespace {

using namespace testing;
using namespace olp;
using namespace olp::dataservice::write;
using namespace olp::tests::common;

const olp::client::HRN kHrn{"hrn:here:data:::catalog"};
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
    kHrn.ToString() + "/apis/ingest/v1";
const std::string kIngestBaseUrl =
    "https://some.ingest.url/ingest/v1/catalogs/" + kHrn.ToString();
const std::string kIngestHttpResponse =
    R"jsonString([{"api":"ingest","version":"v1","baseURL":")jsonString" +
    kIngestBaseUrl + R"jsonString(","parameters":{}}])jsonString";

const std::string kGetCatalogRequest =
    kConfigBaseUrl + "/catalogs/" + kHrn.ToString();
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

// PublishDataGreaterThan20Mb-specific constants:
const std::string kPublishBaseUrl =
    "https://some.publish.url/catalogs/" + kHrn.ToString();
const std::string kPublishRequestUrl =
    "https://api-lookup.data.api.platform.here.com/lookup/v1/resources/" +
    kHrn.ToString() + "/apis/publish/v2";
const std::string kPublishHttpResponse =
    R"jsonString([{"api":"publish","version":"v2","baseURL":")jsonString" +
    kPublishBaseUrl + R"jsonString(","parameters":{}}])jsonString";

const std::string kBlobBaseUrl =
    "https://some.blob.url/catalogs/" + kHrn.ToString();
const std::string kBlobRequestUrl =
    "https://api-lookup.data.api.platform.here.com/lookup/v1/resources/" +
    kHrn.ToString() + "/apis/blob/v1";
const std::string kBlobHttpResponse =
    R"jsonString([{"api":"blob","version":"v1","baseURL":")jsonString" +
    kBlobBaseUrl + R"jsonString(","parameters":{}}])jsonString";

const std::string kPublicationId = "aa-bbbbb-cccc-ddddd-qqqqq";
const std::string kInitPublicationUrl = kPublishBaseUrl + "/publications";
const std::string kInitPublicationHttpResponse =
    R"jsonString({"catalogId":"catalog","catalogVersion":99999,"details":{}, "id":")jsonString" +
    kPublicationId + R"jsonString(","layerIds":[")jsonString" + kLayerName +
    R"jsonString("]})jsonString";

const std::string kMockedDataHandle = "some-generated-uuid";
const std::string kPutBlobRequestUrl =
    kBlobBaseUrl + "/layers/" + kLayerName + "/data/" + kMockedDataHandle;

const std::string kUploadPartitionRequestUrl = kPublishBaseUrl + "/layers/" +
                                               kLayerName + "/publications/" +
                                               kPublicationId + "/partitions";

const std::string kSubmitPublicationRequestUrl =
    kPublishBaseUrl + "/publications/" + kPublicationId;

class MockStreamLayerClientImpl : public StreamLayerClientImpl {
 public:
  using StreamLayerClientImpl::PublishDataGreaterThanTwentyMib;
  using StreamLayerClientImpl::PublishDataLessThanTwentyMib;
  using StreamLayerClientImpl::StreamLayerClientImpl;

  MOCK_METHOD(PublishSdiiResponse, IngestSdii,
              (model::PublishSdiiRequest request,
               client::CancellationContext context),
              (override));

  MOCK_METHOD(PublishDataResponse, PublishDataTask,
              (model::PublishDataRequest request,
               client::CancellationContext context),
              (override));

  MOCK_METHOD(std::string, GenerateUuid, (), (const, override));
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
      kHrn, StreamLayerClientSettings{}, settings_);

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

TEST_F(StreamLayerClientImplTest, SuccessfullyPublishDataLessThanTwentyMib) {
  auto data = std::make_shared<std::vector<unsigned char>>(1, 'a');
  auto request =
      model::PublishDataRequest().WithData(data).WithLayerId(kLayerName);

  auto settings = settings_;
  settings.cache = nullptr;

  MockStreamLayerClientImpl client{kHrn, StreamLayerClientSettings{}, settings};

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

  auto response = client.PublishDataLessThanTwentyMib(
      request, client::CancellationContext{});
  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_EQ(kPostIngestDataTraceID, response.GetResult().GetTraceID());
}

TEST_F(StreamLayerClientImplTest, FaliedPublishDataLessThanTwentyMib) {
  auto data = std::make_shared<std::vector<unsigned char>>(1, 'a');
  auto request =
      model::PublishDataRequest().WithData(data).WithLayerId(kLayerName);

  auto settings = settings_;
  settings.cache = nullptr;

  MockStreamLayerClientImpl client{kHrn, StreamLayerClientSettings{}, settings};

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

    auto response = client.PublishDataLessThanTwentyMib(
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

    auto response = client.PublishDataLessThanTwentyMib(
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
    auto response = client.PublishDataLessThanTwentyMib(
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

    auto response = client.PublishDataLessThanTwentyMib(
        request, client::CancellationContext{});

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
              response.GetError().GetHttpStatusCode());
    EXPECT_TRUE(response.GetError().GetMessage().empty());
  }

  {
    SCOPED_TRACE("Failed on publishing via ingest API");

    EXPECT_CALL(*network_,
                Send(IsPostRequest(kPostIngestDataRequest), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::BAD_REQUEST),
                               std::string{}));

    auto response = client.PublishDataLessThanTwentyMib(
        request, client::CancellationContext{});

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
              response.GetError().GetHttpStatusCode());
    EXPECT_TRUE(response.GetError().GetMessage().empty());
  }
}

TEST_F(StreamLayerClientImplTest, CancelPublishDataLessThanTwentyMib) {
  auto data = std::make_shared<std::vector<unsigned char>>(1, 'a');
  auto request =
      model::PublishDataRequest().WithData(data).WithLayerId(kLayerName);

  auto settings = settings_;
  settings.cache = nullptr;

  MockStreamLayerClientImpl client{kHrn, StreamLayerClientSettings{}, settings};
  {
    SCOPED_TRACE("Cancelled before publish call");

    client::CancellationContext cancel_context;
    cancel_context.CancelOperation();

    auto response =
        client.PublishDataLessThanTwentyMib(request, cancel_context);

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
        client.PublishDataLessThanTwentyMib(request, *cancel_context);

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
        client.PublishDataLessThanTwentyMib(request, *cancel_context);

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
        client.PublishDataLessThanTwentyMib(request, *cancel_context);

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
        client.PublishDataLessThanTwentyMib(request, *cancel_context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(olp::client::ErrorCode::Cancelled,
              response.GetError().GetErrorCode());

    *cancel_context = client::CancellationContext{};
  }
}

TEST_F(StreamLayerClientImplTest, SuccessfullyPublishDataGreaterThanTwentyMib) {
  auto data = std::make_shared<std::vector<unsigned char>>(1, 'a');
  auto request =
      model::PublishDataRequest().WithData(data).WithLayerId(kLayerName);

  auto settings = settings_;
  settings.cache = nullptr;

  const std::string kMockedPartitionId = "some-generated-partition-uuid";
  MockStreamLayerClientImpl client{kHrn, StreamLayerClientSettings{}, settings};

  // Mock the generated UUIDs for the data handle and partition id
  EXPECT_CALL(client, GenerateUuid)
      .WillOnce(Return(kMockedDataHandle))
      .WillOnce(Return(kMockedPartitionId));

  EXPECT_CALL(*network_, Send(IsGetRequest(kConfigRequestUrl), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kConfigHttpResponse));

  EXPECT_CALL(*network_, Send(IsGetRequest(kGetCatalogRequest), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kGetCatalogResponse));

  EXPECT_CALL(*network_, Send(IsGetRequest(kPublishRequestUrl), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kPublishHttpResponse));

  EXPECT_CALL(*network_, Send(IsGetRequest(kBlobRequestUrl), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kBlobHttpResponse));

  EXPECT_CALL(*network_, Send(IsPostRequest(kInitPublicationUrl), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kInitPublicationHttpResponse));

  EXPECT_CALL(*network_, Send(IsPutRequest(kPutBlobRequestUrl), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   std::string{}));

  EXPECT_CALL(*network_,
              Send(IsPostRequest(kUploadPartitionRequestUrl), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::NO_CONTENT),
                                   std::string{}));

  EXPECT_CALL(*network_,
              Send(IsPutRequest(kSubmitPublicationRequestUrl), _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::NO_CONTENT),
                                   std::string{}));
  auto response = client.PublishDataGreaterThanTwentyMib(
      request, client::CancellationContext{});
  ASSERT_TRUE(response.IsSuccessful());
  ASSERT_EQ(kMockedPartitionId, response.GetResult().GetTraceID());
}

TEST_F(StreamLayerClientImplTest, FailedPublishDataGreaterThanTwentyMib) {
  auto data = std::make_shared<std::vector<unsigned char>>(1, 'a');
  auto request =
      model::PublishDataRequest().WithData(data).WithLayerId(kLayerName);

  auto settings = settings_;
  settings.cache = nullptr;

  const std::string kMockedPartitionId = "some-generated-partition-uuid";
  MockStreamLayerClientImpl client{kHrn, StreamLayerClientSettings{}, settings};

  {
    SCOPED_TRACE("Failed on getting a config API URL");

    EXPECT_CALL(*network_, Send(IsGetRequest(kConfigRequestUrl), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::BAD_REQUEST),
                               std::string{}))
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kConfigHttpResponse));

    auto response = client.PublishDataGreaterThanTwentyMib(
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

    auto response = client.PublishDataGreaterThanTwentyMib(
        request, client::CancellationContext{});
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
              response.GetError().GetHttpStatusCode());
  }

  {
    SCOPED_TRACE("Failed on retrieving catalog with invalid layer");

    auto request2 = request;
    request2.WithLayerId("invalid_layer_id");
    auto response = client.PublishDataGreaterThanTwentyMib(
        request2, client::CancellationContext{});

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(olp::client::ErrorCode::InvalidArgument,
              response.GetError().GetErrorCode());
  }

  {
    SCOPED_TRACE("Failed on getting a publish API URL");

    EXPECT_CALL(*network_, Send(IsGetRequest(kPublishRequestUrl), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::BAD_REQUEST),
                               std::string{}))
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kPublishHttpResponse));

    auto response = client.PublishDataGreaterThanTwentyMib(
        request, client::CancellationContext{});
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
              response.GetError().GetHttpStatusCode());
  }

  {
    SCOPED_TRACE("Failed on getting a blob API URL");

    EXPECT_CALL(*network_, Send(IsGetRequest(kBlobRequestUrl), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::BAD_REQUEST),
                               std::string{}))
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kBlobHttpResponse));

    auto response = client.PublishDataGreaterThanTwentyMib(
        request, client::CancellationContext{});
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
              response.GetError().GetHttpStatusCode());
  }

  {
    SCOPED_TRACE("Failed on init publication");

    EXPECT_CALL(*network_, Send(IsPostRequest(kInitPublicationUrl), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::BAD_REQUEST),
                               std::string{}))
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               kInitPublicationHttpResponse));

    auto response = client.PublishDataGreaterThanTwentyMib(
        request, client::CancellationContext{});
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
              response.GetError().GetHttpStatusCode());
  }

  {
    SCOPED_TRACE("Failed on put blob data");

    EXPECT_CALL(client, GenerateUuid).WillOnce(Return(kMockedDataHandle));

    EXPECT_CALL(*network_, Send(IsPutRequest(kPutBlobRequestUrl), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::BAD_REQUEST),
                               std::string{}))
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::OK),
                               std::string{}));

    auto response = client.PublishDataGreaterThanTwentyMib(
        request, client::CancellationContext{});
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
              response.GetError().GetHttpStatusCode());

    Mock::VerifyAndClearExpectations(&client);
  }

  {
    SCOPED_TRACE("Failed on upload partition blob data");

    EXPECT_CALL(client, GenerateUuid)
        .WillOnce(Return(kMockedDataHandle))
        .WillOnce(Return(kMockedPartitionId));

    EXPECT_CALL(*network_,
                Send(IsPostRequest(kUploadPartitionRequestUrl), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::BAD_REQUEST),
                               std::string{}))
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::NO_CONTENT),
                               std::string{}));

    auto response = client.PublishDataGreaterThanTwentyMib(
        request, client::CancellationContext{});
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
              response.GetError().GetHttpStatusCode());

    Mock::VerifyAndClearExpectations(&client);
  }

  {
    SCOPED_TRACE("Failed on submit publication");

    EXPECT_CALL(client, GenerateUuid)
        .WillOnce(Return(kMockedDataHandle))
        .WillOnce(Return(kMockedPartitionId));

    EXPECT_CALL(*network_,
                Send(IsPutRequest(kSubmitPublicationRequestUrl), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::BAD_REQUEST),
                               std::string{}));

    auto response = client.PublishDataGreaterThanTwentyMib(
        request, client::CancellationContext{});
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(olp::http::HttpStatusCode::BAD_REQUEST,
              response.GetError().GetHttpStatusCode());

    Mock::VerifyAndClearExpectations(&client);
  }
}

TEST_F(StreamLayerClientImplTest, QueueAndFlush) {
  const size_t kBatchSize = 10;
  settings_.cache =
      olp::client::OlpClientSettingsFactory::CreateDefaultCache({});

  auto client = std::make_shared<MockStreamLayerClientImpl>(
      kHrn, StreamLayerClientSettings{}, settings_);

  size_t uuid_call_count = 1;
  // Forward trace ID from request to response
  ON_CALL(*client, PublishDataTask(_, _))
      .WillByDefault(
          [](model::PublishDataRequest request,
             client::CancellationContext /*context*/) -> PublishDataResponse {
            PublishDataResult result;
            result.SetTraceID(request.GetTraceId().get());
            return PublishDataResponse{result};
          });
  ON_CALL(*client, GenerateUuid)
      .WillByDefault([&uuid_call_count]() -> std::string {
        return std::to_string(uuid_call_count++);
      });

  EXPECT_CALL(*client, PublishDataTask(_, _)).Times(kBatchSize);
  EXPECT_CALL(*client, GenerateUuid()).Times(kBatchSize);

  // queues all  requests:
  for (size_t i = 0; i < kBatchSize; ++i) {
    model::PublishDataRequest request =
        model::PublishDataRequest()
            .WithTraceId(std::to_string(i))
            .WithData(std::make_shared<std::vector<unsigned char>>(1, 'z'))
            .WithLayerId("layer");

    auto error = client->Queue(std::move(request));
    EXPECT_EQ(boost::none, error) << *error;
  }

  EXPECT_EQ(client->QueueSize(), kBatchSize);

  // Flush all requests and verify whether responses are successful
  auto response = client->Flush(model::FlushRequest()).GetFuture().get();
  EXPECT_EQ(response.size(), kBatchSize);
  std::unordered_set<std::string> trace_ids;
  for (const auto &r : response) {
    EXPECT_TRUE(r.IsSuccessful());
    trace_ids.insert(r.GetResult().GetTraceID());
  }

  EXPECT_EQ(kBatchSize, trace_ids.size());
}

}  // namespace
