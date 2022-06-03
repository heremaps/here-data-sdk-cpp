/*
 * Copyright (C) 2022 HERE Europe B.V.
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

#include <olp/dataservice/read/Types.h>

namespace {

namespace read = olp::dataservice::read;
using olp::client::NetworkStatistics;

bool IsEqual(const NetworkStatistics& lhs, const NetworkStatistics& rhs) {
  return lhs.GetBytesDownloaded() == rhs.GetBytesDownloaded() &&
         lhs.GetBytesUploaded() == rhs.GetBytesUploaded();
}

TEST(ExtendedApiResponseTest, TypesAreBackwardsCompatible) {
  using StringResponse = read::Response<std::string>;
  using ExtendedStringResponse = read::ExtendedResponse<std::string>;

  // Response copy preserves the payload
  ExtendedStringResponse response_copy =
      ExtendedStringResponse("test", NetworkStatistics(1, 2));
  EXPECT_EQ(response_copy.GetResult(), "test");
  EXPECT_TRUE(IsEqual(response_copy.GetPayload(), NetworkStatistics(1, 2)));

  // Extended response could be slices to normal
  StringResponse sliced_response = ExtendedStringResponse("test");
  EXPECT_EQ(sliced_response.GetResult(), "test");

  // Normal response is implicitly converted to extended
  ExtendedStringResponse extended_response = StringResponse("test");
  EXPECT_EQ(extended_response.GetResult(), "test");

  // Normal callback type works for both normal and extended response
  read::Callback<std::string> normal_callback =
      [](read::Response<std::string> response) {
        EXPECT_EQ(response.GetResult(), "test");
      };

  normal_callback(ExtendedStringResponse("test", NetworkStatistics(2, 3)));
  normal_callback(StringResponse("test"));

  // Extended callback type works for both response types
  read::ExtendedCallback<std::string> extended_callback_1 =
      [](read::ExtendedResponse<std::string> response) {
        EXPECT_EQ(response.GetResult(), "test");
        EXPECT_TRUE(IsEqual(response.GetPayload(), NetworkStatistics(2, 3)));
      };

  extended_callback_1(ExtendedStringResponse("test", NetworkStatistics(2, 3)));

  read::ExtendedCallback<std::string> extended_callback_2 =
      [](read::ExtendedResponse<std::string> response) {
        EXPECT_EQ(response.GetResult(), "test");
        EXPECT_TRUE(IsEqual(response.GetPayload(), NetworkStatistics()));
      };

  extended_callback_2(StringResponse("test"));

  // Normal callback type could be casted to extended and consume both response
  // types.
  read::ExtendedCallback<std::string> callback_cast =
      [](read::Response<std::string> response) {
        EXPECT_EQ(response.GetResult(), "test");
      };

  callback_cast(ExtendedStringResponse("test", NetworkStatistics(2, 3)));
  callback_cast(StringResponse("test"));

  // Extended callback could be casted to normal callback, but the payload is
  // sliced away.
  read::Callback<std::string> callback_cast2 =
      [](read::ExtendedResponse<std::string> response) {
        EXPECT_EQ(response.GetResult(), "test");
        EXPECT_TRUE(IsEqual(response.GetPayload(), NetworkStatistics()));
      };

  callback_cast2(ExtendedStringResponse("test", NetworkStatistics(2, 3)));
  callback_cast2(StringResponse("test"));
}

}  // namespace
