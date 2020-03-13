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

#include <gtest/gtest.h>
#include <olp/core/http/NetworkUtils.h>
#include <limits>

namespace {

using namespace olp::http;

TEST(NetworkUtilsTest, CaseInsensitiveCompare) {
  EXPECT_TRUE(NetworkUtils::CaseInsensitiveCompare("", ""));
  EXPECT_TRUE(NetworkUtils::CaseInsensitiveCompare("somestr", "someStr"));
  EXPECT_TRUE(NetworkUtils::CaseInsensitiveCompare("someStr_%wIthN@mb3r5",
                                                   "someStr_%wIthN@mb3r5"));
  EXPECT_FALSE(NetworkUtils::CaseInsensitiveCompare("someStr_sizeDifferent",
                                                    "somestr_sizeDiff"));
  EXPECT_TRUE(NetworkUtils::CaseInsensitiveCompare("1someStr_OffSet",
                                                   "someStr_OffSet", 1));
  EXPECT_FALSE(NetworkUtils::CaseInsensitiveCompare("1WrongStr_OffSet",
                                                    "WrongStr_OffSet", 2));
  EXPECT_TRUE(NetworkUtils::CaseInsensitiveCompare("StringOffsetMaX", "x", 14));
  EXPECT_FALSE(NetworkUtils::CaseInsensitiveCompare(
      "StringOffsetBig", "x", std::numeric_limits<size_t>::max()));
}

TEST(NetworkUtilsTest, CaseInsensitiveStartsWith) {
  EXPECT_TRUE(NetworkUtils::CaseInsensitiveStartsWith("", ""));
  EXPECT_TRUE(NetworkUtils::CaseInsensitiveStartsWith("somestrEqualStrings",
                                                      "someStrequalstrings"));
  EXPECT_TRUE(NetworkUtils::CaseInsensitiveStartsWith(
      "someStr_%wIthN@mb3r5_12345abcd", "someStr_%wIthN@mb3r5"));
  EXPECT_FALSE(NetworkUtils::CaseInsensitiveStartsWith("someStr_sizeLes",
                                                       "someStr_sizeless"));
  EXPECT_TRUE(NetworkUtils::CaseInsensitiveStartsWith("1someStr_OffSet_abcd",
                                                      "someStr_OffSet", 1));
  EXPECT_FALSE(NetworkUtils::CaseInsensitiveStartsWith("1WrongStr_OffSet_abcd",
                                                       "WrongStr_OffSet", 2));
  EXPECT_TRUE(
      NetworkUtils::CaseInsensitiveStartsWith("StringOffsetMaX", "x", 14));
  EXPECT_FALSE(NetworkUtils::CaseInsensitiveStartsWith(
      "StringOffsetBig", "String", std::numeric_limits<size_t>::max()));
}

TEST(NetworkUtilsTest, CaseInsensitiveFind) {
  EXPECT_EQ(0, NetworkUtils::CaseInsensitiveFind("aaaaaaaaaaaaa", "AA", 0));
  EXPECT_EQ(0, NetworkUtils::CaseInsensitiveFind("somestr", "somestr"));
  EXPECT_EQ(1, NetworkUtils::CaseInsensitiveFind("_somestr", "somestr"));
  EXPECT_EQ(7,
            NetworkUtils::CaseInsensitiveFind("someStrsomeStr", "somestr", 2));
  EXPECT_EQ(7,
            NetworkUtils::CaseInsensitiveFind("someStrsomeStr1", "somestr", 2));
  EXPECT_EQ(std::string::npos, NetworkUtils::CaseInsensitiveFind(
                                   "someStrsomeStr1111", "somestr2", 2));
  EXPECT_EQ(0, NetworkUtils::CaseInsensitiveFind("SomeStr", "somestR", 0));
  EXPECT_EQ(std::string::npos,
            NetworkUtils::CaseInsensitiveFind("SomeStr", "somestRing", 0));
  EXPECT_EQ(std::string::npos,
            NetworkUtils::CaseInsensitiveFind("SomeStr", ""));
  EXPECT_EQ(std::string::npos,
            NetworkUtils::CaseInsensitiveFind("", "SomeStr"));
  EXPECT_EQ(std::string::npos, NetworkUtils::CaseInsensitiveFind("", ""));
}

TEST(NetworkUtilsTest, ExtractUserAgentTest) {
  {
    SCOPED_TRACE("User agent is present and extracted");
    Headers headers;
    headers.emplace_back(Header("user-Agent", "agent smith"));
    headers.emplace_back(Header("other-header", "header"));
    std::string user_agent = NetworkUtils::ExtractUserAgent(headers);
    EXPECT_EQ(user_agent, "agent smith");
    EXPECT_EQ(headers.size(), 1);
    EXPECT_EQ(headers[0].first, "other-header");
    EXPECT_EQ(headers[0].second, "header");
  }
  {
    SCOPED_TRACE("User agent is missing and nothing happens");
    Headers headers;
    headers.emplace_back(Header("other-header", "header"));
    std::string user_agent = NetworkUtils::ExtractUserAgent(headers);
    EXPECT_EQ(user_agent, "");
    EXPECT_EQ(headers.size(), 1);
    EXPECT_EQ(headers[0].first, "other-header");
    EXPECT_EQ(headers[0].second, "header");
  }
}

TEST(NetworkUtilsTest, HttpErrorToString) {
  EXPECT_EQ("Unknown Error", HttpErrorToString(1));
  EXPECT_EQ("Continue", HttpErrorToString(100));
  EXPECT_EQ("Switching Protocols", HttpErrorToString(101));
  EXPECT_EQ("OK", HttpErrorToString(200));
  EXPECT_EQ("Created", HttpErrorToString(201));
  EXPECT_EQ("Accepted", HttpErrorToString(202));
  EXPECT_EQ("Non-Authoritative Information", HttpErrorToString(203));
  EXPECT_EQ("No Content", HttpErrorToString(204));
  EXPECT_EQ("Reset Content", HttpErrorToString(205));
  EXPECT_EQ("Partial Content", HttpErrorToString(206));
  EXPECT_EQ("Multiple Choices", HttpErrorToString(300));
  EXPECT_EQ("Moved Permanently", HttpErrorToString(301));
  EXPECT_EQ("Found", HttpErrorToString(302));
  EXPECT_EQ("See Other", HttpErrorToString(303));
  EXPECT_EQ("Not Modified", HttpErrorToString(304));
  EXPECT_EQ("Use Proxy", HttpErrorToString(305));
  EXPECT_EQ("Temporary Redirect", HttpErrorToString(307));
  EXPECT_EQ("Bad Request", HttpErrorToString(400));
  EXPECT_EQ("Unauthorized", HttpErrorToString(401));
  EXPECT_EQ("Payment Required", HttpErrorToString(402));
  EXPECT_EQ("Forbidden", HttpErrorToString(403));
  EXPECT_EQ("Not Found", HttpErrorToString(404));
  EXPECT_EQ("Method Not Allowed", HttpErrorToString(405));
  EXPECT_EQ("Not Acceptable", HttpErrorToString(406));
  EXPECT_EQ("Proxy Authentication Required", HttpErrorToString(407));
  EXPECT_EQ("Request Timeout", HttpErrorToString(408));
  EXPECT_EQ("Conflict", HttpErrorToString(409));
  EXPECT_EQ("Gone", HttpErrorToString(410));
  EXPECT_EQ("Length Required", HttpErrorToString(411));
  EXPECT_EQ("Precondition Failed", HttpErrorToString(412));
  EXPECT_EQ("Request Entity Too Large", HttpErrorToString(413));
  EXPECT_EQ("Request-URI Too Long", HttpErrorToString(414));
  EXPECT_EQ("Unsupported Media Type", HttpErrorToString(415));
  EXPECT_EQ("Requested Range Not Satisfiable", HttpErrorToString(416));
  EXPECT_EQ("Expectation Failed", HttpErrorToString(417));
  EXPECT_EQ("Internal Server Error", HttpErrorToString(500));
  EXPECT_EQ("Not Implementation", HttpErrorToString(501));
  EXPECT_EQ("Bad gateway", HttpErrorToString(502));
  EXPECT_EQ("Service Unavailable", HttpErrorToString(503));
  EXPECT_EQ("Gateway Timeout", HttpErrorToString(504));
  EXPECT_EQ("HTTP Version Not Supported", HttpErrorToString(505));
}

}  // namespace
