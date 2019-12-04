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

#include <olp/core/client/HRN.h>

#include <gtest/gtest.h>

using olp::client::HRN;

namespace {

TEST(HRNTest, InitializeCatalogHRNValid) {
  {
    SCOPED_TRACE("Valid Catalog HRN");
    HRN hrn("hrn:here:data::test:hereos-internal-test-v2");
    EXPECT_FALSE(hrn.IsNull());
  }
  {
    SCOPED_TRACE("Valid Catalog HRN without realm");
    HRN hrn("hrn:here:data:::hereos-internal-test-v2");
    EXPECT_FALSE(hrn.IsNull());
  }
}

TEST(HRNTest, InitializeSchemaHRNValid) {
  {
    SCOPED_TRACE("Valid Schema HRN");
    HRN hrn("hrn:here:schema::test:group_id:artifact_id:version");
    EXPECT_FALSE(hrn.IsNull());
  }
  {
    SCOPED_TRACE("Valid Schema HRN without realm");
    HRN hrn("hrn:here:schema:::group_id:artifact_id:version");
    EXPECT_FALSE(hrn.IsNull());
  }
}

TEST(HRNTest, InitializePipelineHRNValid) {
  {
    SCOPED_TRACE("Valid Pipeline HRN");
    HRN hrn("hrn:here:pipeline::test:test_pipeline");
    EXPECT_FALSE(hrn.IsNull());
  }
  {
    SCOPED_TRACE("Valid Pipeline HRN without realm");
    HRN hrn("hrn:here:pipeline:::test_pipeline");
    EXPECT_FALSE(hrn.IsNull());
  }
}

TEST(HRNTest, InitializeWithStringInvalid) {
  {
    SCOPED_TRACE("Empty HRN");
    HRN hrn("");
    EXPECT_TRUE(hrn.IsNull());
  }
  {
    SCOPED_TRACE("Invalid HRN");
    HRN hrn("invalid_hrn");
    EXPECT_TRUE(hrn.IsNull());
  }
  {
    SCOPED_TRACE("Invalid HRN, valid prefix");
    HRN hrn("hrn:invalid_hrn");
    EXPECT_TRUE(hrn.IsNull());
  }
}

TEST(HRNTest, InitializeCatalogHRNInvalid) {
  {
    SCOPED_TRACE("Invalid Catalog HRN (missing catalog name)");
    HRN hrn("hrn:here:data::test:");
    EXPECT_TRUE(hrn.IsNull());
  }
}

TEST(HRNTest, InitializeSchemaHRNInvalid) {
  {
    SCOPED_TRACE("Invalid Schema HRN (missing group_id)");
    HRN hrn("hrn:here:schema::test::artifact_id:version");
    EXPECT_TRUE(hrn.IsNull());
  }
  {
    SCOPED_TRACE("Invalid Schema HRN (missing artifact_id)");
    HRN hrn("hrn:here:schema::test:group_id::version");
    EXPECT_TRUE(hrn.IsNull());
  }
  {
    SCOPED_TRACE("Invalid Schema HRN (missing version)");
    HRN hrn("hrn:here:schema::test:group_id:artifact_id:");
    EXPECT_TRUE(hrn.IsNull());
  }
}

TEST(HRNTest, InitializePipelineHRNInvalid) {
  {
    SCOPED_TRACE("Invalid Pipeline HRN (missing pipeline id)");
    HRN hrn("hrn:here:pipeline::test:");
    EXPECT_TRUE(hrn.IsNull());
  }
}

TEST(HRNTest, CompareHRNs) {
  SCOPED_TRACE("Compare equal HRNs");
  {
    SCOPED_TRACE("Compare equal Catalog HRNs");
    EXPECT_EQ(HRN("hrn:here:data:::hereos-internal-test-v2"),
              HRN("hrn:here:data:::hereos-internal-test-v2"));
    EXPECT_NE(HRN("hrn:here:data:::hereos-internal-test-v1"),
              HRN("hrn:here:data:::hereos-internal-test-v2"));
    EXPECT_NE(HRN("hrn:here:data:::hereos-internal-test-v2"),
              HRN("hrn:here:data::test:hereos-internal-test-v2"));
  }
  {
    SCOPED_TRACE("Compare equal Schema HRNs");
    EXPECT_EQ(HRN("hrn:here:schema:::group_id:artifact_id:version"),
              HRN("hrn:here:schema:::group_id:artifact_id:version"));
    EXPECT_NE(HRN("hrn:here:schema:::group_id:artifact_id:version_1"),
              HRN("hrn:here:schema:::group_id:artifact_id:version_2"));
    EXPECT_NE(HRN("hrn:here:schema:::group_id:artifact_id:version_1"),
              HRN("hrn:here:schema::test:group_id:artifact_id:version_2"));
  }
  {
    SCOPED_TRACE("Compare equal Pipeline HRNs");
    EXPECT_EQ(HRN("hrn:here:data:::test_pipeline"),
              HRN("hrn:here:data:::test_pipeline"));
    EXPECT_NE(HRN("hrn:here:data:::test_pipeline_1"),
              HRN("hrn:here:data:::test_pipeline_2"));
    EXPECT_NE(HRN("hrn:here:data:::test_pipeline"),
              HRN("hrn:here:data::test:test_pipeline"));
  }
}

TEST(HRNTest, ToString) {
  EXPECT_EQ(HRN("hrn:here:data:::hereos-internal-test-v2").ToString(),
            "hrn:here:data:::hereos-internal-test-v2");
  EXPECT_EQ(HRN("hrn:here:schema:::group_id:artifact_id:version").ToString(),
            "hrn:here:schema:::group_id:artifact_id:version");
  EXPECT_EQ(HRN("hrn:here:data:::test_pipeline").ToString(),
            "hrn:here:data:::test_pipeline");
}

TEST(HRNTest, Parsing) {
  {
    SCOPED_TRACE("Valid Catalog HRN");
    HRN hrn("hrn:here:data:EU:test:hereos-internal-test-v2");
    EXPECT_EQ(hrn.partition, "here");
    EXPECT_EQ(hrn.service, HRN::ServiceType::Data);
    EXPECT_EQ(hrn.region, "EU");
    EXPECT_EQ(hrn.account, "test");
    EXPECT_EQ(hrn.catalogId, "hereos-internal-test-v2");
  }
  {
    SCOPED_TRACE("Valid Schema HRN");
    HRN hrn("hrn:here:schema:CH:test:group_id:artifact_id:version");
    EXPECT_EQ(hrn.partition, "here");
    EXPECT_EQ(hrn.service, HRN::ServiceType::Schema);
    EXPECT_EQ(hrn.region, "CH");
    EXPECT_EQ(hrn.account, "test");
    EXPECT_EQ(hrn.groupId, "group_id");
    EXPECT_EQ(hrn.schemaName, "artifact_id");
    EXPECT_EQ(hrn.version, "version");
  }
  {
    SCOPED_TRACE("Valid Pipeline HRN");
    HRN hrn("hrn:here:pipeline:US:test:test_pipeline");
    EXPECT_EQ(hrn.partition, "here");
    EXPECT_EQ(hrn.service, HRN::ServiceType::Pipeline);
    EXPECT_EQ(hrn.region, "US");
    EXPECT_EQ(hrn.account, "test");
    EXPECT_EQ(hrn.pipelineId, "test_pipeline");
  }
}

}  // namespace
