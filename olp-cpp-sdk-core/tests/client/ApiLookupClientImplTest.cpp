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
#include <olp/core/client/OlpClientSettingsFactory.h>
#include "client/ApiLookupClientImpl.h"

namespace {
namespace client = olp::client;
using testing::_;
using testing::Return;

constexpr auto kConfigBaseUrl =
    "https://config.data.api.platform.sit.here.com/config/v1";

constexpr auto kResponseLookupResource =
    R"jsonString([{"api":"random_service","version":"v8","baseURL":"https://config.data.api.platform.sit.here.com/config/v1","parameters":{}},{"api":"pipelines","version":"v1","baseURL":"https://pipelines.api.platform.sit.here.com/pipeline-service","parameters":{}},{"api":"pipelines","version":"v2","baseURL":"https://pipelines.api.platform.sit.here.com/pipeline-service","parameters":{}}])jsonString";

constexpr auto kResponseLookupPlatform =
    R"jsonString([{"api":"config","version":"v1","baseURL":"https://config.data.api.platform.sit.here.com/config/v1","parameters":{}},{"api":"pipelines","version":"v1","baseURL":"https://pipelines.api.platform.sit.here.com/pipeline-service","parameters":{}},{"api":"pipelines","version":"v2","baseURL":"https://pipelines.api.platform.sit.here.com/pipeline-service","parameters":{}}])jsonString";

class ApiLookupClientImplTestable : public client::ApiLookupClientImpl {
 public:
  ApiLookupClientImplTestable(const client::HRN& catalog,
                              const client::OlpClientSettings& settings)
      : client::ApiLookupClientImpl(catalog, settings) {}

  using client::ApiLookupClientImpl::CreateAndCacheClient;
  using client::ApiLookupClientImpl::GetCachedClient;
};

class ApiLookupClientImplTest : public ::testing::Test {
  void SetUp() override {
    cache_ = std::make_shared<testing::StrictMock<CacheMock>>();
    network_ = std::make_shared<testing::StrictMock<NetworkMock>>();

    settings_.cache = cache_;
    settings_.network_request_handler = network_;
    settings_.task_scheduler =
        client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1);
    settings_.retry_settings.timeout = 1;
  }

 protected:
  client::OlpClientSettings settings_;
  std::shared_ptr<testing::StrictMock<CacheMock>> cache_;
  std::shared_ptr<testing::StrictMock<NetworkMock>> network_;
};

