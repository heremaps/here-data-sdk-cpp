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
#include "FacebookTestUtils.h"
#include <olp/core/http/HttpStatusCode.h>
#include <olp/core/http/Network.h>
#include <olp/core/http/NetworkRequest.h>
#include <olp/core/http/NetworkResponse.h>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <future>
#include <sstream>
#include "CommonTestUtils.h"
#include "testutils/CustomParameters.hpp"

#include <iostream>

namespace {
// constexpr auto USER_PERMISIONS = "email";
constexpr auto kInstalledStatus = "true";

constexpr auto kTestUserPath = "/accounts/test-users";
constexpr auto kFacebookUrl = "https://graph.facebook.com/v2.12";

constexpr auto kInstalled = "installed";
constexpr auto kName = "name";
constexpr auto kPermissions = "permissions";
constexpr auto kId = "id";
}  // namespace

namespace olp {
namespace http {
class Network;
class NetworkSettings;
}  // namespace http

namespace authentication {
class FacebookTestUtils::Impl {
 public:
  /**
   * @brief Constructor
   * @param token_cache_limit Maximum number of tokens that will be cached.
   */
  Impl() = default;

  virtual ~Impl() = default;

  bool CreateFacebookTestUser(
      http::Network& network,
      const olp::http::NetworkSettings& network_settings, FacebookUser& user,
      std::string permissions);
  bool DeleteFacebookTestUser(
      olp::http::Network& network,
      const olp::http::NetworkSettings& network_settings,
      const std::string& user_id);
};

bool FacebookTestUtils::Impl::CreateFacebookTestUser(
    olp::http::Network& network,
    const olp::http::NetworkSettings& network_settings, FacebookUser& user,
    std::string permissions) {
  std::string url = std::string() + kFacebookUrl + "/" +
                    CustomParameters::getArgument("facebook_app_id") +
                    kTestUserPath;
  ;
  url.append(QUESTION_PARAM);
  url.append(ACCESS_TOKEN + EQUALS_PARAM +
             CustomParameters::getArgument("facebook_access_token"));
  url.append(AND_PARAM);
  url.append(kInstalled + EQUALS_PARAM + kInstalledStatus);
  url.append(AND_PARAM);
  url.append(kName + EQUALS_PARAM + TEST_USER_NAME);
  if (!permissions.empty()) {
    url.append(AND_PARAM);
    url.append(kPermissions + EQUALS_PARAM + permissions);
  }
  olp::http::NetworkRequest request(url);
  request.WithVerb(http::NetworkRequest::HttpVerb::POST);
  request.WithSettings(network_settings);

  unsigned int retry = 0u;
  do {
    if (retry > 0u) {
      OLP_SDK_LOG_WARNING(__func__,
                          "Request retry attempted (" << retry << ")");
      std::this_thread::sleep_for(
          std::chrono::seconds(retry * RETRY_DELAY_SECS));
    }

    auto payload = std::make_shared<std::stringstream>();

    std::promise<void> promise;
    auto future = promise.get_future();
    network.Send(
        request, payload,
        [payload, &promise,
         &user](const olp::http::NetworkResponse& network_response) {
          user.status = network_response.GetStatus();
          if (user.status == olp::http::HttpStatusCode::OK) {
            auto document = std::make_shared<rapidjson::Document>();
            rapidjson::IStreamWrapper stream(*payload.get());
            document->ParseStream(stream);
            bool is_valid = !document->HasParseError() &&
                            document->HasMember(ACCESS_TOKEN.c_str()) &&
                            document->HasMember(kId);

            if (is_valid) {
              user.access_token = (*document)[ACCESS_TOKEN.c_str()].GetString();
              user.id = (*document)[kId].GetString();
            }
          }
          promise.set_value();
        });
    future.wait();
  } while ((user.status < 0) && (++retry < MAX_RETRY_COUNT));

  return !user.id.empty() && !user.access_token.empty();
}

bool FacebookTestUtils::Impl::DeleteFacebookTestUser(
    http::Network& network, const olp::http::NetworkSettings& network_settings,
    const std::string& user_id) {
  std::string url = std::string() + kFacebookUrl + "/" + user_id;
  url.append(QUESTION_PARAM);
  url.append(ACCESS_TOKEN + EQUALS_PARAM +
             CustomParameters::getArgument("facebook_access_token"));
  olp::http::NetworkRequest request(url);
  request.WithVerb(http::NetworkRequest::HttpVerb::DEL);
  request.WithSettings(network_settings);

  int status = 0;
  unsigned int retry = 0u;
  do {
    if (retry > 0u) {
      OLP_SDK_LOG_WARNING(__func__,
                          "Request retry attempted (" << retry << ")");
      std::this_thread::sleep_for(
          std::chrono::seconds(retry * RETRY_DELAY_SECS));
    }

    auto payload = std::make_shared<std::stringstream>();

    std::promise<void> promise;
    auto future = promise.get_future();
    network.Send(request, payload,
                 [payload, &promise,
                  &status](const olp::http::NetworkResponse& network_response) {
                   status = network_response.GetStatus();
                   promise.set_value();
                 });
    future.wait();
  } while ((status < 0) && (++retry < MAX_RETRY_COUNT));

  return (status == olp::http::HttpStatusCode::OK);
}

FacebookTestUtils::FacebookTestUtils()
    : impl_(std::make_unique<FacebookTestUtils::Impl>()) {}

FacebookTestUtils::~FacebookTestUtils() = default;

bool FacebookTestUtils::CreateFacebookTestUser(
    http::Network& network, const olp::http::NetworkSettings& network_settings,
    FacebookUser& user, const std::string& permissions) {
  return impl_->CreateFacebookTestUser(network, network_settings, user,
                                       permissions);
}

bool FacebookTestUtils::DeleteFacebookTestUser(
    olp::http::Network& network,
    const olp::http::NetworkSettings& network_settings, std::string user_id) {
  return impl_->DeleteFacebookTestUser(network, network_settings, user_id);
}

}  // namespace authentication
}  // namespace olp
