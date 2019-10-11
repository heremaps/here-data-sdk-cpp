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
#include "../src/ApiClientLookup.h"

using namespace olp;
using namespace client;
using namespace dataservice::read;
using namespace olp::tests::common;

#define OLP_SDK_CONFIG_BASE_URL \
  "https://config.data.api.platform.in.here.com/config/v1"

#define OLP_SDK_HTTP_RESPONSE_LOOKUP_CONFIG                                                    \
  R"jsonString([{"api":"config","version":"v1","baseURL":")jsonString" OLP_SDK_CONFIG_BASE_URL \
  R"jsonString(","parameters":{}},{"api":"pipelines","version":"v1","baseURL":"https://pipelines.api.platform.in.here.com/pipeline-service","parameters":{}},{"api":"pipelines","version":"v2","baseURL":"https://pipelines.api.platform.in.here.com/pipeline-service","parameters":{}}])jsonString"

TEST(ApiClientLookupTest, LookupApi) {
  using namespace testing;
  using testing::Return;

  auto cache = std::make_shared<testing::StrictMock<CacheMock>>();
  auto network = std::make_shared<testing::StrictMock<NetworkMock>>();

  OlpClientSettings settings;
  settings.cache = cache;
  settings.network_request_handler = network;
  settings.retry_settings.timeout = 1;

  const std::string catalog = "hrn:here:data:::hereos-internal-test-v2";
  const auto catalog_hrn = HRN::FromString(catalog);
  const std::string service_name = "random_service";
  const std::string service_url = "http://random_service.com";
  const std::string service_version = "v8";
  const std::string config_url =
      "https://config.data.api.platform.in.here.com/config/v1";
  const std::string cache_key =
      catalog + "::" + service_name + "::" + service_version + "::api";
  const std::string lookup_url =
      "https://api-lookup.data.api.platform.here.com/lookup/v1/resources/" +
      catalog + "/apis/" + service_name + "/" + service_version;

  {
    SCOPED_TRACE("Fetch from cache [CacheOnly] positive");
    EXPECT_CALL(*cache, Get(cache_key, _))
        .Times(1)
        .WillOnce(Return(service_url));

    client::CancellationContext context;
    auto response = ApiClientLookup::LookupApi(
        catalog_hrn, context, service_name, service_version,
        FetchOptions::CacheOnly, settings);

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), service_url);
    Mock::VerifyAndClearExpectations(cache.get());
  }
  {
    SCOPED_TRACE("Fetch from cache [CacheOnly] negative");
    EXPECT_CALL(*cache, Get(cache_key, _))
        .Times(1)
        .WillOnce(Return(boost::any()));

    client::CancellationContext context;
    auto response = ApiClientLookup::LookupApi(
        catalog_hrn, context, service_name, service_version,
        FetchOptions::CacheOnly, settings);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), ErrorCode::NotFound);
    Mock::VerifyAndClearExpectations(cache.get());
  }
  {
    SCOPED_TRACE("Fetch from network");
    EXPECT_CALL(*network, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(
                                         olp::http::HttpStatusCode::OK),
                                     OLP_SDK_HTTP_RESPONSE_LOOKUP_CONFIG));
    EXPECT_CALL(*cache, Put(cache_key, _, _, _))
        .Times(1)
        .WillOnce(Return(true));

    client::CancellationContext context;
    auto response = ApiClientLookup::LookupApi(
        catalog_hrn, context, service_name, service_version,
        FetchOptions::OnlineOnly, settings);

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_EQ(response.GetResult().GetBaseUrl(), OLP_SDK_CONFIG_BASE_URL);
    Mock::VerifyAndClearExpectations(network.get());
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
    auto response = ApiClientLookup::LookupApi(
        catalog_hrn, context, service_name, service_version,
        FetchOptions::OnlineOnly, settings);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::AccessDenied);
    Mock::VerifyAndClearExpectations(network.get());
  }
  {
    SCOPED_TRACE("Network request cancelled by network internally");
    client::CancellationContext context;
    EXPECT_CALL(*network, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce([=](olp::http::NetworkRequest request,
                      olp::http::Network::Payload payload,
                      olp::http::Network::Callback callback,
                      olp::http::Network::HeaderCallback header_callback,
                      olp::http::Network::DataCallback data_callback)
                      -> olp::http::SendOutcome {
          return olp::http::SendOutcome(olp::http::ErrorCode::CANCELLED_ERROR);
        });

    auto response = ApiClientLookup::LookupApi(
        catalog_hrn, context, service_name, service_version,
        FetchOptions::OnlineOnly, settings);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::Cancelled);
    Mock::VerifyAndClearExpectations(network.get());
  }
  {
    SCOPED_TRACE("Network request timed out");
    client::CancellationContext context;
    EXPECT_CALL(*network, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce([=](olp::http::NetworkRequest request,
                      olp::http::Network::Payload payload,
                      olp::http::Network::Callback callback,
                      olp::http::Network::HeaderCallback header_callback,
                      olp::http::Network::DataCallback data_callback)
                      -> olp::http::SendOutcome {
          // note no network response thread spawns
          constexpr auto unused_request_id = 12;
          return olp::http::SendOutcome(unused_request_id);
        });
    EXPECT_CALL(*network, Cancel(_)).Times(1).WillOnce(Return());

    auto response = ApiClientLookup::LookupApi(
        catalog_hrn, context, service_name, service_version,
        FetchOptions::OnlineOnly, settings);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::RequestTimeout);
    Mock::VerifyAndClearExpectations(network.get());
  }
  {
    SCOPED_TRACE("Network request cancelled by user");
    client::CancellationContext context;
    EXPECT_CALL(*network, Send(IsGetRequest(lookup_url), _, _, _, _))
        .Times(1)
        .WillOnce(
            [=, &context](olp::http::NetworkRequest request,
                          olp::http::Network::Payload payload,
                          olp::http::Network::Callback callback,
                          olp::http::Network::HeaderCallback header_callback,
                          olp::http::Network::DataCallback data_callback)
                -> olp::http::SendOutcome {
              // spawn a 'user' response of cancelling
              std::thread([&context]() { context.CancelOperation(); }).detach();

              // note no network response thread spawns

              constexpr auto unused_request_id = 12;
              return olp::http::SendOutcome(unused_request_id);
            });
    EXPECT_CALL(*network, Cancel(_)).Times(1).WillOnce(Return());

    auto response = ApiClientLookup::LookupApi(
        catalog_hrn, context, service_name, service_version,
        FetchOptions::OnlineOnly, settings);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::Cancelled);
    Mock::VerifyAndClearExpectations(network.get());
  }
  {
    SCOPED_TRACE("Network request cancelled before execution setup");
    client::CancellationContext context;

    context.CancelOperation();
    auto response = ApiClientLookup::LookupApi(
        catalog_hrn, context, service_name, service_version,
        FetchOptions::OnlineOnly, settings);

    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              olp::client::ErrorCode::Cancelled);
    Mock::VerifyAndClearExpectations(network.get());
  }
}
