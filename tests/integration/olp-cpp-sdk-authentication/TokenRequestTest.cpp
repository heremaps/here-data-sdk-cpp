/*
 * Copyright (C) 2021 HERE Europe B.V.
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

#include <chrono>
#include <memory>
#include <string>

#include <gtest/gtest.h>
#include "TokenRequest.h"

namespace auth = olp::authentication;

class TokenRequestTest : public ::testing::Test {
 public:
  auth::TokenRequest request;
};

TEST_F(TokenRequestTest, CheckExpireIn) {
  EXPECT_EQ(request.GetExpiresIn().count(), 0);

  request = request.WithExpiresIn(std::chrono::seconds(42));
  EXPECT_EQ(request.GetExpiresIn().count(), 42);
}

TEST_F(TokenRequestTest, CheckBody) {
  EXPECT_EQ(request.GetBody(), auth::RequestBodyType());

  auto requestBody = std::make_shared<std::vector<std::uint8_t>>(42);
  request = request.WithBody(requestBody);
  EXPECT_EQ(*request.GetBody(),
            *std::make_shared<std::vector<std::uint8_t>>(42));
}

TEST_F(TokenRequestTest, CheckAuthenticationCredentials) {
  auto defaultCredentials = auth::AuthenticationCredentials("", "");
  EXPECT_EQ(request.GetCredentials().GetKey(), defaultCredentials.GetKey());
  EXPECT_EQ(request.GetCredentials().GetSecret(),
            defaultCredentials.GetSecret());

  auto authenticationCredentials =
      auth::AuthenticationCredentials("key", "secret");
  request = request.WithCredentials(authenticationCredentials);
  EXPECT_EQ(request.GetCredentials().GetKey(),
            authenticationCredentials.GetKey());
  EXPECT_EQ(request.GetCredentials().GetSecret(),
            authenticationCredentials.GetSecret());
}

TEST_F(TokenRequestTest, CheckBuilder) {
  request =
      request.WithExpiresIn(std::chrono::seconds(42))
          .WithBody(std::make_shared<std::vector<std::uint8_t>>(42))
          .WithCredentials(auth::AuthenticationCredentials("key", "secret"));

  EXPECT_EQ(request.GetExpiresIn().count(), 42);
  EXPECT_EQ(*request.GetBody(),
            *std::make_shared<std::vector<std::uint8_t>>(42));
  EXPECT_EQ(request.GetCredentials().GetKey(), "key");
  EXPECT_EQ(request.GetCredentials().GetSecret(), "secret");
}
