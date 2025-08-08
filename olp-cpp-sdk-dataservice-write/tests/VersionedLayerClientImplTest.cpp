/*
 * Copyright (C) 2020-2021 HERE Europe B.V.
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
#include <olp/authentication/Settings.h>
#include <olp/authentication/TokenProvider.h>
#include <olp/core/client/OlpClientSettingsFactory.h>

// clang-format off
#include "generated/serializer/ApiSerializer.h"
#include "generated/serializer/PublicationSerializer.h"
#include <olp/core/generated/serializer/SerializerWrapper.h>
#include "generated/serializer/JsonSerializer.h"
// clang-format on

#include "VersionedLayerClientImpl.h"
#include "WriteDefaultResponses.h"

namespace {

using testing::_;
using testing::Mock;
using testing::Return;
namespace client = olp::client;
namespace write = olp::dataservice::write;
namespace model = olp::dataservice::write::model;

constexpr auto kAppId = "id";
constexpr auto kAppSecret = "secret";
constexpr auto kLayer = "layer";
const std::string kPublishApiName = "publish";
const auto kHrn = olp::client::HRN{"hrn:here:data:::catalog"};
const auto kLookupPublishApiUrl =
    "https://api-lookup.data.api.platform.here.com/lookup/v1/resources/" +
    kHrn.ToString() + "/apis/publish/v2";
const std::string kPublishUrl =
    "https://tmp.publish.data.api.platform.here.com/publish/v2/catalogs/" +
    kHrn.ToString() + "/publications";
const std::string kUserSigninResponse = R"JSON(
    {"accessToken":"password_grant_token","tokenType":"bearer","expiresIn":3599,"refreshToken":"5j687leur4njgb4osomifn55p0","userId":"HERE-5fa10eda-39ff-4cbc-9b0c-5acba4685649"}
    )JSON";

class VersionedLayerClientImplTest : public ::testing::Test {
 protected:
  void SetUp() override {
    cache_ = std::make_shared<CacheMock>();
    network_ = std::make_shared<NetworkMock>();

    olp::authentication::Settings auth_settings({kAppId, kAppSecret});
    auth_settings.network_request_handler = network_;
    olp::authentication::TokenProviderDefault provider(auth_settings);

    olp::client::AuthenticationSettings auth_client_settings;
    auth_client_settings.token_provider = provider;

    settings_.network_request_handler = network_;
    settings_.cache = cache_;
    settings_.task_scheduler =
        olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);
    settings_.authentication_settings = auth_client_settings;
  }

  void TearDown() override {
    settings_.network_request_handler.reset();
    settings_.cache.reset();

    network_.reset();
    cache_.reset();
  }

  write::model::Apis CreateApiResponse(const std::string& service) {
    const auto apis =
        mockserver::DefaultResponses::GenerateResourceApisResponse(
            kHrn.ToCatalogHRNString());
    auto it = std::find_if(
        apis.begin(), apis.end(),
        [&](const model::Api& obj) -> bool { return obj.GetApi() == service; });

    write::model::Apis result;
    if (it != apis.end()) {
      result.push_back(*it);
    }

    return result;
  }

  std::shared_ptr<CacheMock> cache_;
  std::shared_ptr<NetworkMock> network_;
  olp::client::OlpClientSettings settings_;
};

TEST_F(VersionedLayerClientImplTest, StartBatch) {
  const auto catalog = kHrn.ToCatalogHRNString();
  const auto api = CreateApiResponse(kPublishApiName);
  const auto publication =
      mockserver::DefaultResponses::GeneratePublicationResponse({kLayer}, {});
  ASSERT_FALSE(api.empty());

  // auth token should be valid till the end of all tests
  EXPECT_CALL(
      *network_,
      Send(IsPostRequest(olp::authentication::kHereAccountProductionTokenUrl),
           _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kUserSigninResponse));

  {
    SCOPED_TRACE("Successful request, future");

    EXPECT_CALL(*network_, Send(IsGetRequest(kLookupPublishApiUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     olp::serializer::serialize(api)));
    EXPECT_CALL(*network_, Send(IsPostRequest(kPublishUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     olp::serializer::serialize(publication)));

    // mock apis caching
    EXPECT_CALL(*cache_, Get(_, _)).Times(1);
    EXPECT_CALL(*cache_, Put(_, _, _, _))
        .WillOnce([](const std::string& /*key*/, const boost::any& /*value*/,
                     const olp::cache::Encoder& /*encoder*/,
                     time_t /*expiry*/) { return true; });

    write::VersionedLayerClientImpl write_client(kHrn, settings_);
    const auto batch_request = model::StartBatchRequest().WithLayers({kLayer});
    auto future = write_client.StartBatch(batch_request).GetFuture();

    const auto response = future.get();
    const auto& result = response.GetResult();

    EXPECT_TRUE(response.IsSuccessful());
    ASSERT_TRUE(result.GetId());
    ASSERT_TRUE(result.GetDetails());
    ASSERT_TRUE(result.GetLayerIds());
    ASSERT_EQ(result.GetLayerIds()->size(), 1);
    ASSERT_EQ(result.GetLayerIds()->front(), kLayer);
    ASSERT_NE("", response.GetResult().GetId().value());
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Successful request, callback");

    EXPECT_CALL(*network_, Send(IsGetRequest(kLookupPublishApiUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     olp::serializer::serialize(api)));
    EXPECT_CALL(*network_, Send(IsPostRequest(kPublishUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     olp::serializer::serialize(publication)));

    // mock apis caching
    EXPECT_CALL(*cache_, Get(_, _)).Times(1);
    EXPECT_CALL(*cache_, Put(_, _, _, _))
        .WillOnce([](const std::string& /*key*/, const boost::any& /*value*/,
                     const olp::cache::Encoder& /*encoder*/,
                     time_t /*expiry*/) { return true; });

    std::promise<write::StartBatchResponse> promise;
    write::VersionedLayerClientImpl write_client(kHrn, settings_);
    const auto batch_request = model::StartBatchRequest().WithLayers({kLayer});
    const auto token = write_client.StartBatch(
        batch_request, [&promise](write::StartBatchResponse response) {
          promise.set_value(std::move(response));
        });

    auto future = promise.get_future();
    const auto response = future.get();
    const auto& result = response.GetResult();

    EXPECT_TRUE(response.IsSuccessful());
    ASSERT_TRUE(result.GetId());
    ASSERT_TRUE(result.GetDetails());
    ASSERT_TRUE(result.GetLayerIds());
    ASSERT_EQ(result.GetLayerIds()->size(), 1);
    ASSERT_EQ(result.GetLayerIds()->front(), kLayer);
    ASSERT_NE("", response.GetResult().GetId().value());
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Apis load from cache");

    EXPECT_CALL(*network_, Send(IsPostRequest(kPublishUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     olp::serializer::serialize(publication)));

    // mock apis caching
    EXPECT_CALL(*cache_, Get(_, _))
        .WillOnce([&api](const std::string&, const olp::cache::Decoder&) {
          return api[0].GetBaseUrl();
        });

    write::VersionedLayerClientImpl write_client(kHrn, settings_);
    const auto batch_request = model::StartBatchRequest().WithLayers({kLayer});
    auto future = write_client.StartBatch(batch_request).GetFuture();

    const auto response = future.get();
    const auto& result = response.GetResult();

    EXPECT_TRUE(response.IsSuccessful());
    ASSERT_TRUE(result.GetId());
    ASSERT_TRUE(result.GetDetails());
    ASSERT_TRUE(result.GetLayerIds());
    ASSERT_EQ(result.GetLayerIds()->size(), 1);
    ASSERT_EQ(result.GetLayerIds()->front(), kLayer);
    ASSERT_NE("", response.GetResult().GetId().value());
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("No layer");
    write::VersionedLayerClientImpl write_client(kHrn, settings_);
    const auto batch_request = model::StartBatchRequest();
    auto future = write_client.StartBatch(batch_request).GetFuture();

    const auto response = future.get();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              client::ErrorCode::InvalidArgument);
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Empty layers array");
    write::VersionedLayerClientImpl write_client(kHrn, settings_);
    const auto batch_request = model::StartBatchRequest().WithLayers({});
    auto future = write_client.StartBatch(batch_request).GetFuture();

    const auto response = future.get();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              client::ErrorCode::InvalidArgument);
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }
}

