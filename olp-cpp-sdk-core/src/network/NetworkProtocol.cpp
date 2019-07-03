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

#include "olp/core/network/NetworkProtocol.h"

#ifdef NETWORK_HAS_CURL
#include "curl/NetworkProtocolCurl.h"
#endif

#ifdef NETWORK_HAS_ANDROID
#include "android/NetworkProtocolAndroid.h"
#endif

#ifdef NETWORK_HAS_IOS
#include "network/ios/NetworkProtocolIos.h"
#endif

#ifdef NETWORK_HAS_WINHTTP
#include "winhttp/NetworkProtocolWinHttp.h"
#endif

#include <memory>

#include "olp/core/network/NetworkRequestPriorityQueueDecorator.h"
#include "olp/core/network/NetworkResponse.h"

namespace olp {
namespace network {
namespace {

std::shared_ptr<NetworkProtocol> GetProtocol() {
#ifdef NETWORK_HAS_CURL
  return std::make_shared<NetworkProtocolCurl>();
#endif
#ifdef NETWORK_HAS_ANDROID
  return std::make_shared<NetworkProtocolAndroid>();
#endif
#ifdef NETWORK_HAS_IOS
  return std::make_shared<NetworkProtocolIos>();
#endif
#ifdef NETWORK_HAS_WINHTTP
  return std::make_shared<NetworkProtocolWinHttp>();
#endif
  return nullptr;
}

} // end of anonymous namespace

std::string NetworkProtocol::HttpErrorToString(int error) {
  switch (error) {
    case 100:
      return "Continue";
    case 101:
      return "Switching Protocols";
    case 200:
      return "OK";
    case 201:
      return "Created";
    case 202:
      return "Accepted";
    case 203:
      return "Non-Authoritative Information";
    case 204:
      return "No Content";
    case 205:
      return "Reset Content";
    case 206:
      return "Partial Content";
    case 300:
      return "Multiple Choices";
    case 301:
      return "Moved Permanently";
    case 302:
      return "Found";
    case 303:
      return "See Other";
    case 304:
      return "Not Modified";
    case 305:
      return "Use Proxy";
    case 307:
      return "Temporary Redirect";
    case 400:
      return "Bad Request";
    case 401:
      return "Unauthorized";
    case 402:
      return "Payment Required";
    case 403:
      return "Forbidden";
    case 404:
      return "Not Found";
    case 405:
      return "Method Not Allowed";
    case 406:
      return "Not Acceptable";
    case 407:
      return "Proxy Authentication Required";
    case 408:
      return "Request Timeout";
    case 409:
      return "Conflict";
    case 410:
      return "Gone";
    case 411:
      return "Length Required";
    case 412:
      return "Precondition Failed";
    case 413:
      return "Request Entity Too Large";
    case 414:
      return "Request-URI Too Long";
    case 415:
      return "Unsupported Media Type";
    case 416:
      return "Requested Range Not Satisfiable";
    case 417:
      return "Expectation Failed";
    case 500:
      return "Internal Server Error";
    case 501:
      return "Not Implementation";
    case 502:
      return "Bad gateway";
    case 503:
      return "Service Unavailable";
    case 504:
      return "Gateway Timeout";
    case 505:
      return "HTTP Version Not Supported";
  }
  return "Unknown Error";
}

void HandleSynchronousNetworkErrors(NetworkProtocol::ErrorCode error_code,
                                    Network::RequestId request_id,
                                    const Network::Callback& callback) {
  if (error_code == NetworkProtocol::ErrorInvalidRequest)
    callback(NetworkResponse(request_id, Network::ErrorCode::InvalidURLError,
                             "Invalid Request"));
  else if (error_code == NetworkProtocol::ErrorNoConnection)
    callback(NetworkResponse(request_id, Network::Offline, "Offline"));
  else if (error_code == NetworkProtocol::ErrorIO)
    callback(NetworkResponse(request_id, Network::IOError, "I/O Error"));
  else
    callback(
        NetworkResponse(request_id, Network::UnknownError, "Unknown error"));
}

std::shared_ptr<NetworkProtocol> DefaultNetworkProtocolFactory::Create(
    void*) const {
  auto protocol = GetProtocol();
  if (protocol) {
    return std::make_shared<NetworkRequestPriorityQueueDecorator>(protocol);
  }

  return nullptr;
}

}  // namespace network
}  // namespace olp
