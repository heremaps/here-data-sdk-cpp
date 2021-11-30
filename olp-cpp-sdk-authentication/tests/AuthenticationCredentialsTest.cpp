/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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
#include "olp/authentication/AuthenticationCredentials.h"

namespace auth = olp::authentication;

namespace {
constexpr auto valid_file_path = "existing.file";
constexpr auto invalid_file_path = "nonexisting.file";
constexpr auto valid_credentials =
    "here.user.id = HERE-111\n"
    "here.client.id = 123\n"
    "here.access.key.id = 234\n"
    "here.access.key.secret = 345\n"
    "here.token.endpoint.url = https://account.api.here.com/oauth2/token";
constexpr auto invalid_credentials =
    "here.user.id = HERE-111\n"
    "here.client.id = 222\n"
    "here.access.key.id = 333\n"
    "_here.access.key.secret = 4444\n"
    "here.token.endpoint.url = https://account.api.here.com/oauth2/token";
}  // namespace

TEST(AuthenticationCredentialsTest, ReadFromStream) {
  {
    SCOPED_TRACE("Credentials parse succeeds");

    std::stringstream ss(valid_credentials);
    const auto credentials =
        auth::AuthenticationCredentials::ReadFromStream(ss);

    EXPECT_TRUE(credentials);

    EXPECT_EQ(credentials->GetKey(), "234");
    EXPECT_EQ(credentials->GetSecret(), "345");
  }
  {
    SCOPED_TRACE("Bad content in the stream");

    std::stringstream ss(invalid_credentials);
    const auto credentials =
        auth::AuthenticationCredentials::ReadFromStream(ss);

    EXPECT_FALSE(credentials);
  }
}

TEST(AuthenticationCredentialsTest, ReadFromFile) {
  {
    SCOPED_TRACE("Credentials file successfuly parsed");

    // Create a file with a valid structure.
    std::ofstream stream(valid_file_path);
    stream << valid_credentials;
    stream.close();

    const auto credentials =
        auth::AuthenticationCredentials::ReadFromFile(valid_file_path);

    EXPECT_TRUE(credentials);
    EXPECT_NE(credentials.value().GetEndpointUrl(), "");

    // Remove the file.
    remove(valid_file_path);
  }
  {
    SCOPED_TRACE("Missing credential file");

    const auto credentials =
        auth::AuthenticationCredentials::ReadFromFile(invalid_file_path);

    EXPECT_FALSE(credentials);
  }
}


TEST(AuthenticationCredentialsTest, CanBeCopied) {
  auth::AuthenticationCredentials credentials("test_key", "test_secret");
  auth::AuthenticationCredentials copy = credentials;
  EXPECT_EQ(credentials.GetKey(), copy.GetKey());
  EXPECT_EQ(credentials.GetSecret(), copy.GetSecret());
}