TEST_F(VersionedLayerClientImplTest, StartBatchCancel) {
  const auto catalog = kHrn.ToCatalogHRNString();

  // auth token should be valid till the end of all test cases
  EXPECT_CALL(
      *network_,
      Send(IsPostRequest(olp::authentication::kHereAccountProductionTokenUrl),
           _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kUserSigninResponse));

  {
    SCOPED_TRACE("Cancel");

    const auto apis =
        mockserver::DefaultResponses::GenerateResourceApisResponse(catalog);
    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    auto wait_for_cancel = std::make_shared<std::promise<void>>();
    auto pause_for_cancel = std::make_shared<std::promise<void>>();

    std::tie(request_id, send_mock, cancel_mock) =
        GenerateNetworkMockActions(wait_for_cancel, pause_for_cancel,
                                   {olp::http::HttpStatusCode::OK,
                                    olp::serializer::serialize(apis).c_str()});

    EXPECT_CALL(*network_, Send(IsGetRequest(kLookupPublishApiUrl), _, _, _, _))
        .WillOnce(testing::Invoke(std::move(send_mock)));
    EXPECT_CALL(*network_, Cancel(_))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));

    // mock apis caching
    EXPECT_CALL(*cache_, Get(_, _)).Times(1);

    std::promise<write::StartBatchResponse> promise;
    write::VersionedLayerClientImpl write_client(kHrn, settings_);
    const auto batch_request = model::StartBatchRequest().WithLayers({kLayer});
    const auto token = write_client.StartBatch(
        batch_request, [&promise](write::StartBatchResponse response) {
          promise.set_value(std::move(response));
        });

    wait_for_cancel->get_future().get();
    token.Cancel();
    pause_for_cancel->set_value();

    auto future = promise.get_future();
    const auto response = future.get();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("On client deletion");

    const auto apis =
        mockserver::DefaultResponses::GenerateResourceApisResponse(catalog);
    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    auto wait_for_cancel = std::make_shared<std::promise<void>>();
    auto pause_for_cancel = std::make_shared<std::promise<void>>();

    std::tie(request_id, send_mock, cancel_mock) =
        GenerateNetworkMockActions(wait_for_cancel, pause_for_cancel,
                                   {olp::http::HttpStatusCode::OK,
                                    olp::serializer::serialize(apis).c_str()});

    EXPECT_CALL(*network_, Send(IsGetRequest(kLookupPublishApiUrl), _, _, _, _))
        .WillOnce(testing::Invoke(std::move(send_mock)));
    EXPECT_CALL(*network_, Cancel(_))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));

    // mock apis caching
    EXPECT_CALL(*cache_, Get(_, _)).Times(1);

    auto write_client =
        std::make_shared<write::VersionedLayerClientImpl>(kHrn, settings_);
    const auto batch_request = model::StartBatchRequest().WithLayers({kLayer});
    auto future = write_client->StartBatch(batch_request).GetFuture();

    wait_for_cancel->get_future().get();
    write_client.reset();
    pause_for_cancel->set_value();

    const auto response = future.get();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Cancellable future");

    const auto apis =
        mockserver::DefaultResponses::GenerateResourceApisResponse(catalog);
    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    auto wait_for_cancel = std::make_shared<std::promise<void>>();
    auto pause_for_cancel = std::make_shared<std::promise<void>>();

    std::tie(request_id, send_mock, cancel_mock) =
        GenerateNetworkMockActions(wait_for_cancel, pause_for_cancel,
                                   {olp::http::HttpStatusCode::OK,
                                    olp::serializer::serialize(apis).c_str()});

    EXPECT_CALL(*network_, Send(_, _, _, _, _))
        .WillOnce(testing::Invoke(std::move(send_mock)));
    EXPECT_CALL(*network_, Cancel(_))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));

    // mock apis caching
    EXPECT_CALL(*cache_, Get(_, _)).Times(1);

    write::VersionedLayerClientImpl write_client(kHrn, settings_);
    const auto batch_request = model::StartBatchRequest().WithLayers({kLayer});
    const auto cancellable = write_client.StartBatch(batch_request);
    auto token = cancellable.GetCancellationToken();

    wait_for_cancel->get_future().get();
    token.Cancel();
    pause_for_cancel->set_value();

    const auto response = cancellable.GetFuture().get();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }
}

