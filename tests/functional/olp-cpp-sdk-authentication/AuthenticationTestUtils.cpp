/*
 * Copyright (C) 2019-2025 HERE Europe B.V.
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

#include "AuthenticationTestUtils.h"

#include <future>
#include <thread>

#ifndef WIN32
#include <unistd.h>
#endif

#include <olp/core/http/HttpStatusCode.h>
#include <olp/core/http/Network.h>
#include <olp/core/logging/Log.h>

#include <boost/json/parse.hpp>
#include <boost/json/system_error.hpp>
#include <testutils/CustomParameters.hpp>
#include "TestConstants.h"

using namespace ::olp::authentication;

namespace {
constexpr auto kFacebookUrl = "https://graph.facebook.com/v2.12";
constexpr auto kId = "id";

constexpr auto kGoogleApiUrl = "https://www.googleapis.com/";
constexpr auto kGoogleOauth2Endpoint = "oauth2/v3/token";
constexpr auto kGoogleClientIdParam = "client_id";
constexpr auto kGoogleClientSecretParam = "client_secret";
constexpr auto kGoogleRefreshTokenParam = "refresh_token";
constexpr auto kGoogleRefreshTokenGrantType = "grant_type=refresh_token";

constexpr auto kArcgisUrl = "https://www.arcgis.com/sharing/rest/oauth2/token";
}  // namespace

bool AuthenticationTestUtils::CreateFacebookTestUser(
    olp::http::Network &network,
    const olp::http::NetworkSettings &network_settings, FacebookUser &user,
    const std::string &permissions) {
  constexpr auto kTestUserPath = "/accounts/test-users";

  std::string url = std::string() + kFacebookUrl + "/" +
                    CustomParameters::getArgument("facebook_app_id") +
                    kTestUserPath;
  url.append(kQuestionParam);
  url.append(kAccessToken + kEqualsParam +
             CustomParameters::getArgument("facebook_access_token"));
  url.append(kAndParam);
  url.append("installed" + kEqualsParam + "true");
  url.append(kAndParam);
  url.append("name" + kEqualsParam + kTestUserName);
  if (!permissions.empty()) {
    url.append(kAndParam);
    url.append("permissions" + kEqualsParam + permissions);
  }
  olp::http::NetworkRequest request(url);
  request.WithVerb(olp::http::NetworkRequest::HttpVerb::POST);
  request.WithSettings(network_settings);

  unsigned int retry = 0u;
  do {
    if (retry > 0u) {
      OLP_SDK_LOG_WARNING(__func__,
                          "Request retry attempted (" << retry << ")");
      std::this_thread::sleep_for(
          std::chrono::seconds(retry * kRetryDelayInSecs));
    }

    auto payload = std::make_shared<std::stringstream>();

    std::promise<void> promise;
    auto future = promise.get_future();
    network.Send(
        request, payload,
        [payload, &promise,
         &user](const olp::http::NetworkResponse &network_response) {
          user.token.status = network_response.GetStatus();
          if (user.token.status == olp::http::HttpStatusCode::OK) {
            boost::json::error_code ec;
            auto document = boost::json::parse(*payload, ec);
            const bool is_valid = !ec.failed() && document.is_object() &&
                                  document.as_object().contains(kAccessToken) &&
                                  document.as_object().contains(kId);

            if (is_valid) {
              user.token.access_token =
                  document.as_object()[kAccessToken].get_string().c_str();
              user.id = document.as_object()[kId].get_string().c_str();
            }
          }
          promise.set_value();
        });
    future.wait();
  } while ((user.token.status < 0) && (++retry < kMaxRetryCount));

  return !user.id.empty() && !user.token.access_token.empty();
}

bool AuthenticationTestUtils::DeleteFacebookTestUser(
    olp::http::Network &network,
    const olp::http::NetworkSettings &network_settings,
    const std::string &user_id) {
  std::string url = std::string() + kFacebookUrl + "/" + user_id;
  url.append(kQuestionParam);
  url.append(kAccessToken + kEqualsParam +
             CustomParameters::getArgument("facebook_access_token"));
  olp::http::NetworkRequest request(url);
  request.WithVerb(olp::http::NetworkRequest::HttpVerb::DEL);
  request.WithSettings(network_settings);

  int status = 0;
  unsigned int retry = 0u;
  do {
    if (retry > 0u) {
      OLP_SDK_LOG_WARNING(__func__,
                          "Request retry attempted (" << retry << ")");
      std::this_thread::sleep_for(
          std::chrono::seconds(retry * kRetryDelayInSecs));
    }

    auto payload = std::make_shared<std::stringstream>();

    std::promise<void> promise;
    auto future = promise.get_future();
    network.Send(request, payload,
                 [payload, &promise,
                  &status](const olp::http::NetworkResponse &network_response) {
                   status = network_response.GetStatus();
                   promise.set_value();
                 });
    future.wait();
  } while ((status < 0) && (++retry < kMaxRetryCount));

  return (status == olp::http::HttpStatusCode::OK);
}

bool AuthenticationTestUtils::GetGoogleAccessToken(
    olp::http::Network &network,
    const olp::http::NetworkSettings &network_settings,
    AccessTokenResponse &token) {
  std::string url = std::string() + kGoogleApiUrl + kGoogleOauth2Endpoint;
  url.append(kQuestionParam);
  url.append(kGoogleClientIdParam + kEqualsParam +
             CustomParameters::getArgument("google_client_id"));
  url.append(kAndParam);
  url.append(kGoogleClientSecretParam + kEqualsParam +
             CustomParameters::getArgument("google_client_secret"));
  url.append(kAndParam);
  url.append(kGoogleRefreshTokenParam + kEqualsParam +
             CustomParameters::getArgument("google_client_token"));
  url.append(kAndParam);
  url.append(kGoogleRefreshTokenGrantType);

  olp::http::NetworkRequest request(url);
  request.WithVerb(olp::http::NetworkRequest::HttpVerb::POST);
  request.WithSettings(network_settings);

  return GetAccessTokenImpl(network, request, token);
}

std::shared_ptr<std::vector<unsigned char>>
AuthenticationTestUtils::GenerateArcGisClientBody() {
  constexpr auto kGrantType = "grant_type";
  constexpr auto kClientId = "client_id";
  constexpr auto kRefreshToken = "refresh_token";

  std::string data;
  data.append(kClientId)
      .append(kEqualsParam)
      .append(CustomParameters::getArgument("arcgis_app_id"))
      .append(kAndParam);
  data.append(kGrantType)
      .append(kEqualsParam)
      .append(kRefreshToken)
      .append(kAndParam);
  data.append(kRefreshToken)
      .append(kEqualsParam)
      .append(CustomParameters::getArgument("arcgis_access_token"));
  return std::make_shared<std::vector<unsigned char>>(data.begin(), data.end());
}

bool AuthenticationTestUtils::GetArcGisAccessToken(
    olp::http::Network &network,
    const olp::http::NetworkSettings &network_settings,
    AccessTokenResponse &token) {
  olp::http::NetworkRequest request(kArcgisUrl);
  request.WithVerb(olp::http::NetworkRequest::HttpVerb::POST);
  request.WithSettings(network_settings);

  request.WithBody(GenerateArcGisClientBody());
  request.WithHeader("content-type", "application/x-www-form-urlencoded");
  request.WithSettings(network_settings);

  return GetAccessTokenImpl(network, request, token);
}

bool AuthenticationTestUtils::GetAccessTokenImpl(
    olp::http::Network &network, const olp::http::NetworkRequest &request,
    AccessTokenResponse &token) {
  unsigned int retry = 0u;
  do {
    if (retry > 0u) {
      OLP_SDK_LOG_WARNING(__func__,
                          "Request retry attempted (" << retry << ")");
      std::this_thread::sleep_for(
          std::chrono::seconds(retry * kRetryDelayInSecs));
    }

    auto payload = std::make_shared<std::stringstream>();

    std::promise<void> promise;
    auto future = promise.get_future();
    network.Send(
        request, payload,
        [payload, &promise,
         &token](const olp::http::NetworkResponse &network_response) {
          token.status = network_response.GetStatus();
          if (token.status == olp::http::HttpStatusCode::OK) {
            boost::json::error_code ec;
            auto document = boost::json::parse(*payload, ec);
            bool is_valid = !ec.failed() && document.is_object() &&
                            document.as_object().contains(kAccessToken);
            if (is_valid) {
              token.access_token =
                  document.as_object()[kAccessToken].get_string().c_str();
            }
          }
          promise.set_value();
        });
    future.wait();
  } while ((token.status < 0) && (++retry < kMaxRetryCount));

  return !token.access_token.empty();
}

void AuthenticationTestUtils::DeleteHereUser(
    olp::http::Network &network,
    const olp::http::NetworkSettings &network_settings,
    const std::string &user_bearer_token,
    const DeleteHereUserCallback &callback) {
  constexpr auto kAuthorization = "Authorization";
  constexpr auto kContentType = "Content-Type";
  constexpr auto kApplicationJson = "application/json";
  constexpr auto kDeleteUserEndpoint = "/user/me";

  std::string url = kHereAccountStagingURL;
  url.append(kDeleteUserEndpoint);

  olp::http::NetworkRequest request(url);
  request.WithVerb(olp::http::NetworkRequest::HttpVerb::DEL);
  request.WithHeader(kAuthorization, GenerateBearerHeader(user_bearer_token));
  request.WithHeader(kContentType, kApplicationJson);
  request.WithSettings(network_settings);

  std::shared_ptr<std::stringstream> payload =
      std::make_shared<std::stringstream>();
  network.Send(
      request, payload,
      [callback, payload](const olp::http::NetworkResponse &network_response) {
        DeleteUserResponse response;
        response.status = network_response.GetStatus();
        response.error = network_response.GetError();
        callback(response);
      });
}

std::string AuthenticationTestUtils::GenerateBearerHeader(
    const std::string &user_bearer_token) {
  std::string authorization = "Bearer ";
  authorization += user_bearer_token;
  return authorization;
}
