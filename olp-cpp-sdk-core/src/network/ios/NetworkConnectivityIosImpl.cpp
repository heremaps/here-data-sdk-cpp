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
#include <olp/core/logging/Log.h>
#include <olp/core/network/NetworkConnectivity.h>

#include <CoreFoundation/CoreFoundation.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <netdb.h>

namespace olp {
namespace network {
// Apple implementation, uses SCNetworkReachabilityGetFlags library call to
// detect network connection.
bool NetworkConnectivity::IsNetworkConnected() {
  struct sockaddr_in zeroAddress;
  bzero(&zeroAddress, sizeof(zeroAddress));
  zeroAddress.sin_len = sizeof(zeroAddress);
  zeroAddress.sin_family = AF_INET;

  SCNetworkReachabilityRef reachabilityRef =
      SCNetworkReachabilityCreateWithAddress(
          kCFAllocatorDefault, (const struct sockaddr*)&zeroAddress);

  SCNetworkReachabilityFlags flags;
  if (SCNetworkReachabilityGetFlags(reachabilityRef, &flags)) {
    if ((flags & kSCNetworkReachabilityFlagsReachable) == 0) {
      // if target host is not reachable
      return false;
    }

    if ((flags & kSCNetworkReachabilityFlagsConnectionRequired) == 0) {
      // if target host is reachable and no connection is required
      //  then we'll assume (for now) that your on Wi-Fi
      return true;  // This is a wifi connection.
    }

    if ((((flags & kSCNetworkReachabilityFlagsConnectionOnDemand) != 0) ||
         (flags & kSCNetworkReachabilityFlagsConnectionOnTraffic) != 0)) {
      // ... and the connection is on-demand (or on-traffic) if the
      //     calling application is using the CFSocketStream or higher APIs

      if ((flags & kSCNetworkReachabilityFlagsInterventionRequired) == 0) {
        // ... and no [user] intervention is needed
        return true;  // This is a wifi connection.
      }
    }

#if TARGET_OS_IPHONE
    if ((flags & kSCNetworkReachabilityFlagsIsWWAN) ==
        kSCNetworkReachabilityFlagsIsWWAN) {
      // ... but WWAN connections are OK if the calling application
      //     is using the CFNetwork (CFSocketStream?) APIs.
      return true;
      // This is a cellular connection.
    }
#endif
  }

  return false;
}
}  // network
}  // namespace olp