TEST_F(ApiLookupClientImplTest, LookupApi) {
  const std::string catalog =
      "hrn:here:data::olp-here-test:hereos-internal-test-v2";
  const auto catalog_hrn = client::HRN::FromString(catalog);
  const std::string service_name = "random_service";
  const std::string service_url = "http://random_service.com";
  const std::string service_version = "v8";
  const std::string config_url =
      "https://config.data.api.platform.sit.here.com/config/v1";
  const std::string cache_key =
      catalog + "::" + service_name + "::" + service_version + "::api";
  const std::string lookup_url =
      "https://api-lookup.data.api.platform.here.com/lookup/v1/resources/" +
      catalog + "/apis";
  const std::string lookup_url_platform =
      "https://api-lookup.data.api.platform.here.com/lookup/v1/platform/apis";

  {
    SCOPED_TRACE("Fetch from cache [CacheOnly] positive");
    EXPECT_CALL(*cache_, Get(cache_key, _))
        .Times(1)
        .WillOnce(Return(service_url));

    client::CancellationContext context;
    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    auto response = client.LookupApi(service_name, service_version,
                                     client::FetchOptions::CacheOnly, context);

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), service_url);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Fetch from cache [CacheOnly] negative");
    EXPECT_CALL(*cache_, Get(cache_key, _))
        .Times(1)
        .WillOnce(Return(boost::any()));

    client::CancellationContext context;
    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    auto response = client.LookupApi(service_name, service_version,
                                     client::FetchOptions::CacheOnly, context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::NotFound);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Fetch from network");
    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupResource));
    EXPECT_CALL(*cache_, Put(_, _, _, _)).Times(0);

    client::CancellationContext context;
    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    auto response = client.LookupApi(service_name, service_version,
                                     client::FetchOptions::OnlineOnly, context);

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), kConfigBaseUrl);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Expiry from headers, resource");

    const time_t expiry = 13;
    const olp::http::Header header = {"Cache-Control",
                                      "max-age=" + std::to_string(expiry)};

    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupResource, {header}));
    EXPECT_CALL(*cache_, Put(_, _, _, expiry))
        .Times(3)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*cache_, Get(cache_key, _))
        .Times(1)
        .WillOnce(Return(boost::any()));

    client::CancellationContext context;
    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    auto response =
        client.LookupApi(service_name, service_version,
                         client::FetchOptions::OnlineIfNotFound, context);

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), kConfigBaseUrl);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Expiry from headers, platform");

    const time_t expiry = 13;
    const olp::http::Header header = {"Cache-Control",
                                      "max-age=" + std::to_string(expiry)};

    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url_platform), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupPlatform, {header}));
    EXPECT_CALL(*cache_, Put(_, _, _, expiry))
        .Times(3)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*cache_, Get(_, _)).Times(1).WillOnce(Return(boost::any()));

    client::CancellationContext context;
    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    auto response = client.LookupApi(
        "config", "v1", client::FetchOptions::OnlineIfNotFound, context);

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), kConfigBaseUrl);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Unknown service name");

    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupResource));

    client::CancellationContext context;
    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    auto response = client.LookupApi("unknown_service", service_version,
                                     client::FetchOptions::OnlineOnly, context);

    const auto& error = response.GetError();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(error.GetErrorCode(), client::ErrorCode::ServiceUnavailable);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Unknown service version");

    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupResource));

    client::CancellationContext context;
    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    auto response = client.LookupApi(service_name, "123",
                                     client::FetchOptions::OnlineOnly, context);

    const auto& error = response.GetError();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(error.GetErrorCode(), client::ErrorCode::ServiceUnavailable);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
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
    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    auto response = client.LookupApi(service_name, service_version,
                                     client::FetchOptions::OnlineOnly, context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              client::ErrorCode::AccessDenied);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
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

    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    auto response = client.LookupApi(service_name, service_version,
                                     client::FetchOptions::OnlineOnly, context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
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

    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    auto response = client.LookupApi(service_name, service_version,
                                     client::FetchOptions::OnlineOnly, context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              client::ErrorCode::RequestTimeout);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
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

    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    auto response = client.LookupApi(service_name, service_version,
                                     client::FetchOptions::OnlineOnly, context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Network request cancelled before execution setup");
    client::CancellationContext context;

    context.CancelOperation();
    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    auto response = client.LookupApi(service_name, service_version,
                                     client::FetchOptions::OnlineOnly, context);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Client caching from online");
    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupResource));

    EXPECT_CALL(*cache_, Get(_, _)).Times(1).WillOnce(Return(boost::any()));

    // Response contains three services that are cached independently.
    EXPECT_CALL(*cache_, Put(_, _, _, _)).Times(3);

    ApiLookupClientImplTestable client(catalog_hrn, settings_);

    // Loop few times to make sure everything is cached and do to produce cache
    // or online lookups.
    for (auto i = 0; i < 3; i++) {
      auto response = client.LookupApi(service_name, service_version,
                                       client::FetchOptions::OnlineIfNotFound,
                                       client::CancellationContext());

      ASSERT_TRUE(response.IsSuccessful());
      EXPECT_EQ(response.GetResult().GetBaseUrl(), kConfigBaseUrl);
    }

    auto cached_client = client.GetCachedClient(service_name, service_version);
    ASSERT_TRUE(cached_client);
    EXPECT_EQ(cached_client->GetBaseUrl(), kConfigBaseUrl);

    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Client caching from cache");
    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _)).Times(0);

    EXPECT_CALL(*cache_, Get(cache_key, _))
        .Times(1)
        .WillOnce(Return(service_url));

    ApiLookupClientImplTestable client(catalog_hrn, settings_);

    // Loop few times to make sure everything is cached and do to produce cache
    // or online lookups.
    for (auto i = 0; i < 3; i++) {
      auto response = client.LookupApi(service_name, service_version,
                                       client::FetchOptions::OnlineIfNotFound,
                                       client::CancellationContext());

      ASSERT_TRUE(response.IsSuccessful());
      EXPECT_EQ(response.GetResult().GetBaseUrl(), service_url);
    }

    auto cached_client = client.GetCachedClient(service_name, service_version);
    ASSERT_TRUE(cached_client);
    EXPECT_EQ(cached_client->GetBaseUrl(), service_url);

    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
  }
}

