/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#include <cstdio>
#include <fstream>

#include <gtest/gtest.h>

#include "../src/AuthenticationClientUtils.h"

namespace {
constexpr auto kTime = "Fri, 29 May 2020 11:07:45 GMT";
}  // namespace

namespace auth = olp::authentication;

TEST(AuthenticationClientTest, TimeParsing) {
  {
    SCOPED_TRACE("Parse time");
    EXPECT_EQ(auth::ParseTime(kTime), 1590750465);
  }
}

TEST(AuthenticationClientTest, GenerateAuthorizationHeader) {
  auth::AuthenticationCredentials credentials("key", "secret");
  const auto url = "https://auth.server.com";
  auto sig = auth::GenerateAuthorizationHeader(credentials, url, 0, "unique");
  auto expected_sig =
      "oauth_consumer_key=key&oauth_nonce=unique&oauth_signature_method=HMAC-"
      "SHA256&oauth_timestamp=0&oauth_version=1.0POST&https%3A%2F%2Fauth."
      "server.com&oauth_consumer_key%3Dkey%26oauth_nonce%3Dunique%26oauth_"
      "signature_method%3DHMAC-SHA256%26oauth_timestamp%3D0%26oauth_version%"
      "3D1.0OAuth "
      "oauth_consumer_key=\"key\",oauth_nonce=\"unique\",oauth_signature_"
      "method=\"HMAC-SHA256\",oauth_timestamp=\"0\",oauth_version=\"1.0\","
      "oauth_signature=\"ncwRtcqRSM04FIFch8Ay4l7bRmp96lifuHEops4AqEw%3D\"";
  EXPECT_EQ(sig, expected_sig);
}
