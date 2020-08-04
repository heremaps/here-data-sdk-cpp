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
#include <mocks/CacheMock.h>
#include <mocks/NetworkMock.h>
#include <olp/core/cache/CacheSettings.h>
#include <olp/core/client/ApiLookupClient.h>
#include <olp/core/client/OlpClientSettingsFactory.h>

namespace {
namespace client = olp::client;
using testing::_;
using testing::Return;

constexpr auto kConfigBaseUrl =
    "https://config.data.api.platform.in.here.com/config/v1";

constexpr auto kResponseLookupResource =
    R"jsonString([{"api":"random_service","version":"v8","baseURL":"https://config.data.api.platform.in.here.com/config/v1","parameters":{}},{"api":"pipelines","version":"v1","baseURL":"https://pipelines.api.platform.in.here.com/pipeline-service","parameters":{}},{"api":"pipelines","version":"v2","baseURL":"https://pipelines.api.platform.in.here.com/pipeline-service","parameters":{}}])jsonString";

constexpr auto kResponseLookupPlatform =
    R"jsonString([{"api":"config","version":"v1","baseURL":"https://config.data.api.platform.in.here.com/config/v1","parameters":{}},{"api":"pipelines","version":"v1","baseURL":"https://pipelines.api.platform.in.here.com/pipeline-service","parameters":{}},{"api":"pipelines","version":"v2","baseURL":"https://pipelines.api.platform.in.here.com/pipeline-service","parameters":{}}])jsonString";

class ApiLookupClientTest : public ::testing::Test {
  void SetUp() override {
    network_ = std::make_shared<testing::StrictMock<NetworkMock>>();

    settings_.cache = client::OlpClientSettingsFactory::CreateDefaultCache({});
    settings_.network_request_handler = network_;
    settings_.task_scheduler =
        client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);
    settings_.retry_settings.timeout = 1;
  }

 protected:
  client::OlpClientSettings settings_;
  std::shared_ptr<testing::StrictMock<NetworkMock>> network_;
};