TEST_F(ApiLookupClientImplTest, CustomProvider) {
  const std::string catalog =
      "hrn:here:data::olp-here-test:hereos-internal-test-v2";
  const auto catalog_hrn = client::HRN::FromString(catalog);
  const std::string service_name = "random_service";
  const std::string service_url = "http://random_service.com";
  const std::string service_version = "v8";
  const std::string config_url =
      "https://config.data.api.platform.sit.here.com/config/v1";
  const std::string lookup_url = "https://some-lookup-url.com/lookup/v1";
  const std::string request_lookup_url =
      lookup_url + "/resources/" + catalog + "/apis";

  client::ApiLookupSettings lookup_settings;
  lookup_settings.lookup_endpoint_provider = [&lookup_url](const std::string&) {
    return lookup_url;
  };

  settings_.api_lookup_settings = lookup_settings;

  EXPECT_CALL(*network_, Send(IsGetRequest(request_lookup_url), _, _, _, _))
      .Times(1)
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kResponseLookupResource));
  EXPECT_CALL(*cache_, Put(_, _, _, _)).Times(0);

  client::CancellationContext context;
  client::ApiLookupClientImpl client(catalog_hrn, settings_);
  auto response = client.LookupApi(service_name, service_version,
                                   client::OnlineOnly, context);

  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_EQ(response.GetResult().GetBaseUrl(), kConfigBaseUrl);
  testing::Mock::VerifyAndClearExpectations(network_.get());
  testing::Mock::VerifyAndClearExpectations(cache_.get());
}

TEST_F(ApiLookupClientImplTest, CustomCatalogProvider) {
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
    EXPECT_CALL(*cache_, Put(_, _, _, _)).Times(0);

    client::ApiLookupSettings lookup_settings;
    lookup_settings.catalog_endpoint_provider =
        [&provider_url](const client::HRN&) { return provider_url; };
    settings_.api_lookup_settings = lookup_settings;

    client::CancellationContext context;
    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    auto response = client.LookupApi(service_name, service_version,
                                     client::OnlineOnly, context);

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), static_base_url);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Non-static url catalog");

    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupResource));
    EXPECT_CALL(*cache_, Put(_, _, _, _)).Times(0);

    client::ApiLookupSettings lookup_settings;
    lookup_settings.catalog_endpoint_provider = [](const client::HRN&) {
      return "";
    };
    settings_.api_lookup_settings = lookup_settings;

    client::CancellationContext context;
    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    auto response = client.LookupApi(service_name, service_version,
                                     client::OnlineOnly, context);

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), kConfigBaseUrl);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
  }
}

