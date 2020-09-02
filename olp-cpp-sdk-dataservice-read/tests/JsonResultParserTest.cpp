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
#include <olp/dataservice/read/Types.h>
#include "generated/parser/PartitionsParser.h"
#include "JsonResultParser.h"
#include "generated/serializer/PartitionsSerializer.h"
#include "generated/serializer/JsonSerializer.h"
#include "ReadDefaultResponses.h"
#include "ExtendedApiResponse.h"
// clang-format on

namespace dr = olp::dataservice::read;

TEST(JsonResultParserTest, ParseResult) {
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
  {
    SCOPED_TRACE("Verify merged responses");
    auto partitions2 =
        mockserver::ReadDefaultResponses::GeneratePartitionsResponse(2);
    auto partitions_string2 = olp::serializer::serialize(partitions2);

    auto str = std::stringstream(partitions_string + partitions_string2);

    auto responce = olp::parser::parse_result<dr::PartitionsResponse>(str);
    ASSERT_FALSE(responce.IsSuccessful());
    ASSERT_EQ(responce.GetError().GetErrorCode(),
              olp::client::ErrorCode::Unknown);
    ASSERT_EQ(responce.GetError().GetMessage(), "Fail parsing response.");
  }
  {
    SCOPED_TRACE("Verify missed last brace");
    std::string json_input =
        partitions_string.substr(0, partitions_string.size() - 1);

    auto str = std::stringstream(json_input);
    auto responce = olp::parser::parse_result<dr::PartitionsResponse>(str);
    ASSERT_FALSE(responce.IsSuccessful());
    ASSERT_EQ(responce.GetError().GetErrorCode(),
              olp::client::ErrorCode::Unknown);
    ASSERT_EQ(responce.GetError().GetMessage(), "Fail parsing response.");
  }
}

TEST(JsonResultParserTest, ExtendedResponse) {
  auto partitions =
      mockserver::ReadDefaultResponses::GeneratePartitionsResponse();
  auto partitions_string = olp::serializer::serialize(partitions);
  using ExtendedResponse =
      dr::ExtendedApiResponse<dr::PartitionsResult, olp::client::ApiError, int>;

  {
    SCOPED_TRACE("Verify valid extended partitions responce");
    auto str = std::stringstream(partitions_string);

    auto responce = olp::parser::parse_result<ExtendedResponse>(str, 100);
    ASSERT_TRUE(responce.IsSuccessful());
    ASSERT_EQ(10u, responce.GetResult().GetPartitions().size());
    ASSERT_EQ(100, responce.GetPayload());
  }
  {
    SCOPED_TRACE("Verify corrupted with additional symbol");
    auto str = std::stringstream(partitions_string + "_");

    auto responce = olp::parser::parse_result<ExtendedResponse>(str, 100);
    ASSERT_FALSE(responce.IsSuccessful());
    ASSERT_EQ(responce.GetError().GetErrorCode(),
              olp::client::ErrorCode::Unknown);
    ASSERT_EQ(responce.GetError().GetMessage(), "Fail parsing response.");
  }
}

TEST(JsonResultParserTest, WithArgs) {
  auto partitions =
      mockserver::ReadDefaultResponses::GeneratePartitionsResponse();
  auto partitions_string = olp::serializer::serialize(partitions);

  struct Result {
    dr::PartitionsResult result;
    std::string data;
    int additional_data;
  };

  using ResponseWithArgs = dr::Response<Result>;

  {
    SCOPED_TRACE("Verify valid partitions responce with some args");
    auto str = std::stringstream(partitions_string);

    auto responce =
        olp::parser::parse_result<ResponseWithArgs, dr::PartitionsResult>(
            str, "data", 10);
    ASSERT_TRUE(responce.IsSuccessful());
    ASSERT_EQ(10u, responce.GetResult().result.GetPartitions().size());
    ASSERT_EQ("data", responce.GetResult().data);
    ASSERT_EQ(10, responce.GetResult().additional_data);
  }
  {
    SCOPED_TRACE("Verify corrupted with additional symbol");
    auto str = std::stringstream(partitions_string + "_");

    auto responce =
        olp::parser::parse_result<ResponseWithArgs, dr::PartitionsResult>(
            str, "data", 10);
    ASSERT_FALSE(responce.IsSuccessful());
    ASSERT_EQ(responce.GetError().GetErrorCode(),
              olp::client::ErrorCode::Unknown);
    ASSERT_EQ(responce.GetError().GetMessage(), "Fail parsing response.");
  }
}
