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

#include <gtest/gtest.h>
#include <olp/core/http/HttpStatusCode.h>
#include "../src/SignInResultImpl.h"

namespace {
constexpr auto kClientId = "4GaUh8X9CpOax4CCC3of";
constexpr auto kTokenResponse =
    "{\"accessToken\":"
    "\"eyJhbGciOiJSUzUxMiIsImN0eSI6IkpXVCIsImlzcyI6IkhFUkUiLCJhaWQiOiI0R2FVaDhY"
    "OUNwT2F4NENDQzNvZiIsImlhdCI6MTU3ODMxMDExNCwiZXhwIjoxNTc4MzEzNzE0LCJraWQiOi"
    "JqMSJ9."
    "ZXlKaGJHY2lPaUprYVhJaUxDSmxibU1pT2lKQk1qVTJRMEpETFVoVE5URXlJbjAuLnFMb0Jmd0"
    "1rSVktRHhuRFA1Ym9kYlEubmdZZkJIUFBsNXVBazZvY0w5djVTa3R4QU4xalE4RENjTzBiYnJO"
    "Y2FuUkdKWFRvaU8tRXRuNGFNZE44OTBXaDR5QUlKaWI4LTUzY3lVMWVDZF9iX0ZCRmYxWEFsVU"
    "ZkU1lFdUtFWHpoVUVVVnB0ZE1QdlY2VnVEVWktbTJoYWQ3ZEd4YUtXdVNoVVJNVFpOa1NHZy13"
    "Lm9wdHJQUzRYQi1CNnBxdTZxa1Y4a09WQlNLbXFLUVhBODRMUV9IS0RZTms."
    "UGyfwSjA8g6gDfLY4sCEu4fkrgp8WNm-uq5F2KLJ-zcDJIwoPKNrJjd2FEZdllG8-"
    "tbbVWx5fwxn9qV6vLbm5BOMdvX3eMw_FQgsEPQAX3cOyv3he-"
    "3xIlw0WjEAEfI6hOZnx89FdkKGXuUp7cOPaM-b133mFsE-"
    "S5uuuQ3vYnztKcfmEwc8SSzgCiyRTuUYUE_ESZdpwQi1jx1rbag7fBFqIIGNSVN_"
    "d6Scpg2s6ybHftoXajMVydnvmG4Iyls8eOeCcLDYMUPcgy05Uqc5CMvGSsL8WUOJAoLWw3eF_"
    "ik1Xyv4iHRz3-67EAzzdwtL6nPbN8c0S16leVshnfdUZA\",\"tokenType\":\"bearer\","
    "\"expiresIn\":3599}";
constexpr auto kInvalidTokenResponse =
    "{\"accessToken\":"
    "\"eyJhbGciOiJSUzUxbx5sImN0eSI6IkpXVCIsImlzcyI6IkhFUkUiLCJhaWQiOiI0R2FVaDhY"
    "OUNwT2F4NENDQzNvZiIsImlhdCI6MTU3ODMxMDExNCwiZXhwIjoxNTc4MzEzNzE0LCJraWQiOi"
    "JqMSJ9."
    "ZXlKaGJHY2lPaUprYVhJaUxDSmxibU1pT2lKQk1qVTJRMEpETFVoVE5URXlJbjAuLnFMb0Jmd0"
    "1rSVktRHhuRFA1Ym9kYlEubmdZZkJIUFBsNXVBazZvY0w5djVTa3R4QU4xalE4RENjTzBiYnJO"
    "Y2FuUkdKWFRvaU8tRXRuNGFNZE44OTBXaDR5QUlKaWI4LTUzY3lVMWVDZF9iX0ZCRmYxWEFsVU"
    "ZkU1lFdUtFWHpoVUVVVnB0ZE1QdlY2VnVEVWktbTJoYWQ3ZEd4YUtXdVNoVVJNVFpOa1NHZy13"
    "Lm9wdHJQUzRYQi1CNnBxdTZxa1Y4a09WQlNLbXFLUVhBODRMUV9IS0RZTms."
    "UGyfwSjA8g6gDfLY4sCEu4fkrgp8WNm-uq5F2KLJ-zcDJIwoPKNrJjd2FEZdllG8-"
    "tbbVWx5fwxn9qV6vLbm5BOMdvX3eMw_FQgsEPQAX3cOyv3he-"
    "3xIlw0WjEAEfI6hOZnx89FdkKGXuUp7cOPaM-b133mFsE-"
    "S5uuuQ3vYnztKcfmEwc8SSzgCiyRTuUYUE_ESZdpwQi1jx1rbag7fBFqIIGNSVN_"
    "d6Scpg2s6ybHftoXajMVydnvmG4Iyls8eOeCcLDYMUPcgy05Uqc5CMvGSsL8WUOJAoLWw3eF_"
    "ik1Xyv4iHRz3-67EAzzdwtL6nPbN8c0S16leVshnfdUZA\",\"tokenType\":\"bearer\","
    "\"expiresIn\":3599}";
constexpr auto kNoIdFieldInToken =
    "{\"accessToken\":"
    "\"eyJhbGciOiJSUzUxMiIsImN0eSI6IkpXVCIsImlzcyI6IkhFUkUiLCJhaWQiOjMsImlhdCI6"
    "MTU3ODMxMDExNCwiZXhwIjoxNTc4MzEzNzE0LCJraWQiOiJqMSJ9."
    "ZXlKaGJHY2lPaUprYVhJaUxDSmxibU1pT2lKQk1qVTJRMEpETFVoVE5URXlJbjAuLnFMb0Jmd0"
    "1rSVktRHhuRFA1Ym9kYlEubmdZZkJIUFBsNXVBazZvY0w5djVTa3R4QU4xalE4RENjTzBiYnJO"
    "Y2FuUkdKWFRvaU8tRXRuNGFNZE44OTBXaDR5QUlKaWI4LTUzY3lVMWVDZF9iX0ZCRmYxWEFsVU"
    "ZkU1lFdUtFWHpoVUVVVnB0ZE1QdlY2VnVEVWktbTJoYWQ3ZEd4YUtXdVNoVVJNVFpOa1NHZy13"
    "Lm9wdHJQUzRYQi1CNnBxdTZxa1Y4a09WQlNLbXFLUVhBODRMUV9IS0RZTms."
    "UGyfwSjA8g6gDfLY4sCEu4fkrgp8WNm-uq5F2KLJ-zcDJIwoPKNrJjd2FEZdllG8-"
    "tbbVWx5fwxn9qV6vLbm5BOMdvX3eMw_FQgsEPQAX3cOyv3he-"
    "3xIlw0WjEAEfI6hOZnx89FdkKGXuUp7cOPaM-b133mFsE-"
    "S5uuuQ3vYnztKcfmEwc8SSzgCiyRTuUYUE_ESZdpwQi1jx1rbag7fBFqIIGNSVN_"
    "d6Scpg2s6ybHftoXajMVydnvmG4Iyls8eOeCcLDYMUPcgy05Uqc5CMvGSsL8WUOJAoLWw3eF_"
    "ik1Xyv4iHRz3-67EAzzdwtL6nPbN8c0S16leVshnfdUZA\",\"tokenType\":\"bearer\","
    "\"expiresIn\":3599}";
constexpr auto kWrongIdFieldInToken =
    "{\"accessToken\":"
    "\"eyJhbGciOiJSUzUxMiIsImN0eSI6IkpXVCIsImlzcyI6IkhFUkUiLCJhaWQiOjMsImlhdCI6"
    "MTU3ODMxMDExNCwiZXhwIjoxNTc4MzEzNzE0LCJraWQiOiJqMSJ9."
    "ZXlKaGJHY2lPaUprYVhJaUxDSmxibU1pT2lKQk1qVTJRMEpETFVoVE5URXlJbjAuLnFMb0Jmd0"
    "1rSVktRHhuRFA1Ym9kYlEubmdZZkJIUFBsNXVBazZvY0w5djVTa3R4QU4xalE4RENjTzBiYnJO"
    "Y2FuUkdKWFRvaU8tRXRuNGFNZE44OTBXaDR5QUlKaWI4LTUzY3lVMWVDZF9iX0ZCRmYxWEFsVU"
    "ZkU1lFdUtFWHpoVUVVVnB0ZE1QdlY2VnVEVWktbTJoYWQ3ZEd4YUtXdVNoVVJNVFpOa1NHZy13"
    "Lm9wdHJQUzRYQi1CNnBxdTZxa1Y4a09WQlNLbXFLUVhBODRMUV9IS0RZTms."
    "UGyfwSjA8g6gDfLY4sCEu4fkrgp8WNm-uq5F2KLJ-zcDJIwoPKNrJjd2FEZdllG8-"
    "tbbVWx5fwxn9qV6vLbm5BOMdvX3eMw_FQgsEPQAX3cOyv3he-"
    "3xIlw0WjEAEfI6hOZnx89FdkKGXuUp7cOPaM-b133mFsE-"
    "S5uuuQ3vYnztKcfmEwc8SSzgCiyRTuUYUE_ESZdpwQi1jx1rbag7fBFqIIGNSVN_"
    "d6Scpg2s6ybHftoXajMVydnvmG4Iyls8eOeCcLDYMUPcgy05Uqc5CMvGSsL8WUOJAoLWw3eF_"
    "ik1Xyv4iHRz3-67EAzzdwtL6nPbN8c0S16leVshnfdUZA\",\"tokenType\":\"bearer\","
    "\"expiresIn\":3599}";
constexpr auto kWrongFormatToken =
    "{\"accessToken\":"
    "\"eyJhbGciOiJSUzUxMiIsImN0eSI6IkpXVCIsImlzcyI6IkhFUkUiLCJhaWQiOiI0R2FVaDhY"
    "OUNwT2F4NENDQzNvZiIsImlhdCI6MTU3ODMxMDExNCwiZXhwIjoxNTc4MzEzNzE0LCJraWQiOi"
    "JqMSJ9\",\"tokenType\":\"bearer\",\"expiresIn\":3599}";

TEST(SignInResultImplTest, ValidToken) {
  auto doc = std::make_shared<rapidjson::Document>();
  doc->Parse(kTokenResponse);
  olp::authentication::SignInResultImpl result(olp::http::HttpStatusCode::OK,
                                               std::string(), doc);
  EXPECT_EQ(result.GetClientId(), kClientId);
}

TEST(SignInResultImplTest, WrongIdField) {
  auto doc = std::make_shared<rapidjson::Document>();
  doc->Parse(kWrongIdFieldInToken);
  olp::authentication::SignInResultImpl result(olp::http::HttpStatusCode::OK,
                                               std::string(), doc);
  EXPECT_TRUE(result.GetClientId().empty());
}

TEST(SignInResultImplTest, NoIdField) {
  auto doc = std::make_shared<rapidjson::Document>();
  doc->Parse(kNoIdFieldInToken);
  olp::authentication::SignInResultImpl result(olp::http::HttpStatusCode::OK,
                                               std::string(), doc);
  EXPECT_TRUE(result.GetClientId().empty());
}

TEST(SignInResultImplTest, NotSupportedFormat) {
  auto doc = std::make_shared<rapidjson::Document>();
  doc->Parse(kWrongFormatToken);
  olp::authentication::SignInResultImpl result(olp::http::HttpStatusCode::OK,
                                               std::string(), doc);
  EXPECT_TRUE(result.GetClientId().empty());
}

TEST(SignInResultImplTest, InvalidToken) {
  auto doc = std::make_shared<rapidjson::Document>();
  doc->Parse(kInvalidTokenResponse);
  olp::authentication::SignInResultImpl result(olp::http::HttpStatusCode::OK,
                                               std::string(), doc);
  EXPECT_TRUE(result.GetClientId().empty());
}
}  // namespace
