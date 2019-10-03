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

#ifndef WIN32
#include <unistd.h>
#endif

#include <future>
#include <sstream>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <olp/core/http/HttpStatusCode.h>
#include <olp/core/http/Network.h>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>
#include <testutils/CustomParameters.hpp>
#include "TestConstants.h"

namespace {
constexpr auto kInstalledStatus = "true";

constexpr auto kTestUserPath = "/accounts/test-users";
constexpr auto kFacebookUrl = "https://graph.facebook.com/v2.12";

constexpr auto kInstalled = "installed";
constexpr auto kName = "name";
constexpr auto kPermissions = "permissions";
constexpr auto kId = "id";

const std::string kAccessToken = "access_token";
}  // namespace

namespace olp {
namespace authentication {
class FacebookTestUtils::Impl {
 public:
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
  url.append(kQuestionParam);
  url.append(kAccessToken + kEqualsParam +
             CustomParameters::getArgument("facebook_access_token"));
  url.append(kAndParam);
  url.append(kInstalled + kEqualsParam + kInstalledStatus);
  url.append(kAndParam);
  url.append(kName + kEqualsParam + kTestUserName);
  if (!permissions.empty()) {
    url.append(kAndParam);
    url.append(kPermissions + kEqualsParam + permissions);
  }
  olp::http::NetworkRequest request(url);
  request.WithVerb(http::NetworkRequest::HttpVerb::POST);
  request.WithSettings(network_settings);

  unsigned int retry = 0u;
  do {
    if (retry > 0u) {
      OLP_SDK_LOG_WARNING(__func__, "Request retry attempted (" << retry
                                                                << ")");
      std::this_thread::sleep_for(
          std::chrono::seconds(retry * kRetryDelayInSecs));
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
                            document->HasMember(kAccessToken.c_str()) &&
                            document->HasMember(kId);

            if (is_valid) {
              user.access_token = (*document)[kAccessToken.c_str()].GetString();
              user.id = (*document)[kId].GetString();
            }
          }
          promise.set_value();
        });
    future.wait();
  } while ((user.status < 0) && (++retry < kMaxRetryCount));

  return !user.id.empty() && !user.access_token.empty();
}

bool FacebookTestUtils::Impl::DeleteFacebookTestUser(
    http::Network& network, const olp::http::NetworkSettings& network_settings,
    const std::string& user_id) {
  std::string url = std::string() + kFacebookUrl + "/" + user_id;
  url.append(kQuestionParam);
  url.append(kAccessToken + kEqualsParam +
             CustomParameters::getArgument("facebook_access_token"));
  olp::http::NetworkRequest request(url);
  request.WithVerb(http::NetworkRequest::HttpVerb::DEL);
  request.WithSettings(network_settings);

  int status = 0;
  unsigned int retry = 0u;
  do {
    if (retry > 0u) {
      OLP_SDK_LOG_WARNING(__func__, "Request retry attempted (" << retry
                                                                << ")");
      std::this_thread::sleep_for(
          std::chrono::seconds(retry * kRetryDelayInSecs));
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
  } while ((status < 0) && (++retry < kMaxRetryCount));

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
