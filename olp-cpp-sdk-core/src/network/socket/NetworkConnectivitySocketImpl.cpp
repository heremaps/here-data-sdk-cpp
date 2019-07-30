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

#include <string.h>
#include <cerrno>
#include <memory>
#include <vector>

#ifndef _WIN32
#include <arpa/inet.h>
#include <resolv.h>
#include <unistd.h>
#else
#include <winsock2.h>
#endif

#include "olp/core/logging/Log.h"
#include "olp/core/network/NetworkConnectivity.h"

namespace olp {
namespace network {
#define LOG_TAG "olp::network::NetworkConnectivity"

bool canConnectToAddress(const sockaddr_in& dsockaddr) {
#ifndef _WIN32
  int socket_desc = socket(dsockaddr.sin_family, SOCK_STREAM, 0);
  if (socket_desc == -1) {
    EDGE_SDK_LOG_WARNING(LOG_TAG, "socket() failed for ip address "
                                      << inet_ntoa(dsockaddr.sin_addr) << ": "
                                      << strerror(errno));
    return false;
  }

  if (connect(socket_desc, (struct sockaddr*)&dsockaddr, sizeof(dsockaddr)) ==
      -1) {
    EDGE_SDK_LOG_WARNING(LOG_TAG, "connect() failed for ip address "
                                      << inet_ntoa(dsockaddr.sin_addr) << ": "
                                      << strerror(errno));
    close(socket_desc);
    return false;
  }

  return close(socket_desc) != -1;
#else
  SOCKET socket_desc = INVALID_SOCKET;
  socket_desc = socket(dsockaddr.sin_family, SOCK_STREAM, IPPROTO_TCP);
  if (socket_desc == INVALID_SOCKET) {
    EDGE_SDK_LOG_WARNING(LOG_TAG, "socket() failed for ip address "
                                      << inet_ntoa(dsockaddr.sin_addr) << ": "
                                      << strerror(errno));
    return false;
  }

  if (connect(socket_desc, (struct sockaddr*)&dsockaddr, sizeof(dsockaddr)) ==
      SOCKET_ERROR) {
    EDGE_SDK_LOG_WARNING(LOG_TAG, "connect() failed for ip address "
                                      << inet_ntoa(dsockaddr.sin_addr) << ": "
                                      << strerror(errno));
    closesocket(socket_desc);
    return false;
  }

  return closesocket(socket_desc) != SOCKET_ERROR;
#endif
}

template <int domain = AF_INET>
bool canConnectToAddress(const char* ip_address, short port) {
  sockaddr_in server = {0};
  server.sin_addr.s_addr = inet_addr(ip_address);
  server.sin_family = domain;
  server.sin_port = port;

  return canConnectToAddress(server);
}

template <int domain = AF_INET>
bool canConnectToPublicDNS() {
  // list of common public DNS providers (primary and secondary adresses)
  // If none are reachable then we likely don't have any network connectivity.
  const static std::vector<const char*> publicDNSList = {
      "209.244.0.3",  "209.244.0.4",      // Level3
      "64.6.64.6",    "64.6.65.6",        // Verisign
      "8.8.8.8",      "8.8.4.4",          // Google
      "9.9.9.9",      "149.112.112.112",  // Quad9
      "84.200.69.80", "84.200.70.40"      // DNS.WATCH
  };

  for (auto dnsAddress : publicDNSList) {
    if (canConnectToAddress<domain>(dnsAddress, htons(53))) {
      return true;
    }
  }
  return false;
}

template <typename T>
class PointerTraits;

template <typename T>
class PointerTraits<T*> {
 public:
  using value_type = T;
};

bool canConnectToConfiguredDNS() {
#if !defined(ANDROID) && !defined(_WIN32)
  // Android implementation of the resolv library doesn't expose the configured
  // DNS server listing, will have to fall back to public DNS in this case
  // unless a better Android specific method can be found.
  using res_type = PointerTraits<res_state>::value_type;
  std::unique_ptr<res_type> res(new res_type());
  if (res_ninit(res.get()) != -1) {
    for (auto i = 0; i < res->nscount; ++i) {
      if (canConnectToAddress(res->nsaddr_list[i])) {
        return true;
      }
    }
  }
#endif
  return false;
}

template <int domain = AF_INET>
bool canConnectToDNS() {
  return canConnectToConfiguredDNS() || canConnectToPublicDNS<domain>();
}

bool NetworkConnectivity::IsNetworkConnected() { return canConnectToDNS(); }

}  // namespace network
}  // namespace olp
