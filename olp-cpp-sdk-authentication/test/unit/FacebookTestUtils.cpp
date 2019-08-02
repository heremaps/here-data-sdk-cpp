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

const static string USER_PERMISIONS = "email";
const static string INSTALLED_STATUS = "true";

const static string TEST_USER_PATH = "/accounts/test-users";
const static string FACEBOOK_URL = "https://graph.facebook.com/v2.12";

const static string INSTALLED = "installed";
const static string NAME = "name";
const static string PERMISSIONS = "permissions";
const static string ID = "id";

namespace olp {
namespace authentication {
class FacebookTestUtils::Impl {
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

  bool createFacebookTestUser(FacebookUser& user, std::string permissions);
  bool deleteFacebookTestUser(std::string user_id);

 private:
  ScopedNetworkPtr getScopedNetwork();

 private:
  std::weak_ptr<ScopedNetwork> networkPtr;
  std::mutex networkPtrLock;
};

FacebookTestUtils::Impl::ScopedNetworkPtr
FacebookTestUtils::Impl::getScopedNetwork() {
  lock_guard<mutex> lock(networkPtrLock);
  auto netPtr = networkPtr.lock();
  if (netPtr) return netPtr;

  auto result = make_shared<ScopedNetwork>();
  networkPtr = result;
  return result;
}

bool FacebookTestUtils::Impl::createFacebookTestUser(FacebookUser& user,
                                                     string permissions) {
  string url = FACEBOOK_URL + "/" +
               CustomParameters::getArgument("facebook_app_id") +
               TEST_USER_PATH;
  ;
  url.append(QUESTION_PARAM);
  url.append(ACCESS_TOKEN + EQUALS_PARAM +
             CustomParameters::getArgument("facebook_access_token"));
  url.append(AND_PARAM);
  url.append(INSTALLED + EQUALS_PARAM + INSTALLED_STATUS);
  url.append(AND_PARAM);
  url.append(NAME + EQUALS_PARAM + TEST_USER_NAME);
  if (!permissions.empty()) {
    url.append(AND_PARAM);
    url.append(PERMISSIONS + EQUALS_PARAM + permissions);
  }
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
                            document->HasMember(ACCESS_TOKEN.c_str()) &&
                            document->HasMember(ID.c_str());

            if (is_valid) {
              user.access_token = (*document)[ACCESS_TOKEN.c_str()].GetString();
              user.id = (*document)[ID.c_str()].GetString();
            }
          }
          promise.set_value();
        });
    future.wait();
  } while ((user.status < 0) && (++retry < MAX_RETRY_COUNT));

  return !user.id.empty() && !user.access_token.empty();
}

bool FacebookTestUtils::Impl::deleteFacebookTestUser(string user_id) {
  string url = FACEBOOK_URL + "/" + user_id;
  url.append(QUESTION_PARAM);
  url.append(ACCESS_TOKEN + EQUALS_PARAM +
             CustomParameters::getArgument("facebook_access_token"));
  NetworkRequest request(url, 0, NetworkRequest::PriorityDefault,
                         NetworkRequest::HttpVerb::DEL);

  auto networkPtr = getScopedNetwork();

  int status = 0;
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
    networkPtr->getNetwork().Send(request, payload,
                                  [networkPtr, payload, &promise, &status](
                                      const NetworkResponse& network_response) {
                                    status = network_response.Status();
                                    promise.set_value();
                                  });
    future.wait();
  } while ((status < 0) && (++retry < MAX_RETRY_COUNT));

  return (status == 200);
}

FacebookTestUtils::FacebookTestUtils()
    : impl_(std::make_unique<FacebookTestUtils::Impl>()) {}

FacebookTestUtils::~FacebookTestUtils() = default;

bool FacebookTestUtils::createFacebookTestUser(FacebookUser& user,
                                               string permissions) {
  return impl_->createFacebookTestUser(user, permissions);
}

bool FacebookTestUtils::deleteFacebookTestUser(string user_id) {
  return impl_->deleteFacebookTestUser(user_id);
}

}  // namespace authentication
}  // namespace olp
