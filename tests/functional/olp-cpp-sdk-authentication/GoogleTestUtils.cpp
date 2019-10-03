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

#include "GoogleTestUtils.h"

#ifndef WIN32
#include <unistd.h>
#endif

#include <future>
#include <iostream>
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

namespace olp {
namespace authentication {
constexpr auto kGoogleApiUrl = "https://www.googleapis.com/";
constexpr auto kGoogleOauth2Endpoint = "oauth2/v3/token";
constexpr auto kGoogleClientIdParam = "client_id";
constexpr auto kGoogleClientSecretParam = "client_secret";
constexpr auto kGoogleRefreshTokenParam = "refresh_token";
constexpr auto kGoogleRefreshTokenGrantType = "grant_type=refresh_token";

class GoogleTestUtils::Impl {
 public:
  Impl();

  virtual ~Impl();

  bool GetAccessToken(olp::http::Network& network,
                      const olp::http::NetworkSettings& network_settings,
                      GoogleTestUtils::GoogleUser& user);
};

GoogleTestUtils::Impl::Impl() = default;

GoogleTestUtils::Impl::~Impl() = default;

bool GoogleTestUtils::Impl::GetAccessToken(
    http::Network& network, const olp::http::NetworkSettings& network_settings,
    GoogleTestUtils::GoogleUser& user) {
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
                            document->HasMember(kAccessToken.c_str());

            if (is_valid) {
              user.access_token = (*document)[kAccessToken.c_str()].GetString();
            }
          } else {
            std::cout << "GetAccessToken: status=" << user.status
                      << ", error=" << network_response.GetError() << std::endl;
          }
          promise.set_value();
        });
    future.wait();
  } while ((user.status < 0) && (++retry < kMaxRetryCount));

  return !user.access_token.empty();
}

GoogleTestUtils::GoogleTestUtils()
    : d_(std::make_unique<GoogleTestUtils::Impl>()) {}

GoogleTestUtils::~GoogleTestUtils() = default;

bool GoogleTestUtils::GetAccessToken(
    olp::http::Network& network,
    const olp::http::NetworkSettings& network_settings,
    GoogleTestUtils::GoogleUser& user) {
  return d_->GetAccessToken(network, network_settings, user);
}

}  // namespace authentication
}  // namespace olp
