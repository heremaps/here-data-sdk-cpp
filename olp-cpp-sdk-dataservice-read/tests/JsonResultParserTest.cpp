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

#include <string>

#include <gtest/gtest.h>
// clang-format off
// this order is required
#include "generated/model/Api.h"
#include <olp/dataservice/read/Types.h>
#include "generated/parser/ApiParser.h"
#include "generated/api/PlatformApi.h"
#include "generated/parser/PartitionsParser.h"
#include "generated/parser/VersionResponseParser.h"
#include "JsonResultParser.h"
#include "generated/serializer/ApiSerializer.h"
#include "generated/serializer/PartitionsSerializer.h"
#include "generated/serializer/VersionResponseSerializer.h"
#include "generated/serializer/JsonSerializer.h"
#include "ReadDefaultResponses.h"
#include "ApiDefaultResponses.h"
// clang-format on

namespace dr = olp::dataservice::read;

TEST(JsonResultParserTest, Api) {
  auto data = mockserver::ApiDefaultResponses::GeneratePlatformApisResponse();
  std::string api_string = "[";
  for (const auto &el : data) {
    api_string.append(olp::serializer::serialize(el));
    api_string.append(",");
  }
  api_string[api_string.length() - 1] = ']';
  {
    SCOPED_TRACE("Verify valid version responce");
    auto str = std::stringstream(api_string);

    auto responce =
        olp::parser::parse_result<dr::PlatformApi::ApisResponse>(str);
    ASSERT_TRUE(responce.IsSuccessful());
  }
  {
    SCOPED_TRACE("Verify corrupted with additional symbol");
    auto str = std::stringstream(api_string + "_");

    auto responce =
        olp::parser::parse_result<dr::PlatformApi::ApisResponse>(str);
    ASSERT_FALSE(responce.IsSuccessful());
    ASSERT_EQ(responce.GetError().GetErrorCode(),
              olp::client::ErrorCode::Unknown);
    ASSERT_EQ(responce.GetError().GetMessage(), "Fail parsing response.");
  }
  {
    SCOPED_TRACE("Verify corrupted symbol");
    api_string[0] = '-';
    auto str = std::stringstream(api_string);

    auto responce =
        olp::parser::parse_result<dr::PlatformApi::ApisResponse>(str);
    ASSERT_FALSE(responce.IsSuccessful());
    ASSERT_EQ(responce.GetError().GetErrorCode(),
              olp::client::ErrorCode::Unknown);
    ASSERT_EQ(responce.GetError().GetMessage(), "Fail parsing response.");
  }
}

TEST(JsonResultParserTest, Versions) {
  auto version = mockserver::ReadDefaultResponses::GenerateVersionResponse(44);
  auto version_string = olp::serializer::serialize(version);
  {
    SCOPED_TRACE("Verify valid version responce");
    auto str = std::stringstream(version_string);

    auto responce = olp::parser::parse_result<dr::CatalogVersionResponse>(str);
    ASSERT_TRUE(responce.IsSuccessful());
    ASSERT_EQ(44, responce.GetResult().GetVersion());
  }
  {
    SCOPED_TRACE("Verify corrupted with additional symbol");
    auto str = std::stringstream(version_string + "_");

    auto responce = olp::parser::parse_result<dr::CatalogVersionResponse>(str);
    ASSERT_FALSE(responce.IsSuccessful());
    ASSERT_EQ(responce.GetError().GetErrorCode(),
              olp::client::ErrorCode::Unknown);
    ASSERT_EQ(responce.GetError().GetMessage(), "Fail parsing response.");
  }
  {
    SCOPED_TRACE("Verify corrupted symbol");
    version_string[0] = '-';
    auto str = std::stringstream(version_string);

    auto responce = olp::parser::parse_result<dr::CatalogVersionResponse>(str);
    ASSERT_FALSE(responce.IsSuccessful());
    ASSERT_EQ(responce.GetError().GetErrorCode(),
              olp::client::ErrorCode::Unknown);
    ASSERT_EQ(responce.GetError().GetMessage(), "Fail parsing response.");
  }
}

TEST(JsonResultParserTest, Partitions) {
  auto partitions =
      mockserver::ReadDefaultResponses::GeneratePartitionsResponse();
  auto partitions_string = olp::serializer::serialize(partitions);

  {
    SCOPED_TRACE("Verify valid partitions responce");
    auto str = std::stringstream(partitions_string);

    auto responce = olp::parser::parse_result<dr::PartitionsResponse>(str);
    ASSERT_TRUE(responce.IsSuccessful());
    ASSERT_EQ(10u, responce.GetResult().GetPartitions().size());
  }
  {
    SCOPED_TRACE("Verify corrupted with additional symbol");
    auto str = std::stringstream(partitions_string + "_");

    auto responce = olp::parser::parse_result<dr::PartitionsResponse>(str);
    ASSERT_FALSE(responce.IsSuccessful());
    ASSERT_EQ(responce.GetError().GetErrorCode(),
              olp::client::ErrorCode::Unknown);
    ASSERT_EQ(responce.GetError().GetMessage(), "Fail parsing response.");
  }
}
