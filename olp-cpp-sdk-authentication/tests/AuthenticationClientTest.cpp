/*
 * Copyright (C) 2020-2026 HERE Europe B.V.
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

#include <memory>
#include <string>
#include <thread>

#include <gmock/gmock.h>

#include <matchers/NetworkUrlMatchers.h>
#include "AuthenticationClientImpl.h"
#include "AuthenticationClientImplTestable.h"
#include "AuthenticationClientUtils.h"
#include "AuthenticationMockedResponses.h"
#include "mocks/NetworkMock.h"

namespace {
constexpr auto kTime = "Fri, 29 May 2020 11:07:45 GMT";
constexpr auto kEpochTime = "Thu, 1 Jan 1970 00:00:00 GMT";
constexpr auto kSummerTime = "Tue, 18 Jun 2024 12:25:35 GMT";

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;
using std::placeholders::_5;
using std::placeholders::_6;
using std::placeholders::_7;
using testing::_;
using testing::Contains;
using testing::DoAll;
using testing::ElementsAreArray;
using testing::Not;
using testing::Pair;
using testing::Return;
using testing::SaveArg;
using testing::WithArg;

constexpr auto kBlob1 = "1st string blob";
constexpr auto kBlob2 = "2nd string blob";
constexpr auto kBlob3 = "3rd string blob";
constexpr auto kMtlsTokenEndpointUrl =
    "https://mtls.account.api.here.com/mtls/token";

}  // namespace

namespace auth = olp::authentication;
namespace client = olp::client;

using AuthenticationClientImplTestable =
    mocks::AuthenticationClientImplTestable;

ACTION_P(Wait, time) { std::this_thread::sleep_for(time); }

TEST(AuthenticationClientTest, AuthenticationWithoutNetwork) {
  auth::AuthenticationSettings settings;
  settings.network_request_handler = nullptr;

  AuthenticationClientImplTestable auth_impl(settings);

  const auth::AuthenticationCredentials credentials("", "");

  {
    SCOPED_TRACE("SignUpHereUser, Offline");

    auth_impl.SignUpHereUser(
        credentials, {},
        [=](const auth::AuthenticationClient::SignUpResponse& response) {
          EXPECT_FALSE(response.IsSuccessful());
          EXPECT_EQ(response.GetError().GetErrorCode(),
                    client::ErrorCode::NetworkConnection);
        });
  }

  {
    SCOPED_TRACE("SignOut, Offline");

    auth_impl.SignOut(
        credentials, {},
        [=](const auth::AuthenticationClient::SignOutUserResponse& response) {
          EXPECT_FALSE(response.IsSuccessful());
          EXPECT_EQ(response.GetError().GetErrorCode(),
                    client::ErrorCode::NetworkConnection);
        });
  }
}

TEST(AuthenticationClientTest, SignUpWithUnsuccessfulSend) {
  using testing::_;

  auth::AuthenticationSettings settings;
  auto networkMock = std::make_shared<NetworkMock>();

  ON_CALL(*networkMock, Send(_, _, _, _, _))
      .WillByDefault([](olp::http::NetworkRequest, olp::http::Network::Payload,
                        olp::http::Network::Callback,
                        olp::http::Network::HeaderCallback,
                        olp::http::Network::DataCallback) {
        return olp::http::SendOutcome(olp::http::ErrorCode::UNKNOWN_ERROR);
      });

  settings.network_request_handler = networkMock;

  AuthenticationClientImplTestable auth_impl(settings);

  const auth::AuthenticationCredentials credentials("", "");

  auth_impl.SignUpHereUser(
      credentials, {},
      [=](const auth::AuthenticationClient::SignUpResponse& response) {
        EXPECT_FALSE(response.IsSuccessful());
        EXPECT_EQ(response.GetError().GetErrorCode(),
                  client::ErrorCode::Unknown);
      });
}

TEST(AuthenticationClientTest, SignOutAccessDenied) {
  using testing::_;

  auth::AuthenticationSettings settings;
  settings.network_request_handler = std::make_shared<NetworkMock>();

  AuthenticationClientImplTestable auth_impl(settings);

  const auth::AuthenticationCredentials credentials("", "");

  auth_impl.SignOut(
      credentials, {},
      [=](const auth::AuthenticationClient::SignOutUserResponse& response) {
        EXPECT_FALSE(response.IsSuccessful());
        EXPECT_EQ(response.GetError().GetErrorCode(),
                  client::ErrorCode::AccessDenied);
      });
}

TEST(AuthenticationClientTest, Timestamp) {
  using testing::_;

  auth::AuthenticationSettings settings;
  settings.use_system_time = false;
  settings.network_request_handler = std::make_shared<NetworkMock>();

  AuthenticationClientImplTestable auth_impl(settings);

  const std::time_t initial_time = 10;
  const std::time_t time_limit = 20;

  const auth::AuthenticationCredentials credentials("", "");

  const auto timestamp_predicate = testing::AllOf(
      testing::Ge(initial_time), testing::Le(initial_time + time_limit));

  const auto request_time = std::chrono::milliseconds(500);

  const client::HttpResponse retriable_response(
      olp::http::HttpStatusCode::TOO_MANY_REQUESTS);

  {
    SCOPED_TRACE("SignInClient");

    EXPECT_CALL(auth_impl, GetTimeFromServer(_, _))
        .WillOnce(testing::Return(initial_time));

    std::time_t time = 0;

    EXPECT_CALL(auth_impl, CallAuth(_, _, _, _, _, timestamp_predicate, _))
        .Times(3)
        .WillRepeatedly(testing::DoAll(testing::SaveArg<5>(&time),
                                       Wait(request_time),
                                       testing::Return(retriable_response)));

    auth_impl.SignInClient(credentials, {}, nullptr);

    EXPECT_GT(time, initial_time);
  }
  {
    SCOPED_TRACE("SignInHereUser");

    EXPECT_CALL(auth_impl, GetTimeFromServer(_, _))
        .WillOnce(testing::Return(initial_time));

    std::time_t time = 0;

    EXPECT_CALL(auth_impl, CallAuth(_, _, _, _, _, timestamp_predicate, _))
        .Times(3)
        .WillRepeatedly(testing::DoAll(testing::SaveArg<5>(&time),
                                       Wait(request_time),
                                       testing::Return(retriable_response)));

    auth_impl.SignInHereUser(credentials, {}, nullptr);

    EXPECT_GT(time, initial_time);
  }
  {
    SCOPED_TRACE("SignInRefresh");

    EXPECT_CALL(auth_impl, GetTimeFromServer(_, _))
        .WillOnce(testing::Return(initial_time));

    std::time_t time = 0;

    EXPECT_CALL(auth_impl, CallAuth(_, _, _, _, _, timestamp_predicate, _))
        .Times(3)
        .WillRepeatedly(testing::DoAll(testing::SaveArg<5>(&time),
                                       Wait(request_time),
                                       testing::Return(retriable_response)));

    auth_impl.SignInRefresh(credentials, {}, nullptr);

    EXPECT_GT(time, initial_time);
  }
  {
    SCOPED_TRACE("SignInFederated");

    EXPECT_CALL(auth_impl, GetTimeFromServer(_, _))
        .WillOnce(testing::Return(initial_time));

    std::time_t time = 0;

    EXPECT_CALL(auth_impl, CallAuth(_, _, _, _, _, timestamp_predicate, _))
        .Times(3)
        .WillRepeatedly(testing::DoAll(testing::SaveArg<5>(&time),
                                       Wait(request_time),
                                       testing::Return(retriable_response)));

    auth_impl.SignInFederated(credentials, {}, nullptr);

    EXPECT_GT(time, initial_time);
  }
}

TEST(AuthenticationClientTest, TimeParsing) {
  {
    SCOPED_TRACE("Parse time");
    EXPECT_EQ(auth::ParseTime(kTime), 1590750465);
  }

  {
    SCOPED_TRACE("Parse epoch time");
    EXPECT_EQ(auth::ParseTime(kEpochTime), 0);
  }

  {
    SCOPED_TRACE("Parse summer time");
    EXPECT_EQ(auth::ParseTime(kSummerTime), 1718713535);
  }
}

TEST(AuthenticationClientTest, GenerateAuthorizationHeader) {
  auth::AuthenticationCredentials credentials("key", "secret");
  const auto url = "https://auth.server.com";
  auto sig = auth::GenerateAuthorizationHeader(credentials, url, 0, "unique");
  auto expected_sig =
      "OAuth "
      "oauth_consumer_key=\"key\",oauth_nonce=\"unique\",oauth_signature_"
      "method=\"HMAC-SHA256\",oauth_timestamp=\"0\",oauth_version=\"1.0\","
      "oauth_signature=\"g1pNnGH65Pl%2B%2FoUNm%2BJBAM9%2BjjgmSuknucOiOwFGFQE%"
      "3D\"";
  EXPECT_EQ(sig, expected_sig);
}

TEST(AuthenticationClientTest, SignInWithCustomUrlAndBody) {
  constexpr auto custom_url = "https://example.com/user/login";
  const auto custom_body = std::string("custom_body");
  olp::http::NetworkRequest expected_request{""};

  const auth::AuthenticationCredentials credentials("", "", custom_url);
  auth::SignInProperties properties;
  properties.custom_body = custom_body;

  auth::AuthenticationSettings settings;
  auto network_mock = std::make_shared<NetworkMock>();
  settings.network_request_handler = network_mock;

  AuthenticationClientImplTestable auth_impl(settings);

  EXPECT_CALL(*network_mock, Send)
      .WillOnce(DoAll(
          SaveArg<0>(&expected_request),
          Return(olp::http::SendOutcome(olp::http::ErrorCode::UNKNOWN_ERROR))));

  EXPECT_CALL(auth_impl, CallAuth)
      .WillOnce(std::bind(&AuthenticationClientImplTestable::RealCallAuth,
                          &auth_impl, _1, _2, _3, _4, _5, _6, _7));

  auth_impl.SignInClient(
      credentials, properties,
      [=](const auth::AuthenticationClient::SignInClientResponse& response) {
        EXPECT_FALSE(response.IsSuccessful());
        EXPECT_EQ(response.GetError().GetErrorCode(),
                  client::ErrorCode::Unknown);
      });

  EXPECT_EQ(expected_request.GetUrl(), custom_url);
  EXPECT_THAT(*expected_request.GetBody(), ElementsAreArray(custom_body));
  EXPECT_THAT(expected_request.GetHeaders(),
              Not(Contains(Pair("Content-Type", _))));
}

TEST(AuthenticationClientTest, SignInMtls) {
  auth::AuthenticationSettings settings;
  settings.token_endpoint_url = kMtlsTokenEndpointUrl;
  settings.network_request_handler =
      std::make_shared<testing::StrictMock<NetworkMock>>();

  {
    SCOPED_TRACE("Failed to create network");

    AuthenticationClientImplTestable auth_impl(settings);

    olp::http::NetworkInitializationSettings actual_network_settings;
    EXPECT_CALL(auth_impl, CreateNetworkRequestHandler(_))
        .WillOnce(DoAll(SaveArg<0>(&actual_network_settings), Return(nullptr)));

    auth::MtlsProperties properties;
    properties.ca_cert_pem = kBlob1;
    properties.client_cert_pem = kBlob2;
    properties.client_key_pem = kBlob3;

    std::promise<auth::AuthenticationClient::SignInClientResponse>
        response_promise;
    auth_impl.SignInMtls(
        properties,
        [&](const auth::AuthenticationClient::SignInClientResponse& response) {
          response_promise.set_value(response);
        });

    auto request_future = response_promise.get_future();
    auto response = request_future.get();
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              client::ErrorCode::NetworkConnection);

    const auto& certificates = actual_network_settings.certificate_settings;
    EXPECT_EQ(properties.ca_cert_pem, certificates.cert_file_blob);
    EXPECT_EQ(properties.client_cert_pem, certificates.client_cert_file_blob);
    EXPECT_EQ(properties.client_key_pem, certificates.client_key_file_blob);
  }

  {
    SCOPED_TRACE("Failed to Send. Retriable error then non retriable");

    AuthenticationClientImplTestable auth_impl(settings);

    auto auth_network_mock =
        std::make_shared<testing::StrictMock<NetworkMock>>();
    EXPECT_CALL(auth_impl, CreateNetworkRequestHandler(_))
        .WillOnce(Return(auth_network_mock));

    std::vector<olp::http::NetworkRequest> actual_requests;
    const std::string kScope = "random_scope";

    EXPECT_CALL(*auth_network_mock, Send)
        .WillOnce(DoAll(
            WithArg<0>([&](olp::http::NetworkRequest request) {
              EXPECT_THAT(request, IsPostRequest(settings.token_endpoint_url));
              EXPECT_THAT(request,
                          BodyContains("\"scope\":\"" + kScope + "\""));
              actual_requests.emplace_back(std::move(request));
            }),
            Return(olp::http::SendOutcome(
                olp::http::ErrorCode::NETWORK_OVERLOAD_ERROR))))
        .WillOnce(DoAll(
            WithArg<0>([&](olp::http::NetworkRequest request) {
              EXPECT_THAT(request, IsPostRequest(settings.token_endpoint_url));
              EXPECT_THAT(request,
                          BodyContains("\"scope\":\"" + kScope + "\""));
              actual_requests.emplace_back(request);
            }),
            Return(
                olp::http::SendOutcome(olp::http::ErrorCode::UNKNOWN_ERROR))));

    auth::MtlsProperties properties;
    properties.scope = kScope;

    std::promise<auth::AuthenticationClient::SignInClientResponse>
        response_promise;
    auth_impl.SignInMtls(
        properties,
        [&](const auth::AuthenticationClient::SignInClientResponse& response) {
          response_promise.set_value(response);
        });

    auto request_future = response_promise.get_future();
    auto response = request_future.get();
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(), client::ErrorCode::Unknown);

    EXPECT_EQ(actual_requests.size(), 2U);
  }

  {
    SCOPED_TRACE("Failed to Send. Failed all retries");

    AuthenticationClientImplTestable auth_impl(settings);

    auto auth_network_mock =
        std::make_shared<testing::StrictMock<NetworkMock>>();
    EXPECT_CALL(auth_impl, CreateNetworkRequestHandler(_))
        .WillOnce(Return(auth_network_mock));

    std::vector<olp::http::NetworkRequest> actual_requests;
    const std::string kScope = "scope 02";

    EXPECT_CALL(*auth_network_mock, Send)
        .Times(settings.retry_settings.max_attempts)
        .WillRepeatedly(DoAll(
            WithArg<0>([&](olp::http::NetworkRequest request) {
              EXPECT_THAT(request, IsPostRequest(settings.token_endpoint_url));
              EXPECT_THAT(request,
                          BodyContains("\"scope\":\"" + kScope + "\""));
              actual_requests.emplace_back(std::move(request));
            }),
            Return(olp::http::SendOutcome(olp::http::ErrorCode::IO_ERROR))));

    auth::MtlsProperties properties;
    properties.scope = kScope;

    std::promise<auth::AuthenticationClient::SignInClientResponse>
        response_promise;
    auth_impl.SignInMtls(
        properties,
        [&](const auth::AuthenticationClient::SignInClientResponse& response) {
          response_promise.set_value(response);
        });

    auto request_future = response_promise.get_future();
    auto response = request_future.get();
    EXPECT_FALSE(response.IsSuccessful());
    EXPECT_EQ(response.GetError().GetErrorCode(),
              client::ErrorCode::NetworkConnection);

    EXPECT_EQ(actual_requests.size(), settings.retry_settings.max_attempts);
  }

  {
    SCOPED_TRACE("Success");

    AuthenticationClientImplTestable auth_impl(settings);

    auto auth_network_mock =
        std::make_shared<testing::StrictMock<NetworkMock>>();
    EXPECT_CALL(auth_impl, CreateNetworkRequestHandler(_))
        .WillOnce(Return(auth_network_mock));

    const std::string kScope = "scope";

    EXPECT_CALL(*auth_network_mock, Send)
        .WillOnce(DoAll(
            WithArg<0>([&](olp::http::NetworkRequest request) {
              EXPECT_THAT(request, IsPostRequest(settings.token_endpoint_url));
              EXPECT_THAT(request,
                          BodyContains("\"scope\":\"" + kScope + "\""));
            }),
            ReturnHttpResponse(GetResponse(olp::http::HttpStatusCode::OK),
                               kResponseWithScope)));

    auth::MtlsProperties properties;
    properties.scope = kScope;

    std::promise<auth::AuthenticationClient::SignInClientResponse>
        response_promise;
    auth_impl.SignInMtls(
        properties,
        [&](const auth::AuthenticationClient::SignInClientResponse& response) {
          response_promise.set_value(response);
        });

    auto request_future = response_promise.get_future();
    auto response = request_future.get();

    EXPECT_TRUE(response.IsSuccessful());
    EXPECT_FALSE(response.GetResult().GetAccessToken().empty());
    EXPECT_EQ(kResponseToken, response.GetResult().GetAccessToken());
    EXPECT_EQ("bearer", response.GetResult().GetTokenType());
    EXPECT_EQ(response.GetResult().GetScope(), kScope);
  }
}
