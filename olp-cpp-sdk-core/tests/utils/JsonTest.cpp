/*
 * Copyright (C) 2026 HERE Europe B.V.
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

#include <boost/json/parse.hpp>

namespace {

using UtilsTest = testing::Test;

const std::string kJsonData = R"json(
    {
        "catalogs": [
            {
            "hrn": "hrn:here:data::olp:ocm",
            "version": 17,
            "dependencies": [],
            "metadata_type": "partitions"
            }
        ],
        "regions": [
            {
            "id": 423423,
            "parent_id": 14123411,
            "catalogs": [
                {
                "hrn": "hrn:here:data::olp:ocm",
                "status": "pending",
                "size_raw_bytes": 4623545,
                "layer_groups": [
                    "rendering"
                ],
                "tiles": [
                    2354325,
                    5243252
                ]
                }
            ]
            }
        ]
    })json";

TEST(UtilsTest, BoostJsonAvailable) {
  boost::system::error_code error_code;
  auto parsed_json = boost::json::parse(kJsonData, error_code);

  EXPECT_FALSE(error_code.failed());
}

}  // namespace
