/*
 * Copyright (C) 2020-2025 HERE Europe B.V.
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

#include "AuthenticationClientImpl.h"
#include "AuthenticationClientUtils.h"
#include "mocks/NetworkMock.h"

namespace {
constexpr auto kTime = "Fri, 29 May 2020 11:07:45 GMT";
constexpr auto kEpochTime = "Thu, 1 Jan 1970 00:00:00 GMT";
constexpr auto kSummerTime = "Tue, 18 Jun 2024 12:25:35 GMT";
}  // namespace

namespace auth = olp::authentication;
namespace client = olp::client;

class AuthenticationClientImplTestable : public auth::AuthenticationClientImpl {
 public:
  explicit AuthenticationClientImplTestable(
      auth::AuthenticationSettings settings)
      : AuthenticationClientImpl(settings) {}

  MOCK_METHOD(auth::TimeResponse, GetTimeFromServer,
              (client::CancellationContext context,
               const client::OlpClient& client),
              (const, override));

  MOCK_METHOD(client::HttpResponse, CallAuth,
              (const client::OlpClient&, const std::string&,
               client::CancellationContext,
               const auth::AuthenticationCredentials&,
               client::OlpClient::RequestBodyType, std::time_t,
               const std::string&),
              (override));

  client::HttpResponse RealCallAuth(
      const client::OlpClient& client, const std::string& endpoint,
      client::CancellationContext context,
      const auth::AuthenticationCredentials& credentials,
      client::OlpClient::RequestBodyType body, std::time_t time,
      const std::string& content_type) {
    return auth::AuthenticationClientImpl::CallAuth(
        client, endpoint, std::move(context), credentials, std::move(body),
        time, content_type);
  }
};

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
  // Making CPPLINT happy
  using testing::_;
  using testing::Contains;
  using testing::DoAll;
  using testing::ElementsAreArray;
  using testing::Not;
  using testing::Pair;
  using testing::Return;
  using testing::SaveArg;

  using std::placeholders::_1;
  using std::placeholders::_2;
  using std::placeholders::_3;
  using std::placeholders::_4;
  using std::placeholders::_5;
  using std::placeholders::_6;
  using std::placeholders::_7;

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