TEST_F(ApiLookupClientImplTest, LookupApiAsync) {
  const std::string catalog =
      "hrn:here:data::olp-here-test:hereos-internal-test-v2";
  const auto catalog_hrn = client::HRN::FromString(catalog);
  const std::string service_name = "random_service";
  const std::string service_url = "http://random_service.com";
  const std::string service_version = "v8";
  const std::string config_url =
      "https://config.data.api.platform.sit.here.com/config/v1";
  const std::string cache_key =
      catalog + "::" + service_name + "::" + service_version + "::api";
  const std::string lookup_url =
      "https://api-lookup.data.api.platform.here.com/lookup/v1/resources/" +
      catalog + "/apis";
  const std::string lookup_url_platform =
      "https://api-lookup.data.api.platform.here.com/lookup/v1/platform/apis";

  {
    SCOPED_TRACE("Fetch from cache [CacheOnly] positive");
    EXPECT_CALL(*cache_, Get(cache_key, _))
        .Times(1)
        .WillOnce(Return(service_url));

    std::promise<client::ApiLookupClient::LookupApiResponse> promise;
    auto future = promise.get_future();
    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    client.LookupApi(
        service_name, service_version, client::FetchOptions::CacheOnly,
        [&promise](client::ApiLookupClient::LookupApiResponse response) {
          promise.set_value(std::move(response));
        });

    auto response = future.get();

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), service_url);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Fetch from cache [CacheOnly] negative");
    EXPECT_CALL(*cache_, Get(cache_key, _))
        .Times(1)
        .WillOnce(Return(boost::any()));

    std::promise<client::ApiLookupClient::LookupApiResponse> promise;
    auto future = promise.get_future();
    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    client.LookupApi(
        service_name, service_version, client::FetchOptions::CacheOnly,
        [&promise](client::ApiLookupClient::LookupApiResponse response) {
          promise.set_value(std::move(response));
        });

    auto response = future.get();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::NotFound);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Fetch from network");
    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupResource));
    EXPECT_CALL(*cache_, Put(cache_key, _, _, _)).Times(0);

    std::promise<client::ApiLookupClient::LookupApiResponse> promise;
    auto future = promise.get_future();
    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    client.LookupApi(
        service_name, service_version, client::FetchOptions::OnlineOnly,
        [&promise](client::ApiLookupClient::LookupApiResponse response) {
          promise.set_value(std::move(response));
        });

    auto response = future.get();

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), kConfigBaseUrl);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Expiry from headers, resource");

    const time_t expiry = 13;
    const olp::http::Header header = {"Cache-Control",
                                      "max-age=" + std::to_string(expiry)};

    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupResource, {header}));
    EXPECT_CALL(*cache_, Put(_, _, _, expiry))
        .Times(3)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*cache_, Get(cache_key, _))
        .Times(1)
        .WillOnce(Return(boost::any()));

    std::promise<client::ApiLookupClient::LookupApiResponse> promise;
    auto future = promise.get_future();
    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    client.LookupApi(
        service_name, service_version, client::FetchOptions::OnlineIfNotFound,
        [&promise](client::ApiLookupClient::LookupApiResponse response) {
          promise.set_value(std::move(response));
        });

    auto response = future.get();

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), kConfigBaseUrl);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Expiry from headers, platform");

    const time_t expiry = 13;
    const olp::http::Header header = {"Cache-Control",
                                      "max-age=" + std::to_string(expiry)};

    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url_platform), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupPlatform, {header}));
    EXPECT_CALL(*cache_, Put(_, _, _, expiry))
        .Times(3)
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*cache_, Get(_, _)).Times(1).WillOnce(Return(boost::any()));

    std::promise<client::ApiLookupClient::LookupApiResponse> promise;
    auto future = promise.get_future();
    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    client.LookupApi(
        "config", "v1", client::FetchOptions::OnlineIfNotFound,
        [&promise](client::ApiLookupClient::LookupApiResponse response) {
          promise.set_value(std::move(response));
        });

    auto response = future.get();

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), kConfigBaseUrl);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Unknown service name");

    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupResource));

    std::promise<client::ApiLookupClient::LookupApiResponse> promise;
    auto future = promise.get_future();
    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    client.LookupApi(
        "unknown_service", service_version, client::FetchOptions::OnlineOnly,
        [&promise](client::ApiLookupClient::LookupApiResponse response) {
          promise.set_value(std::move(response));
        });

    auto response = future.get();
    const auto& error = response.GetError();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(error.GetErrorCode(), client::ErrorCode::ServiceUnavailable);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Unknown service version");

    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupResource));

    std::promise<client::ApiLookupClient::LookupApiResponse> promise;
    auto future = promise.get_future();
    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    client.LookupApi(
        service_name, "123", client::FetchOptions::OnlineOnly,
        [&promise](client::ApiLookupClient::LookupApiResponse response) {
          promise.set_value(std::move(response));
        });

    auto response = future.get();
    const auto& error = response.GetError();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(error.GetErrorCode(), client::ErrorCode::ServiceUnavailable);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
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
    client::ApiLookupClientImpl client(catalog_hrn, settings_);
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
    testing::Mock::VerifyAndClearExpectations(cache_.get());
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
    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    client.LookupApi(
        service_name, service_version, client::FetchOptions::OnlineOnly,
        [&promise](client::ApiLookupClient::LookupApiResponse response) {
          promise.set_value(std::move(response));
        });

    auto response = future.get();

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Cancelled);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Client caching from online");
    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupResource));

    EXPECT_CALL(*cache_, Get(_, _)).Times(1).WillOnce(Return(boost::any()));

    // Response contains three services that are cached independently.
    EXPECT_CALL(*cache_, Put(_, _, _, _)).Times(3);

    ApiLookupClientImplTestable client(catalog_hrn, settings_);

    // Loop few times to make sure everything is cached and do not produce cache
    // or online lookups.
    for (auto i = 0; i < 3; i++) {
      std::promise<client::ApiLookupClient::LookupApiResponse> promise;
      auto future = promise.get_future();
      client.LookupApi(
          service_name, service_version, client::FetchOptions::OnlineIfNotFound,
          [&promise](client::ApiLookupClient::LookupApiResponse response) {
            promise.set_value(std::move(response));
          });

      auto response = future.get();

      ASSERT_TRUE(response.IsSuccessful());
      EXPECT_EQ(response.GetResult().GetBaseUrl(), kConfigBaseUrl);
    }

    auto cached_client = client.GetCachedClient(service_name, service_version);
    ASSERT_TRUE(cached_client);
    EXPECT_EQ(cached_client->GetBaseUrl(), kConfigBaseUrl);

    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Client caching from cache");
    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _)).Times(0);

    EXPECT_CALL(*cache_, Get(cache_key, _))
        .Times(1)
        .WillOnce(Return(service_url));

    ApiLookupClientImplTestable client(catalog_hrn, settings_);

    // Loop few times to make sure everything is cached and do not produce cache
    // or online lookups.
    for (auto i = 0; i < 3; i++) {
      std::promise<client::ApiLookupClient::LookupApiResponse> promise;
      auto future = promise.get_future();
      client.LookupApi(
          service_name, service_version, client::FetchOptions::OnlineIfNotFound,
          [&promise](client::ApiLookupClient::LookupApiResponse response) {
            promise.set_value(std::move(response));
          });

      auto response = future.get();

      ASSERT_TRUE(response.IsSuccessful());
      EXPECT_EQ(response.GetResult().GetBaseUrl(), service_url);
    }

    auto cached_client = client.GetCachedClient(service_name, service_version);
    ASSERT_TRUE(cached_client);
    EXPECT_EQ(cached_client->GetBaseUrl(), service_url);

    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
  }
}

