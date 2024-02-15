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
#include <mocks/CacheMock.h>
#include <mocks/NetworkMock.h>
#include <olp/authentication/Settings.h>
#include <olp/authentication/TokenProvider.h>
#include <olp/core/client/OlpClientSettingsFactory.h>

// clang-format off
#include "generated/serializer/ApiSerializer.h"
#include "generated/serializer/PublicationSerializer.h"
#include "generated/serializer/CatalogSerializer.h"
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
const auto kHrn = olp::client::HRN{"hrn:here:data:::catalog"};

const std::string kUserSigninResponse = R"JSON(
    {"accessToken":"password_grant_token","tokenType":"bearer","expiresIn":3599,"refreshToken":"5j687leur4njgb4osomifn55p0","userId":"HERE-5fa10eda-39ff-4cbc-9b0c-5acba4685649"}
    )JSON";

class VersionedLayerClientImplPublishToBatchTest : public ::testing::Test {
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

    // auth token should be valid till the end of all tests
    MockAuth();
  }

  void TearDown() override {
    settings_.network_request_handler.reset();
    settings_.cache.reset();

    network_.reset();
    cache_.reset();
  }

  void MockAuth() {
    EXPECT_CALL(
        *network_,
        Send(IsPostRequest(olp::authentication::kHereAccountProductionTokenUrl),
             _, _, _, _))
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kUserSigninResponse));
  }

  model::Catalog MockConfigRequest(
      const std::string& layer_id, int status = olp::http::HttpStatusCode::OK,
      const std::string& content_type = "content_type",
      const std::string& content_encoding = "encoding") {
    const auto hrn_str = kHrn.ToCatalogHRNString();
    model::Catalog catalog;
    model::Layer layer;
    layer.SetId(layer_id);
    layer.SetContentType(content_type);
    layer.SetContentEncoding(content_encoding);
    catalog.SetLayers({layer});

    auto config_api = MockApiRequest("config");
    EXPECT_CALL(*network_, Send(IsGetRequest(config_api.GetBaseUrl() +
                                             "/catalogs/" + hrn_str),
                                _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(status),
                               olp::serializer::serialize(catalog)));

    return catalog;
  }

  void MockUploadBlobRequest(
      const std::string& layer_id,
      int status = olp::http::HttpStatusCode::NO_CONTENT) {
    auto blob_api = MockApiRequest("blob");
    const std::string url_prefix =
        blob_api.GetBaseUrl() + "/layers/" + layer_id + "/data/";

    EXPECT_CALL(*network_, Send(IsPutRequestPrefix(url_prefix), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(status), {}));
  }

  void MockPublishPartitionRequest(
      const model::Publication& publication, const std::string& layer,
      int status = olp::http::HttpStatusCode::NO_CONTENT) {
    auto api = MockApiRequest("publish");
    const std::string url = api.GetBaseUrl() + "/layers/" + layer +
                            "/publications/" + publication.GetId().get() +
                            "/partitions";

    EXPECT_CALL(*network_, Send(IsPostRequest(url), _, _, _, _))
        .WillOnce(ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(status), {}));
  }

  model::Api MockApiRequest(const std::string& service,
                            int status = olp::http::HttpStatusCode::OK) {
    const auto hrn_str = kHrn.ToCatalogHRNString();
    const auto apis = CreateApiResponse(service);
    const auto& service_api = apis[0];
    std::string url =
        "https://api-lookup.data.api.platform.here.com/lookup/v1/" +
        (service == "config" ? "platform" : "resources/" + hrn_str) + "/apis/" +
        service + "/" + service_api.GetVersion();

    EXPECT_CALL(*network_, Send(IsGetRequest(url), _, _, _, _))
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(status),
                               olp::serializer::serialize(apis)));

    return service_api;
  }

  write::model::Apis CreateApiResponse(const std::string& service) {
    auto apis = mockserver::DefaultResponses::GenerateResourceApisResponse(
        kHrn.ToCatalogHRNString());
    auto platform_apis =
        mockserver::DefaultResponses::GeneratePlatformApisResponse();
    apis.insert(apis.end(), platform_apis.begin(), platform_apis.end());

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

TEST_F(VersionedLayerClientImplPublishToBatchTest, PublishToBatch) {
  const auto catalog = kHrn.ToCatalogHRNString();
  const std::string partition = "132";
  const auto publication =
      mockserver::DefaultResponses::GeneratePublicationResponse({kLayer}, {});

  {
    SCOPED_TRACE("Successful request, future");

    MockConfigRequest(kLayer);
    MockUploadBlobRequest(kLayer);
    MockPublishPartitionRequest(publication, kLayer);

    EXPECT_CALL(*cache_, Get(_, _)).Times(3);
    EXPECT_CALL(*cache_, Contains(_)).Times(1);
    EXPECT_CALL(*cache_, Put(_, _, _, _))
        .WillRepeatedly([](const std::string& /*key*/,
                           const boost::any& /*value*/,
                           const olp::cache::Encoder& /*encoder*/,
                           time_t /*expiry*/) { return true; });

    write::VersionedLayerClientImpl client(kHrn, settings_);
    auto request =
        model::PublishPartitionDataRequest()
            .WithData(std::make_shared<std::vector<unsigned char>>(20, 0x30))
            .WithLayerId(kLayer)
            .WithPartitionId(partition);
    auto future = client.PublishToBatch(publication, request).GetFuture();

    const auto response = future.get();
    const auto& result = response.GetResult();

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(result.GetTraceID(), partition);
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Successful request, callback");

    MockConfigRequest(kLayer);
    MockUploadBlobRequest(kLayer);
    MockPublishPartitionRequest(publication, kLayer);

    EXPECT_CALL(*cache_, Get(_, _)).Times(3);
    EXPECT_CALL(*cache_, Contains(_)).Times(1);
    EXPECT_CALL(*cache_, Put(_, _, _, _))
        .WillRepeatedly([](const std::string& /*key*/,
                           const boost::any& /*value*/,
                           const olp::cache::Encoder& /*encoder*/,
                           time_t /*expiry*/) { return true; });

    std::promise<write::PublishPartitionDataResponse> promise;
    auto future = promise.get_future();
    auto request =
        model::PublishPartitionDataRequest()
            .WithData(std::make_shared<std::vector<unsigned char>>(20, 0x30))
            .WithLayerId(kLayer)
            .WithPartitionId(partition);

    write::VersionedLayerClientImpl client(kHrn, settings_);
    client.PublishToBatch(
        publication, request,
        [&promise](write::PublishPartitionDataResponse response) {
          promise.set_value(std::move(response));
        });

    const auto response = future.get();
    const auto& result = response.GetResult();

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(result.GetTraceID(), partition);
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Publication without id");

    write::VersionedLayerClientImpl client(kHrn, settings_);
    auto request =
        model::PublishPartitionDataRequest()
            .WithData(std::make_shared<std::vector<unsigned char>>(20, 0x30))
            .WithLayerId(kLayer)
            .WithPartitionId(partition);
    auto future = client.PublishToBatch({}, request).GetFuture();

    const auto response = future.get();
    const auto& error = response.GetError();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(error.GetErrorCode(), client::ErrorCode::InvalidArgument);
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Reuqest without layer");

    write::VersionedLayerClientImpl client(kHrn, settings_);
    auto request =
        model::PublishPartitionDataRequest()
            .WithData(std::make_shared<std::vector<unsigned char>>(20, 0x30))
            .WithPartitionId(partition);
    auto future = client.PublishToBatch(publication, request).GetFuture();

    const auto response = future.get();
    const auto& error = response.GetError();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(error.GetErrorCode(), client::ErrorCode::InvalidArgument);
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Invalid layer name");

    MockConfigRequest(kLayer);

    // mock catalog caching
    EXPECT_CALL(*cache_, Get(_, _)).Times(1);
    EXPECT_CALL(*cache_, Contains(_)).Times(1);
    EXPECT_CALL(*cache_, Put(_, _, _, _))
        .WillRepeatedly([](const std::string& /*key*/,
                           const boost::any& /*value*/,
                           const olp::cache::Encoder& /*encoder*/,
                           time_t /*expiry*/) { return true; });

    write::VersionedLayerClientImpl client(kHrn, settings_);
    auto request =
        model::PublishPartitionDataRequest()
            .WithData(std::make_shared<std::vector<unsigned char>>(20, 0x30))
            .WithLayerId("invalid_layer")
            .WithPartitionId(partition);
    auto future = client.PublishToBatch(publication, request).GetFuture();

    const auto response = future.get();
    const auto& error = response.GetError();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(error.GetErrorCode(), client::ErrorCode::InvalidArgument);
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Request without data and partition");

    MockConfigRequest(kLayer);
    MockUploadBlobRequest(kLayer);
    MockPublishPartitionRequest(publication, kLayer);

    // mock apis caching
    EXPECT_CALL(*cache_, Get(_, _)).Times(3);
    EXPECT_CALL(*cache_, Contains(_)).Times(1);
    EXPECT_CALL(*cache_, Put(_, _, _, _))
        .WillRepeatedly([](const std::string& /*key*/,
                           const boost::any& /*value*/,
                           const olp::cache::Encoder& /*encoder*/,
                           time_t /*expiry*/) { return true; });

    write::VersionedLayerClientImpl client(kHrn, settings_);
    auto request = model::PublishPartitionDataRequest().WithLayerId(kLayer);
    auto future = client.PublishToBatch(publication, request).GetFuture();

    const auto response = future.get();

    EXPECT_TRUE(response.IsSuccessful());
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Invalid layer settings");

    MockConfigRequest(kLayer, olp::http::HttpStatusCode::OK, {}, {});

    EXPECT_CALL(*cache_, Get(_, _)).Times(1);
    EXPECT_CALL(*cache_, Contains(_)).Times(1);
    EXPECT_CALL(*cache_, Put(_, _, _, _))
        .WillRepeatedly([](const std::string& /*key*/,
                           const boost::any& /*value*/,
                           const olp::cache::Encoder& /*encoder*/,
                           time_t /*expiry*/) { return true; });

    write::VersionedLayerClientImpl client(kHrn, settings_);
    auto request =
        model::PublishPartitionDataRequest()
            .WithData(std::make_shared<std::vector<unsigned char>>(20, 0x30))
            .WithLayerId(kLayer)
            .WithPartitionId(partition);
    auto future = client.PublishToBatch(publication, request).GetFuture();

    const auto response = future.get();
    const auto& error = response.GetError();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(error.GetErrorCode(), client::ErrorCode::InvalidArgument);
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }
}

TEST_F(VersionedLayerClientImplPublishToBatchTest, NetworkErrors) {
  const auto catalog = kHrn.ToCatalogHRNString();
  const auto mock_error = olp::http::HttpStatusCode::BAD_REQUEST;
  const std::string partition = "132";
  const auto publication =
      mockserver::DefaultResponses::GeneratePublicationResponse({kLayer}, {});

  {
    SCOPED_TRACE("Publish partition fail");

    MockConfigRequest(kLayer);
    MockUploadBlobRequest(kLayer);
    MockPublishPartitionRequest(publication, kLayer, mock_error);

    EXPECT_CALL(*cache_, Get(_, _)).Times(3);
    EXPECT_CALL(*cache_, Contains(_)).Times(1);
    EXPECT_CALL(*cache_, Put(_, _, _, _))
        .WillRepeatedly([](const std::string& /*key*/,
                           const boost::any& /*value*/,
                           const olp::cache::Encoder& /*encoder*/,
                           time_t /*expiry*/) { return true; });

    write::VersionedLayerClientImpl client(kHrn, settings_);
    auto request =
        model::PublishPartitionDataRequest()
            .WithData(std::make_shared<std::vector<unsigned char>>(20, 0x30))
            .WithLayerId(kLayer)
            .WithPartitionId(partition);
    auto future = client.PublishToBatch(publication, request).GetFuture();

    const auto response = future.get();
    const auto& error = response.GetError();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(error.GetHttpStatusCode(), mock_error);
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Publish api fail");

    MockConfigRequest(kLayer);
    MockUploadBlobRequest(kLayer);
    MockApiRequest("publish", mock_error);

    EXPECT_CALL(*cache_, Get(_, _)).Times(3);
    EXPECT_CALL(*cache_, Contains(_)).Times(1);
    EXPECT_CALL(*cache_, Put(_, _, _, _))
        .WillRepeatedly([](const std::string& /*key*/,
                           const boost::any& /*value*/,
                           const olp::cache::Encoder& /*encoder*/,
                           time_t /*expiry*/) { return true; });

    write::VersionedLayerClientImpl client(kHrn, settings_);
    auto request =
        model::PublishPartitionDataRequest()
            .WithData(std::make_shared<std::vector<unsigned char>>(20, 0x30))
            .WithLayerId(kLayer)
            .WithPartitionId(partition);
    auto future = client.PublishToBatch(publication, request).GetFuture();

    const auto response = future.get();
    const auto& error = response.GetError();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(error.GetHttpStatusCode(), mock_error);
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Upload blob fail");

    MockConfigRequest(kLayer);
    MockUploadBlobRequest(kLayer, mock_error);

    EXPECT_CALL(*cache_, Get(_, _)).Times(2);
    EXPECT_CALL(*cache_, Contains(_)).Times(1);
    EXPECT_CALL(*cache_, Put(_, _, _, _))
        .WillRepeatedly([](const std::string& /*key*/,
                           const boost::any& /*value*/,
                           const olp::cache::Encoder& /*encoder*/,
                           time_t /*expiry*/) { return true; });

    write::VersionedLayerClientImpl client(kHrn, settings_);
    auto request =
        model::PublishPartitionDataRequest()
            .WithData(std::make_shared<std::vector<unsigned char>>(20, 0x30))
            .WithLayerId(kLayer)
            .WithPartitionId(partition);
    auto future = client.PublishToBatch(publication, request).GetFuture();

    const auto response = future.get();
    const auto& error = response.GetError();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(error.GetHttpStatusCode(), mock_error);
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Blob api fail");

    MockConfigRequest(kLayer);
    MockApiRequest("blob", mock_error);

    EXPECT_CALL(*cache_, Get(_, _)).Times(2);
    EXPECT_CALL(*cache_, Contains(_)).Times(1);
    EXPECT_CALL(*cache_, Put(_, _, _, _))
        .WillRepeatedly([](const std::string& /*key*/,
                           const boost::any& /*value*/,
                           const olp::cache::Encoder& /*encoder*/,
                           time_t /*expiry*/) { return true; });

    write::VersionedLayerClientImpl client(kHrn, settings_);
    auto request =
        model::PublishPartitionDataRequest()
            .WithData(std::make_shared<std::vector<unsigned char>>(20, 0x30))
            .WithLayerId(kLayer)
            .WithPartitionId(partition);
    auto future = client.PublishToBatch(publication, request).GetFuture();

    const auto response = future.get();
    const auto& error = response.GetError();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(error.GetHttpStatusCode(), mock_error);
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Get config fail");

    MockConfigRequest(kLayer, mock_error);

    EXPECT_CALL(*cache_, Get(_, _)).Times(1);
    EXPECT_CALL(*cache_, Contains(_)).Times(1);
    EXPECT_CALL(*cache_, Put(_, _, _, _))
        .WillRepeatedly([](const std::string& /*key*/,
                           const boost::any& /*value*/,
                           const olp::cache::Encoder& /*encoder*/,
                           time_t /*expiry*/) { return true; });

    write::VersionedLayerClientImpl client(kHrn, settings_);
    auto request =
        model::PublishPartitionDataRequest()
            .WithData(std::make_shared<std::vector<unsigned char>>(20, 0x30))
            .WithLayerId(kLayer)
            .WithPartitionId(partition);
    auto future = client.PublishToBatch(publication, request).GetFuture();

    const auto response = future.get();
    const auto& error = response.GetError();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(error.GetHttpStatusCode(), mock_error);
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Config api fail");

    MockApiRequest("config", mock_error);

    EXPECT_CALL(*cache_, Get(_, _)).Times(1);
    EXPECT_CALL(*cache_, Contains(_)).Times(1);

    write::VersionedLayerClientImpl client(kHrn, settings_);
    auto request =
        model::PublishPartitionDataRequest()
            .WithData(std::make_shared<std::vector<unsigned char>>(20, 0x30))
            .WithLayerId(kLayer)
            .WithPartitionId(partition);
    auto future = client.PublishToBatch(publication, request).GetFuture();

    const auto response = future.get();
    const auto& error = response.GetError();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(error.GetHttpStatusCode(), mock_error);
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }
}

TEST_F(VersionedLayerClientImplPublishToBatchTest, Cancel) {
  const auto catalog = kHrn.ToCatalogHRNString();
  const std::string partition = "132";
  const auto publication =
      mockserver::DefaultResponses::GeneratePublicationResponse({kLayer}, {});

  {
    SCOPED_TRACE("Callback");

    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    auto wait_for_cancel = std::make_shared<std::promise<void>>();
    auto pause_for_cancel = std::make_shared<std::promise<void>>();

    std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
        wait_for_cancel, pause_for_cancel, {olp::http::HttpStatusCode::OK, {}});

    MockConfigRequest(kLayer);
    auto api = MockApiRequest("blob");

    EXPECT_CALL(*network_,
                Send(IsPutRequestPrefix(api.GetBaseUrl()), _, _, _, _))
        .WillOnce(testing::Invoke(std::move(send_mock)));
    EXPECT_CALL(*network_, Cancel(_))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));

    EXPECT_CALL(*cache_, Get(_, _)).Times(2);
    EXPECT_CALL(*cache_, Contains(_)).Times(1);
    EXPECT_CALL(*cache_, Put(_, _, _, _))
        .WillRepeatedly([](const std::string& /*key*/,
                           const boost::any& /*value*/,
                           const olp::cache::Encoder& /*encoder*/,
                           time_t /*expiry*/) { return true; });

    std::promise<write::PublishPartitionDataResponse> promise;
    auto request =
        model::PublishPartitionDataRequest()
            .WithData(std::make_shared<std::vector<unsigned char>>(20, 0x30))
            .WithLayerId(kLayer)
            .WithPartitionId(partition);

    write::VersionedLayerClientImpl client(kHrn, settings_);
    const auto token = client.PublishToBatch(
        publication, request,
        [&promise](write::PublishPartitionDataResponse response) {
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
    SCOPED_TRACE("Future");

    olp::http::RequestId request_id;
    NetworkCallback send_mock;
    CancelCallback cancel_mock;

    auto wait_for_cancel = std::make_shared<std::promise<void>>();
    auto pause_for_cancel = std::make_shared<std::promise<void>>();

    std::tie(request_id, send_mock, cancel_mock) = GenerateNetworkMockActions(
        wait_for_cancel, pause_for_cancel, {olp::http::HttpStatusCode::OK, {}});

    MockConfigRequest(kLayer);
    auto api = MockApiRequest("blob");

    EXPECT_CALL(*network_,
                Send(IsPutRequestPrefix(api.GetBaseUrl()), _, _, _, _))
        .WillOnce(testing::Invoke(std::move(send_mock)));
    EXPECT_CALL(*network_, Cancel(_))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));

    EXPECT_CALL(*cache_, Get(_, _)).Times(2);
    EXPECT_CALL(*cache_, Contains(_)).Times(1);
    EXPECT_CALL(*cache_, Put(_, _, _, _))
        .WillRepeatedly([](const std::string& /*key*/,
                           const boost::any& /*value*/,
                           const olp::cache::Encoder& /*encoder*/,
                           time_t /*expiry*/) { return true; });

    auto request =
        model::PublishPartitionDataRequest()
            .WithData(std::make_shared<std::vector<unsigned char>>(20, 0x30))
            .WithLayerId(kLayer)
            .WithPartitionId(partition);

    write::VersionedLayerClientImpl client(kHrn, settings_);
    const auto cancellable = client.PublishToBatch(publication, request);
    auto token = cancellable.GetCancellationToken();

    wait_for_cancel->get_future().get();
    token.Cancel();
    pause_for_cancel->set_value();

    auto future = cancellable.GetFuture();
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
        wait_for_cancel, pause_for_cancel, {olp::http::HttpStatusCode::OK, {}});

    MockConfigRequest(kLayer);
    auto api = MockApiRequest("blob");

    EXPECT_CALL(*network_,
                Send(IsPutRequestPrefix(api.GetBaseUrl()), _, _, _, _))
        .WillOnce(testing::Invoke(std::move(send_mock)));
    EXPECT_CALL(*network_, Cancel(_))
        .WillOnce(testing::Invoke(std::move(cancel_mock)));

    EXPECT_CALL(*cache_, Get(_, _)).Times(2);
    EXPECT_CALL(*cache_, Contains(_)).Times(1);
    EXPECT_CALL(*cache_, Put(_, _, _, _))
        .WillRepeatedly([](const std::string& /*key*/,
                           const boost::any& /*value*/,
                           const olp::cache::Encoder& /*encoder*/,
                           time_t /*expiry*/) { return true; });

    auto request =
        model::PublishPartitionDataRequest()
            .WithData(std::make_shared<std::vector<unsigned char>>(20, 0x30))
            .WithLayerId(kLayer)
            .WithPartitionId(partition);

    auto client =
        std::make_shared<write::VersionedLayerClientImpl>(kHrn, settings_);
    auto future = client->PublishToBatch(publication, request).GetFuture();

    wait_for_cancel->get_future().get();
    client.reset();
    pause_for_cancel->set_value();

    const auto response = future.get();

    ASSERT_FALSE(response.IsSuccessful());
    ASSERT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);
    Mock::VerifyAndClearExpectations(network_.get());
    Mock::VerifyAndClearExpectations(cache_.get());
  }
}

}  // namespace
