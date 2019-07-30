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
#include <iostream>
#include <sstream>
#include "CommonTestUtils.h"
#include "testutils/CustomParameters.hpp"

using namespace std;
using namespace rapidjson;
using namespace olp::network;

namespace olp {
namespace authentication {
const static string GOOGLE_API_URL = "https://www.googleapis.com/";
const static string GOOGLE_OAUTH2_ENDPOINT = "oauth2/v3/token";
const static string GOOGLE_CLIENT_ID_PARAM = "client_id";
const static string GOOGLE_CLIENT_SECRET_PARAM = "client_secret";
const static string GOOGLE_REFRESH_TOKEN_PARAM = "refresh_token";
const static string GOOGLE_REFRESH_TOKEN_GRANT_TYPE =
    "grant_type=refresh_token";

class GoogleTestUtils::Impl {
 public:
  class ScopedNetwork {
   public:
    ScopedNetwork() : network() { network.Start(); }

    Network& getNetwork() { return network; }

   private:
    Network network;
  };

  using ScopedNetworkPtr = std::shared_ptr<ScopedNetwork>;
  Impl();

  virtual ~Impl();

  bool getAccessToken(GoogleTestUtils::GoogleUser& user);

 private:
  ScopedNetworkPtr getScopedNetwork();

 private:
  std::weak_ptr<ScopedNetwork> networkPtr;
  std::mutex networkPtrLock;
};

GoogleTestUtils::Impl::Impl() = default;

GoogleTestUtils::Impl::~Impl() = default;

GoogleTestUtils::Impl::ScopedNetworkPtr
GoogleTestUtils::Impl::getScopedNetwork() {
  lock_guard<mutex> lock(networkPtrLock);
  auto netPtr = networkPtr.lock();
  if (netPtr) return netPtr;

  auto result = make_shared<ScopedNetwork>();
  networkPtr = result;
  return result;
}

bool GoogleTestUtils::Impl::getAccessToken(GoogleTestUtils::GoogleUser& user) {
  string url = GOOGLE_API_URL + GOOGLE_OAUTH2_ENDPOINT;
  url.append(QUESTION_PARAM);
  url.append(GOOGLE_CLIENT_ID_PARAM + EQUALS_PARAM +
             CustomParameters::getArgument("google_client_id"));
  url.append(AND_PARAM);
  url.append(GOOGLE_CLIENT_SECRET_PARAM + EQUALS_PARAM +
             CustomParameters::getArgument("google_client_secret"));
  url.append(AND_PARAM);
  url.append(GOOGLE_REFRESH_TOKEN_PARAM + EQUALS_PARAM +
             CustomParameters::getArgument("google_client_token"));
  url.append(AND_PARAM);
  url.append(GOOGLE_REFRESH_TOKEN_GRANT_TYPE);

  NetworkRequest request(url, 0, NetworkRequest::PriorityDefault,
                         NetworkRequest::HttpVerb::POST);

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
         &user](const NetworkResponse& network_response) {
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
          } else {
            std::cout << "getAccessToken: status=" << user.status
                      << ", error=" << network_response.Error() << std::endl;
          }
          promise.set_value();
        });
    future.wait();
  } while ((user.status < 0) && (++retry < MAX_RETRY_COUNT));

  return !user.access_token.empty();
}

GoogleTestUtils::GoogleTestUtils()
    : d(std::make_unique<GoogleTestUtils::Impl>()) {}

GoogleTestUtils::~GoogleTestUtils() = default;

bool GoogleTestUtils::getAccessToken(GoogleTestUtils::GoogleUser& user) {
  return d->getAccessToken(user);
}

}  // namespace authentication
}  // namespace olp