TEST_F(ApiLookupClientImplTest, CustomProviderAsync) {
  const std::string catalog =
      "hrn:here:data::olp-here-test:hereos-internal-test-v2";
  const auto catalog_hrn = client::HRN::FromString(catalog);
  const std::string service_name = "random_service";
  const std::string service_url = "http://random_service.com";
  const std::string service_version = "v8";
  const std::string config_url =
      "https://config.data.api.platform.sit.here.com/config/v1";
  const std::string lookup_url = "https://some-lookup-url.com/lookup/v1";
  const std::string request_lookup_url =
      lookup_url + "/resources/" + catalog + "/apis";

  client::ApiLookupSettings lookup_settings;
  lookup_settings.lookup_endpoint_provider = [&lookup_url](const std::string&) {
    return lookup_url;
  };

  settings_.api_lookup_settings = lookup_settings;

  EXPECT_CALL(*network_, Send(IsGetRequest(request_lookup_url), _, _, _, _))
      .Times(1)
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kResponseLookupResource));
  EXPECT_CALL(*cache_, Put(_, _, _, _)).Times(0);

  std::promise<client::ApiLookupClient::LookupApiResponse> promise;
  auto future = promise.get_future();
  client::ApiLookupClientImpl client(catalog_hrn, settings_);
  client.LookupApi(
      service_name, service_version, client::OnlineOnly,
      [&promise](client::ApiLookupClient::LookupApiResponse response) {
        promise.set_value(std::move(response));
      });

  auto response = future.get();

  EXPECT_TRUE(response.IsSuccessful());
  EXPECT_EQ(response.GetResult().GetBaseUrl(), kConfigBaseUrl);
  testing::Mock::VerifyAndClearExpectations(network_.get());
  testing::Mock::VerifyAndClearExpectations(cache_.get());
}

TEST_F(ApiLookupClientImplTest, CustomCatalogProviderAsync) {
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
    EXPECT_CALL(*cache_, Put(_, _, _, _)).Times(0);

    client::ApiLookupSettings lookup_settings;
    lookup_settings.catalog_endpoint_provider =
        [&provider_url](const client::HRN&) { return provider_url; };
    settings_.api_lookup_settings = lookup_settings;

    std::promise<client::ApiLookupClient::LookupApiResponse> promise;
    auto future = promise.get_future();
    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    client.LookupApi(
        service_name, service_version, client::OnlineOnly,
        [&promise](client::ApiLookupClient::LookupApiResponse response) {
          promise.set_value(std::move(response));
        });

    auto response = future.get();

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), static_base_url);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
  }

  {
    SCOPED_TRACE("Non-static url catalog");

    EXPECT_CALL(*network_, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupResource));
    EXPECT_CALL(*cache_, Put(_, _, _, _)).Times(0);

    client::ApiLookupSettings lookup_settings;
    lookup_settings.catalog_endpoint_provider = [](const client::HRN&) {
      return "";
    };
    settings_.api_lookup_settings = lookup_settings;

    std::promise<client::ApiLookupClient::LookupApiResponse> promise;
    auto future = promise.get_future();
    client::ApiLookupClientImpl client(catalog_hrn, settings_);
    client.LookupApi(
        service_name, service_version, client::OnlineOnly,
        [&promise](client::ApiLookupClient::LookupApiResponse response) {
          promise.set_value(std::move(response));
        });

    auto response = future.get();

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), kConfigBaseUrl);
    testing::Mock::VerifyAndClearExpectations(network_.get());
    testing::Mock::VerifyAndClearExpectations(cache_.get());
  }
}

}  // namespace