TEST_F(ApiLookupClientTest, LookupApi) {
  const std::string catalog =
      "hrn:here:data::olp-here-test:hereos-internal-test-v2";
  const auto catalog_hrn = client::HRN::FromString(catalog);
  const std::string service_name = "random_service";
  const std::string service_version = "v8";
  const std::string config_url =
      "https://config.data.api.platform.in.here.com/config/v1";
  const std::string cache_key =
      catalog + "::" + service_name + "::" + service_version + "::api";
  const std::string lookup_url =
      "https://api-lookup.data.api.platform.here.com/lookup/v1/resources/" +
      catalog + "/apis";
  const std::string lookup_url_platform =
      "https://api-lookup.data.api.platform.here.com/lookup/v1/platform/apis";

  {
    SCOPED_TRACE("Fetch from cache [CacheOnly] negative");

    client::CancellationContext context;
    client::ApiLookupClient client(catalog_hrn, settings_);
    auto response = client.LookupApi(service_name, service_version,
                                     client::FetchOptions::CacheOnly, context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::NotFound);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Fetch from network");
    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupResource));

    client::CancellationContext context;
    client::ApiLookupClient client(catalog_hrn, settings_);
    auto response =
        client.LookupApi(service_name, service_version,
                         client::FetchOptions::OnlineIfNotFound, context);

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), kConfigBaseUrl);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Fetch from cache [CacheOnly] positive");

    client::CancellationContext context;
    client::ApiLookupClient client(catalog_hrn, settings_);
    auto response = client.LookupApi(service_name, service_version,
                                     client::FetchOptions::CacheOnly, context);

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), config_url);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Expiry from headers, resource");

    const time_t expiry = 1;
    const olp::http::Header header = {"Cache-Control",
                                      "max-age=" + std::to_string(expiry)};
    settings_.cache = client::OlpClientSettingsFactory::CreateDefaultCache({});

    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupResource, {header}));

    client::CancellationContext context;
    client::ApiLookupClient client(catalog_hrn, settings_);
    auto response =
        client.LookupApi(service_name, service_version,
                         client::FetchOptions::OnlineIfNotFound, context);

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), kConfigBaseUrl);

    // check cache is expired
    std::this_thread::sleep_for(std::chrono::seconds(expiry + 1));

    response = client.LookupApi(service_name, service_version,
                                client::FetchOptions::CacheOnly, context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::NotFound);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Expiry from headers, platform");

    const time_t expiry = 1;
    const olp::http::Header header = {"Cache-Control",
                                      "max-age=" + std::to_string(expiry)};

    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url_platform), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupPlatform, {header}));

    client::CancellationContext context;
    client::ApiLookupClient client(catalog_hrn, settings_);
    auto response = client.LookupApi(
        "config", "v1", client::FetchOptions::OnlineIfNotFound, context);

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), kConfigBaseUrl);

    // check cache is expired
    std::this_thread::sleep_for(std::chrono::seconds(expiry + 1));

    response = client.LookupApi(service_name, service_version,
                                client::FetchOptions::CacheOnly, context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::NotFound);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Unknown service name");

    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupResource));

    client::CancellationContext context;
    client::ApiLookupClient client(catalog_hrn, settings_);
    auto response = client.LookupApi("unknown_service", service_version,
                                     client::FetchOptions::OnlineOnly, context);

    const auto& error = response.GetError();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(error.GetErrorCode(), client::ErrorCode::ServiceUnavailable);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Unknown service version");

    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupResource));

    client::CancellationContext context;
    client::ApiLookupClient client(catalog_hrn, settings_);
    auto response = client.LookupApi(service_name, "123",
                                     client::FetchOptions::OnlineOnly, context);

    const auto& error = response.GetError();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(error.GetErrorCode(), client::ErrorCode::ServiceUnavailable);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Network error propagated to the user");
    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::UNAUTHORIZED),
                               "Inappropriate"));

    client::CancellationContext context;
    client::ApiLookupClient client(catalog_hrn, settings_);
    auto response = client.LookupApi(service_name, service_version,
                                     client::FetchOptions::OnlineOnly, context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              client::ErrorCode::AccessDenied);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Network request cancelled by network internally");
    client::CancellationContext context;
    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce([=](olp::http::NetworkRequest /*request*/,
                      olp::http::Network::Payload /*payload*/,
                      olp::http::Network::Callback /*callback*/,
                      olp::http::Network::HeaderCallback /*header_callback*/,
                      olp::http::Network::DataCallback /*data_callback*/)
                      -> olp::http::SendOutcome {
          return olp::http::SendOutcome(olp::http::ErrorCode::CANCELLED_ERROR);
        });

    client::ApiLookupClient client(catalog_hrn, settings_);
    auto response = client.LookupApi(service_name, service_version,
                                     client::FetchOptions::OnlineOnly, context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Network request timed out");
    client::CancellationContext context;
    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce([=](olp::http::NetworkRequest /*request*/,
                      olp::http::Network::Payload /*payload*/,
                      olp::http::Network::Callback /*callback*/,
                      olp::http::Network::HeaderCallback /*header_callback*/,
                      olp::http::Network::DataCallback /*data_callback*/)
                      -> olp::http::SendOutcome {
          // note no network response thread spawns
          constexpr auto unused_request_id = 12;
          return olp::http::SendOutcome(unused_request_id);
        });
    EXPECT_CALL(*network_, Cancel(_)).Times(1).WillOnce(Return());

    client::ApiLookupClient client(catalog_hrn, settings_);
    auto response = client.LookupApi(service_name, service_version,
                                     client::FetchOptions::OnlineOnly, context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              client::ErrorCode::RequestTimeout);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Network request cancelled by user");
    client::CancellationContext context;
    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce([=, &context](
                      olp::http::NetworkRequest /*request*/,
                      olp::http::Network::Payload /*payload*/,
                      olp::http::Network::Callback /*callback*/,
                      olp::http::Network::HeaderCallback /*header_callback*/,
                      olp::http::Network::DataCallback /*data_callback*/)
                      -> olp::http::SendOutcome {
          // spawn a 'user' response of cancelling
          std::thread([&context]() { context.CancelOperation(); }).detach();

          // note no network response thread spawns

          constexpr auto unused_request_id = 12;
          return olp::http::SendOutcome(unused_request_id);
        });
    EXPECT_CALL(*network_, Cancel(_)).Times(1).WillOnce(Return());

    client::ApiLookupClient client(catalog_hrn, settings_);
    auto response = client.LookupApi(service_name, service_version,
                                     client::FetchOptions::OnlineOnly, context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Network request cancelled before execution setup");
    client::CancellationContext context;

    context.CancelOperation();
    client::ApiLookupClient client(catalog_hrn, settings_);
    auto response = client.LookupApi(service_name, service_version,
                                     client::FetchOptions::OnlineOnly, context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }
}

TEST_F(ApiLookupClientTest, LookupApiAsync) {
  const std::string catalog =
      "hrn:here:data::olp-here-test:hereos-internal-test-v2";
  const auto catalog_hrn = client::HRN::FromString(catalog);
  const std::string service_name = "random_service";
  const std::string service_version = "v8";
  const std::string config_url =
      "https://config.data.api.platform.in.here.com/config/v1";
  const std::string cache_key =
      catalog + "::" + service_name + "::" + service_version + "::api";
  const std::string lookup_url =
      "https://api-lookup.data.api.platform.here.com/lookup/v1/resources/" +
      catalog + "/apis";
  const std::string lookup_url_platform =
      "https://api-lookup.data.api.platform.here.com/lookup/v1/platform/apis";

  {
    SCOPED_TRACE("Fetch from cache [CacheOnly] negative");

    std::promise<client::ApiLookupClient::LookupApiResponse> promise;
    auto future = promise.get_future();
    client::ApiLookupClient client(catalog_hrn, settings_);
    client.LookupApi(
        service_name, service_version, client::FetchOptions::CacheOnly,
        [&promise](client::ApiLookupClient::LookupApiResponse response) {
          promise.set_value(std::move(response));
        });

    auto response = future.get();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::NotFound);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Fetch from network");

    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupResource));

    std::promise<client::ApiLookupClient::LookupApiResponse> promise;
    auto future = promise.get_future();
    client::ApiLookupClient client(catalog_hrn, settings_);
    client.LookupApi(
        service_name, service_version, client::FetchOptions::OnlineIfNotFound,
        [&promise](client::ApiLookupClient::LookupApiResponse response) {
          promise.set_value(std::move(response));
        });

    auto response = future.get();

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), kConfigBaseUrl);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Fetch from cache [CacheOnly] positive");

    std::promise<client::ApiLookupClient::LookupApiResponse> promise;
    auto future = promise.get_future();
    client::ApiLookupClient client(catalog_hrn, settings_);
    client.LookupApi(
        service_name, service_version, client::FetchOptions::CacheOnly,
        [&promise](client::ApiLookupClient::LookupApiResponse response) {
          promise.set_value(std::move(response));
        });

    auto response = future.get();

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), config_url);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Expiry from headers, resource");

    const time_t expiry = 1;
    const olp::http::Header header = {"Cache-Control",
                                      "max-age=" + std::to_string(expiry)};
    settings_.cache = client::OlpClientSettingsFactory::CreateDefaultCache({});

    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupResource, {header}));

    client::CancellationContext context;
    client::ApiLookupClient client(catalog_hrn, settings_);
    auto response =
        client.LookupApi(service_name, service_version,
                         client::FetchOptions::OnlineIfNotFound, context);

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), kConfigBaseUrl);

    // check cache is expired
    std::this_thread::sleep_for(std::chrono::seconds(expiry + 1));

    response = client.LookupApi(service_name, service_version,
                                client::FetchOptions::CacheOnly, context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::NotFound);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Expiry from headers, platform");

    const time_t expiry = 1;
    const olp::http::Header header = {"Cache-Control",
                                      "max-age=" + std::to_string(expiry)};

    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url_platform), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupPlatform, {header}));

    client::CancellationContext context;
    client::ApiLookupClient client(catalog_hrn, settings_);
    auto response = client.LookupApi(
        "config", "v1", client::FetchOptions::OnlineIfNotFound, context);

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), kConfigBaseUrl);

    // check cache is expired
    std::this_thread::sleep_for(std::chrono::seconds(expiry + 1));

    response = client.LookupApi(service_name, service_version,
                                client::FetchOptions::CacheOnly, context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::NotFound);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Unknown service name");

    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupResource));

    client::CancellationContext context;
    client::ApiLookupClient client(catalog_hrn, settings_);
    auto response = client.LookupApi("unknown_service", service_version,
                                     client::FetchOptions::OnlineOnly, context);

    const auto& error = response.GetError();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(error.GetErrorCode(), client::ErrorCode::ServiceUnavailable);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Unknown service version");

    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupResource));

    client::CancellationContext context;
    client::ApiLookupClient client(catalog_hrn, settings_);
    auto response = client.LookupApi(service_name, "123",
                                     client::FetchOptions::OnlineOnly, context);

    const auto& error = response.GetError();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(error.GetErrorCode(), client::ErrorCode::ServiceUnavailable);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Network error propagated to the user");

    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::UNAUTHORIZED),
                               "Inappropriate"));

    std::promise<client::ApiLookupClient::LookupApiResponse> promise;
    auto future = promise.get_future();
    client::ApiLookupClient client(catalog_hrn, settings_);
    client.LookupApi(
        service_name, service_version, client::FetchOptions::OnlineOnly,
        [&promise](client::ApiLookupClient::LookupApiResponse response) {
          promise.set_value(std::move(response));
        });

    auto response = future.get();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              client::ErrorCode::AccessDenied);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Network request cancelled by network internally");

    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce([](olp::http::NetworkRequest /*request*/,
                     olp::http::Network::Payload /*payload*/,
                     olp::http::Network::Callback /*callback*/,
                     olp::http::Network::HeaderCallback /*header_callback*/,
                     olp::http::Network::DataCallback /*data_callback*/)
                      -> olp::http::SendOutcome {
          return olp::http::SendOutcome(olp::http::ErrorCode::CANCELLED_ERROR);
        });

    std::promise<client::ApiLookupClient::LookupApiResponse> promise;
    auto future = promise.get_future();
    client::ApiLookupClient client(catalog_hrn, settings_);
    client.LookupApi(
        service_name, service_version, client::FetchOptions::OnlineOnly,
        [&promise](client::ApiLookupClient::LookupApiResponse response) {
          promise.set_value(std::move(response));
        });

    auto response = future.get();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }
}

