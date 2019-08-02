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
#include <olp/core/logging/Log.h>
#include <olp/core/network/Network.h>
#include <olp/core/network/NetworkRequest.h>
#include <olp/core/network/NetworkResponse.h>
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

using namespace std;
using namespace rapidjson;
using namespace olp::network;

namespace {
constexpr auto ARCGIS_URL = "https://www.arcgis.com/sharing/rest/oauth2/token";
constexpr auto GRANT_TYPE = "grant_type";
constexpr auto CLIENT_ID = "client_id";
constexpr auto REFRESH_TOKEN = "refresh_token";
}  // namespace

namespace olp {
namespace authentication {
class ArcGisTestUtils::Impl {
 public:
  class ScopedNetwork {
   public:
    ScopedNetwork() : network() { network.Start(); }

    Network& getNetwork() { return network; }

   private:
    Network network;
  };

  using ScopedNetworkPtr = std::shared_ptr<ScopedNetwork>;
  /**
   * @brief Constructor
   * @param token_cache_limit Maximum number of tokens that will be cached.
   */
  Impl() = default;

  virtual ~Impl() = default;

  bool getAccessToken(ArcGisUser& user);

 private:
  ScopedNetworkPtr getScopedNetwork();
  std::shared_ptr<std::vector<unsigned char> > generateClientBody();

 private:
  std::weak_ptr<ScopedNetwork> networkPtr;
  std::mutex networkPtrLock;
};

ArcGisTestUtils::Impl::ScopedNetworkPtr
ArcGisTestUtils::Impl::getScopedNetwork() {
  lock_guard<mutex> lock(networkPtrLock);
  auto netPtr = networkPtr.lock();
  if (netPtr) return netPtr;

  auto result = make_shared<ScopedNetwork>();
  networkPtr = result;
  return result;
}

bool ArcGisTestUtils::Impl::getAccessToken(ArcGisUser& user) {
  NetworkRequest request(ARCGIS_URL, 0, ::NetworkRequest::PriorityDefault,
                         ::NetworkRequest::HttpVerb::POST);

  request.SetContent(generateClientBody());
  request.AddHeader("content-type", "application/x-www-form-urlencoded");

  auto networkPtr = getScopedNetwork();

  unsigned int retry = 0u;
  do {
    if (retry > 0u) {
      EDGE_SDK_LOG_WARNING(__func__,
                           "Request retry attempted (" << retry << ")");
      std::this_thread::sleep_for(
          std::chrono::seconds(retry * RETRY_DELAY_SECS));
    }

    shared_ptr<stringstream> payload = make_shared<stringstream>();

    std::promise<void> promise;
    auto future = promise.get_future();
    networkPtr->getNetwork().Send(
        request, payload,
        [networkPtr, payload, &promise,
         &user](const ::NetworkResponse& network_response) {
          user.status = network_response.Status();
          if (user.status == 200) {
            shared_ptr<Document> document = make_shared<Document>();
            IStreamWrapper stream(*payload.get());
            document->ParseStream(stream);
            bool is_valid = !document->HasParseError() &&
                            document->HasMember(ACCESS_TOKEN.c_str());
            if (is_valid) {
              user.access_token = (*document)[ACCESS_TOKEN.c_str()].GetString();
            }
          }
          promise.set_value();
        });
    future.wait();
  } while ((user.status < 0) && (++retry < MAX_RETRY_COUNT));

  return !user.access_token.empty();
}

std::shared_ptr<std::vector<unsigned char> >
ArcGisTestUtils::Impl::generateClientBody() {
  std::string data;
  data.append(CLIENT_ID)
      .append(EQUALS_PARAM)
      .append(CustomParameters::getArgument("arcgis_app_id"))
      .append(AND_PARAM);
  data.append(GRANT_TYPE)
      .append(EQUALS_PARAM)
      .append(REFRESH_TOKEN)
      .append(AND_PARAM);
  data.append(REFRESH_TOKEN)
      .append(EQUALS_PARAM)
      .append(CustomParameters::getArgument("arcgis_access_token"));
  return std::make_shared<std::vector<unsigned char> >(data.begin(),
                                                       data.end());
}

ArcGisTestUtils::ArcGisTestUtils()
    : d(std::make_unique<ArcGisTestUtils::Impl>()) {}

ArcGisTestUtils::~ArcGisTestUtils() = default;

bool ArcGisTestUtils::getAccessToken(ArcGisUser& user) {
  return d->getAccessToken(user);
}

}  // namespace authentication
}  // namespace olp