TEST_F(VersionedLayerClientImplTest, CompleteBatch) {
  const auto catalog = kHrn.ToCatalogHRNString();
  const auto api = CreateApiResponse(kPublishApiName);
  const auto publication =
      mockserver::DefaultResponses::GeneratePublicationResponse({kLayer}, {});
  ASSERT_FALSE(api.empty());
  ASSERT_TRUE(publication.GetId());

  const auto publication_publish_url = kPublishUrl + "/" + *publication.GetId();

  // auth token should be valid till the end of all tests
  EXPECT_CALL(
      *network_,
      Send(IsPostRequest(olp::authentication::kHereAccountProductionTokenUrl),
           _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kUserSigninResponse));

  {
    SCOPED_TRACE("Successful request, future");

    EXPECT_CALL(*network_, Send(IsGetRequest(kLookupPublishApiUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     olp::serializer::serialize(api)));
    EXPECT_CALL(*network_,
                Send(IsPutRequest(publication_publish_url), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::NO_CONTENT),
                                     {}));

    // mock apis caching
    EXPECT_CALL(*cache_, Get(_, _)).Times(1);
    EXPECT_CALL(*cache_, Put(_, _, _, _))
        .WillOnce([](const std::string& /*key*/, const boost::any& /*value*/,
                     const olp::cache::Encoder& /*encoder*/,
                     time_t /*expiry*/) { return true; });

    write::VersionedLayerClientImpl write_client(kHrn, settings_);
    const auto batch_request = model::StartBatchRequest().WithLayers({kLayer});
    auto future = write_client.CompleteBatch(publication).GetFuture();

    const auto response = future.get();

    EXPECT_TRUE(response.IsSuccessful());
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Successful request, callback");

    EXPECT_CALL(*network_, Send(IsGetRequest(kLookupPublishApiUrl), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     olp::serializer::serialize(api)));
    EXPECT_CALL(*network_,
                Send(IsPutRequest(publication_publish_url), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::NO_CONTENT),
                                     {}));

    // mock apis caching
    EXPECT_CALL(*cache_, Get(_, _)).Times(1);
    EXPECT_CALL(*cache_, Put(_, _, _, _))
        .WillOnce([](const std::string& /*key*/, const boost::any& /*value*/,
                     const olp::cache::Encoder& /*encoder*/,
                     time_t /*expiry*/) { return true; });

    std::promise<write::CompleteBatchResponse> promise;
    write::VersionedLayerClientImpl write_client(kHrn, settings_);
    const auto batch_request = model::StartBatchRequest().WithLayers({kLayer});
    const auto token = write_client.CompleteBatch(
        publication, [&promise](write::CompleteBatchResponse response) {
          promise.set_value(std::move(response));
        });

    auto future = promise.get_future();
    const auto response = future.get();

    EXPECT_TRUE(response.IsSuccessful());
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Apis load from cache");

    EXPECT_CALL(*network_,
                Send(IsPutRequest(publication_publish_url), _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::NO_CONTENT),
                                     {}));

    // mock apis caching
    EXPECT_CALL(*cache_, Get(_, _))
        .WillOnce([&api](const std::string&, const olp::cache::Decoder&) {
          return api[0].GetBaseUrl();
        });

    write::VersionedLayerClientImpl write_client(kHrn, settings_);
    const auto batch_request = model::StartBatchRequest().WithLayers({kLayer});
    auto future = write_client.CompleteBatch(publication).GetFuture();

    const auto response = future.get();

    EXPECT_TRUE(response.IsSuccessful());
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("No publication id");
    model::Publication invalid_publication;

    write::VersionedLayerClientImpl write_client(kHrn, settings_);
    const auto batch_request = model::StartBatchRequest().WithLayers({kLayer});
    auto future = write_client.CompleteBatch(invalid_publication).GetFuture();

    const auto response = future.get();
    const auto& error = response.GetError();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(error.GetErrorCode(), client::ErrorCode::InvalidArgument);
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }
}