TEST_F(ApiLookupClientTest, CustomCatalogProvider) {
  const std::string catalog =
      "hrn:here:data::olp-here-test:hereos-internal-test-v2";
  const auto catalog_hrn = client::HRN::FromString(catalog);
  const std::string service_name = "random_service";
  const std::string service_url = "http://random_service.com";
  const std::string service_version = "v8";
  const std::string provider_url = "https://some-lookup-url.com/lookup/v1";
  const std::string static_base_url = provider_url + "/catalogs/" + catalog;
  const std::string lookup_url =
      "https://api-lookup.data.api.platform.here.com/lookup/v1/resources/" +
      catalog + "/apis";
  {
    SCOPED_TRACE("Static url catalog");

    EXPECT_CALL(*network_, Send(_, _, _, _, _)).Times(0);

    client::ApiLookupSettings lookup_settings;
    lookup_settings.catalog_endpoint_provider =
        [&provider_url](const client::HRN&) { return provider_url; };
    settings_.api_lookup_settings = lookup_settings;

    client::CancellationContext context;
    client::ApiLookupClient client(catalog_hrn, settings_);
    auto response = client.LookupApi(service_name, service_version,
                                     client::OnlineOnly, context);

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), static_base_url);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Non-static url catalog");

    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupResource));

    client::ApiLookupSettings lookup_settings;
    lookup_settings.catalog_endpoint_provider = [](const client::HRN&) {
      return "";
    };
    settings_.api_lookup_settings = lookup_settings;

    client::CancellationContext context;
    client::ApiLookupClient client(catalog_hrn, settings_);
    auto response = client.LookupApi(service_name, service_version,
                                     client::OnlineOnly, context);

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), kConfigBaseUrl);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }
}

