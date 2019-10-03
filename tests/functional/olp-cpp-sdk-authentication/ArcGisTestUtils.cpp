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
#include "ArcGisTestUtils.h"

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
constexpr auto kArcgisUrl = "https://www.arcgis.com/sharing/rest/oauth2/token";
constexpr auto kGrantType = "grant_type";
constexpr auto kClientId = "client_id";
constexpr auto kRefreshToken = "refresh_token";
}  // namespace

namespace olp {
namespace authentication {
class ArcGisTestUtils::Impl {
 public:
  Impl() = default;

  virtual ~Impl() = default;

  bool GetAccessToken(http::Network& network,
                      const olp::http::NetworkSettings& network_settings,
                      ArcGisUser& user);

 private:
  std::shared_ptr<std::vector<unsigned char> > GenerateClientBody();
};

bool ArcGisTestUtils::Impl::GetAccessToken(
    olp::http::Network& network,
    const olp::http::NetworkSettings& network_settings, ArcGisUser& user) {
  olp::http::NetworkRequest request(kArcgisUrl);
  request.WithVerb(http::NetworkRequest::HttpVerb::POST);
  request.WithSettings(network_settings);

  request.WithBody(GenerateClientBody());
  request.WithHeader("content-type", "application/x-www-form-urlencoded");
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
    network.Send(request, payload,
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
                       user.access_token =
                           (*document)[kAccessToken.c_str()].GetString();
                     }
                   }
                   promise.set_value();
                 });
    future.wait();
  } while ((user.status < 0) && (++retry < kMaxRetryCount));

  return !user.access_token.empty();
}

std::shared_ptr<std::vector<unsigned char> >
ArcGisTestUtils::Impl::GenerateClientBody() {
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
  return std::make_shared<std::vector<unsigned char> >(data.begin(),
                                                       data.end());
}

ArcGisTestUtils::ArcGisTestUtils()
    : d_(std::make_unique<ArcGisTestUtils::Impl>()) {}

ArcGisTestUtils::~ArcGisTestUtils() = default;

bool ArcGisTestUtils::GetAccessToken(
    olp::http::Network& network,
    const olp::http::NetworkSettings& network_settings, ArcGisUser& user) {
  return d_->GetAccessToken(network, network_settings, user);
}

}  // namespace authentication
}  // namespace olp
