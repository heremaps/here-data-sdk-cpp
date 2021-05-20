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
#include "../src/ApiClientLookup.h"
namespace {
namespace client = olp::client;
namespace read = olp::dataservice::read;

const auto kConfigBaseUrl =
    "https://config.data.api.platform.sit.here.com/config/v1";

const auto kResponseLookupConfig =
    R"jsonString([{"api":"random_service","version":"v8","baseURL":"https://config.data.api.platform.sit.here.com/config/v1","parameters":{}},{"api":"pipelines","version":"v1","baseURL":"https://pipelines.api.platform.sit.here.com/pipeline-service","parameters":{}},{"api":"pipelines","version":"v2","baseURL":"https://pipelines.api.platform.sit.here.com/pipeline-service","parameters":{}}])jsonString";

TEST(ApiClientLookupTest, LookupApi) {
  using testing::_;
  using testing::Return;

  auto cache = std::make_shared<testing::StrictMock<CacheMock>>();
  auto network = std::make_shared<testing::StrictMock<NetworkMock>>();

  client::OlpClientSettings settings;
  settings.cache = cache;
  settings.network_request_handler = network;
  settings.retry_settings.timeout = 1;

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

  {
    SCOPED_TRACE("Fetch from cache [CacheOnly] positive");
    EXPECT_CALL(*cache, Get(cache_key, _))
        .Times(1)
        .WillOnce(Return(service_url));

    client::CancellationContext context;
    auto response = read::ApiClientLookup::LookupApi(
        catalog_hrn, context, service_name, service_version,
        read::FetchOptions::CacheOnly, settings);

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), service_url);
    testing::Mock::VerifyAndClearExpectations(cache.get());
  }
  {
    SCOPED_TRACE("Fetch from cache [CacheOnly] negative");
    EXPECT_CALL(*cache, Get(cache_key, _))
        .Times(1)
        .WillOnce(Return(boost::any()));

    client::CancellationContext context;
    auto response = read::ApiClientLookup::LookupApi(
        catalog_hrn, context, service_name, service_version,
        read::FetchOptions::CacheOnly, settings);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::NotFound);
    testing::Mock::VerifyAndClearExpectations(cache.get());
  }
  {
    SCOPED_TRACE("Fetch from network");
    EXPECT_CALL(*network, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     kResponseLookupConfig));
    EXPECT_CALL(*cache, Put(cache_key, _, _, _))
        .Times(0)
        .WillOnce(Return(true));

    client::CancellationContext context;
    auto response = read::ApiClientLookup::LookupApi(
        catalog_hrn, context, service_name, service_version,
        read::FetchOptions::OnlineOnly, settings);

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), kConfigBaseUrl);
    testing::Mock::VerifyAndClearExpectations(network.get());
  }
  {
    SCOPED_TRACE("Network error propagated to the user");
    EXPECT_CALL(*network, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                   olp::http::HttpStatusCode::UNAUTHORIZED),
                               "Inappropriate"));

    client::CancellationContext context;
    auto response = read::ApiClientLookup::LookupApi(
        catalog_hrn, context, service_name, service_version,
        read::FetchOptions::OnlineOnly, settings);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::AccessDenied);
    testing::Mock::VerifyAndClearExpectations(network.get());
  }
  {
    SCOPED_TRACE("Network request cancelled by network internally");
    client::CancellationContext context;
    EXPECT_CALL(*network, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce([=](olp::http::NetworkRequest /*request*/,
                      olp::http::Network::Payload /*payload*/,
                      olp::http::Network::Callback /*callback*/,
                      olp::http::Network::HeaderCallback /*header_callback*/,
                      olp::http::Network::DataCallback /*data_callback*/)
                      -> olp::http::SendOutcome {
          return olp::http::SendOutcome(olp::http::ErrorCode::CANCELLED_ERROR);
        });

    auto response = read::ApiClientLookup::LookupApi(
        catalog_hrn, context, service_name, service_version,
        read::FetchOptions::OnlineOnly, settings);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::Cancelled);
    testing::Mock::VerifyAndClearExpectations(network.get());
  }
  {
    SCOPED_TRACE("Network request timed out");
    client::CancellationContext context;
    EXPECT_CALL(*network, Send(IsGetRequest(lookup_url), _, _, _, _))
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
    EXPECT_CALL(*network, Cancel(_)).Times(1).WillOnce(Return());

    auto response = read::ApiClientLookup::LookupApi(
        catalog_hrn, context, service_name, service_version,
        read::FetchOptions::OnlineOnly, settings);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::RequestTimeout);
    testing::Mock::VerifyAndClearExpectations(network.get());
  }
  {
    SCOPED_TRACE("Network request cancelled by user");
    client::CancellationContext context;
    EXPECT_CALL(*network, Send(IsGetRequest(lookup_url), _, _, _, _))
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
    EXPECT_CALL(*network, Cancel(_)).Times(1).WillOnce(Return());

    auto response = read::ApiClientLookup::LookupApi(
        catalog_hrn, context, service_name, service_version,
        read::FetchOptions::OnlineOnly, settings);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::Cancelled);
    testing::Mock::VerifyAndClearExpectations(network.get());
  }
  {
    SCOPED_TRACE("Network request cancelled before execution setup");
    client::CancellationContext context;

    context.CancelOperation();
    auto response = read::ApiClientLookup::LookupApi(
        catalog_hrn, context, service_name, service_version,
        read::FetchOptions::OnlineOnly, settings);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::Cancelled);
    testing::Mock::VerifyAndClearExpectations(network.get());
  }
}

TEST(ApiClientLookupTest, LookupApiConcurrent) {
  using testing::_;
  using testing::Return;

  auto cache = std::make_shared<testing::StrictMock<CacheMock>>();
  auto network = std::make_shared<testing::StrictMock<NetworkMock>>();

  client::OlpClientSettings settings;
  settings.cache = cache;
  settings.network_request_handler = network;
  settings.retry_settings.timeout = 1;

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

  testing::InSequence s;

  EXPECT_CALL(*cache, Get(cache_key, _)).WillOnce(Return(boost::any()));
  EXPECT_CALL(*network, Send(IsGetRequest(lookup_url), _, _, _, _))
      .Times(1)
      .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                       olp::http::HttpStatusCode::OK),
                                   kResponseLookupConfig));
  EXPECT_CALL(*cache, Put(cache_key, _, _, _)).WillOnce(Return(true));
  // Also expect: pipelines v1, pipelines v2
  EXPECT_CALL(*cache, Put(_, _, _, _)).Times(2).WillRepeatedly(Return(true));
  EXPECT_CALL(*cache, Get(cache_key, _))
      .Times(4)
      .WillRepeatedly(Return(service_url));

  const auto threads_count = 5;
  std::vector<std::thread> threads;

  read::repository::NamedMutexStorage named_mutexes;

  for (auto i = 0; i < threads_count; i++) {
    threads.emplace_back([=]() {
      client::CancellationContext context;
      auto response = read::ApiClientLookup::LookupApi(
          catalog_hrn, context, service_name, service_version,
          read::FetchOptions::OnlineIfNotFound, settings, named_mutexes);
    });
  }

  for (auto i = 0; i < threads_count; i++) {
    threads[i].join();
  }

  testing::Mock::VerifyAndClearExpectations(network.get());
  testing::Mock::VerifyAndClearExpectations(cache.get());
}

}  // namespace