TEST_F(ApiLookupClientTest, CustomCatalogProviderAsync) {
  const std::string catalog =
      "hrn:here:data::olp-here-test:hereos-internal-test-v2";
  const auto catalog_hrn = client::HRN::FromString(catalog);
  const std::string service_name = "random_service";
  const std::string service_url = "http://random_service.com";
  const std::string service_version = "v8";
  const std::string provider_url = "https://some-lookup-url.com/lookup/v1";
  const std::string static_base_url = provider_url + "/catalogs/" + catalog;
  const std::string lookup_url =
      "https://api-lookup.data.api.platform.here.com/lookup/v1/resources/" +
      catalog + "/apis";
  {
    SCOPED_TRACE("Static url catalog");

    EXPECT_CALL(*network_, Send(_, _, _, _, _)).Times(0);

    client::ApiLookupSettings lookup_settings;
    lookup_settings.catalog_endpoint_provider =
        [&provider_url](const client::HRN&) { return provider_url; };
    settings_.api_lookup_settings = lookup_settings;

    std::promise<client::ApiLookupClient::LookupApiResponse> promise;
    auto future = promise.get_future();
    client::ApiLookupClient client(catalog_hrn, settings_);
    client.LookupApi(
        service_name, service_version, client::OnlineOnly,
        [&promise](client::ApiLookupClient::LookupApiResponse response) {
          promise.set_value(std::move(response));
        });

    auto response = future.get();

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), static_base_url);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }

  {
    SCOPED_TRACE("Non-static url catalog");

    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupResource));

    client::ApiLookupSettings lookup_settings;
    lookup_settings.catalog_endpoint_provider = [](const client::HRN&) {
      return "";
    };
    settings_.api_lookup_settings = lookup_settings;

    std::promise<client::ApiLookupClient::LookupApiResponse> promise;
    auto future = promise.get_future();
    client::ApiLookupClient client(catalog_hrn, settings_);
    client.LookupApi(
        service_name, service_version, client::OnlineOnly,
        [&promise](client::ApiLookupClient::LookupApiResponse response) {
          promise.set_value(std::move(response));
        });

    auto response = future.get();

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), kConfigBaseUrl);
    testing::Mock::VerifyAndClearExpectations(network_.get());
  }
}

}  // namespace