TEST_F(VersionedLayerClientImplTest, CompleteBatchCancel) {
  const auto catalog = kHrn.ToCatalogHRNString();
  const auto apis =
      mockserver::DefaultResponses::GenerateResourceApisResponse(catalog);
  const auto apis_response_str = olp::serializer::serialize(apis);
  const auto apis_response = apis_response_str.c_str();
  const auto publication =
      mockserver::DefaultResponses::GeneratePublicationResponse({kLayer}, {});
  ASSERT_TRUE(publication.GetId());

  // auth token should be valid till the end of all test cases
  EXPECT_CALL(
      *network_,
      Send(IsPostRequest(olp::authentication::kHereAccountProductionTokenUrl),
           _, _, _, _))
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kUserSigninResponse));

  {
    SCOPED_TRACE("Cancel");

    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    auto wait_for_cancel = std::make_shared<std::promise<void>>();
    auto pause_for_cancel = std::make_shared<std::promise<void>>();

    std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
        wait_for_cancel, pause_for_cancel,
        {olp::http::HttpStatusCode::OK, apis_response});

    EXPECT_CALL(*network_, Send(IsGetRequest(kLookupPublishApiUrl), _, _, _, _))
        .WillOnce(testing::Invoke(std::move(send_mock)));
    EXPECT_CALL(*network_, Cancel(_))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));

    // mock apis caching
    EXPECT_CALL(*cache_, Get(_, _)).Times(1);

    std::promise<write::CompleteBatchResponse> promise;
    write::VersionedLayerClientImpl write_client(kHrn, settings_);
    const auto token = write_client.CompleteBatch(
        publication, [&promise](write::CompleteBatchResponse response) {
          promise.set_value(std::move(response));
        });

    wait_for_cancel->get_future().get();
    token.Cancel();
    pause_for_cancel->set_value();

    auto future = promise.get_future();
    const auto response = future.get();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("On client deletion");

    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    auto wait_for_cancel = std::make_shared<std::promise<void>>();
    auto pause_for_cancel = std::make_shared<std::promise<void>>();

    std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
        wait_for_cancel, pause_for_cancel,
        {olp::http::HttpStatusCode::OK, apis_response});

    EXPECT_CALL(*network_, Send(IsGetRequest(kLookupPublishApiUrl), _, _, _, _))
        .WillOnce(testing::Invoke(std::move(send_mock)));
    EXPECT_CALL(*network_, Cancel(_))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));

    // mock apis caching
    EXPECT_CALL(*cache_, Get(_, _)).Times(1);

    auto write_client =
        std::make_shared<write::VersionedLayerClientImpl>(kHrn, settings_);
    auto future = write_client->CompleteBatch(publication).GetFuture();

    wait_for_cancel->get_future().get();
    write_client.reset();
    pause_for_cancel->set_value();

    const auto response = future.get();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Cancellable future");

    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    auto wait_for_cancel = std::make_shared<std::promise<void>>();
    auto pause_for_cancel = std::make_shared<std::promise<void>>();

    std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
        wait_for_cancel, pause_for_cancel,
        {olp::http::HttpStatusCode::OK, apis_response});

    EXPECT_CALL(*network_, Send(_, _, _, _, _))
        .WillOnce(testing::Invoke(std::move(send_mock)));
    EXPECT_CALL(*network_, Cancel(_))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));

    // mock apis caching
    EXPECT_CALL(*cache_, Get(_, _)).Times(1);

    write::VersionedLayerClientImpl write_client(kHrn, settings_);
    const auto cancellable = write_client.CompleteBatch(publication);
    auto token = cancellable.GetCancellationToken();

    wait_for_cancel->get_future().get();
    token.Cancel();
    pause_for_cancel->set_value();

    const auto response = cancellable.GetFuture().get();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }
}

}  // namespace
