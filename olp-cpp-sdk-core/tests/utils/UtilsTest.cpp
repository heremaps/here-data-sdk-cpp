/*
 * Copyright (C) 2023 HERE Europe B.V.
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

#include <string>

#include <olp/core/utils/Credentials.h>

namespace {

using UtilsTest = testing::Test;

TEST(UtilsTest, Credentials) {
  {
    SCOPED_TRACE("Empty url");

    EXPECT_TRUE(olp::utils::CensorCredentialsInUrl("").empty());
  }

  {
    SCOPED_TRACE("Nothing to censor");

    const std::string url{
        "https://sab.metadata.data.api.platform.here.com/metadata/v1/"
        "catalogs/"
        "hrn:here:data::olp-here:ocm-patch/"
        "versions?endVersion=46&startVersion=0"};

    EXPECT_EQ(olp::utils::CensorCredentialsInUrl(url), url);
  }

  {
    SCOPED_TRACE("Censoring app_id, app_code");

    const std::string app_id{"2ARQ22QED2TMaSsPlC88DO"};

    const std::string app_code{
        "9849asdasdasYiukljbnSIUYAGlhbLASYJDgljkhjblhbuhblkSABLhb12312312321231"
        "12"
        "321312l;kasjdf"};

    const std::string url_with_credentials{
        "https://api-lookup.data.api.platform.here.com/lookup/v1/resources/"
        "hrn:here:data::olp-here:ocm-patch/"
        "apis?app_id=" +
        app_id + "&app_code=" + app_code};

    const auto result =
        olp::utils::CensorCredentialsInUrl(url_with_credentials);

    EXPECT_EQ(url_with_credentials.size(), result.size());
    EXPECT_EQ(result.find(app_id, 0), std::string::npos);
    EXPECT_EQ(result.find(app_code, 0), std::string::npos);
  }

  {
    SCOPED_TRACE("Censoring apiKey");

    const std::string appKey{"SomeApiKey"};

    const std::string url_with_credentials{
        "https://api-lookup.data.api.platform.here.com/lookup/v1/resources/"
        "hrn:here:data::olp-here:ocm-patch/"
        "apis?apiKey=" +
        appKey};

    const auto result =
        olp::utils::CensorCredentialsInUrl(url_with_credentials);

    EXPECT_EQ(url_with_credentials.size(), result.size());
    EXPECT_EQ(result.find(appKey, 0), std::string::npos);
  }
}
}  // namespace
