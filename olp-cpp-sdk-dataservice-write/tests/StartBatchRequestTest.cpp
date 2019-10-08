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

#include <gmock/gmock.h>
#include <olp/dataservice/write/VersionedLayerClient.h>

namespace {

using namespace olp::dataservice::write::model;

TEST(StartBatchRequestTest, StartBatchRequest) {
  ASSERT_FALSE(StartBatchRequest().GetLayers());
  ASSERT_FALSE(StartBatchRequest().GetVersionDependencies());
  ASSERT_FALSE(StartBatchRequest().GetBillingTag());

  auto sbr =
      StartBatchRequest()
          .WithLayers({"layer1", "layer2"})
          .WithVersionDependencies({{false, "hrn1", 0}, {true, "hrn2", 1}})
          .WithBillingTag("billingTag");
  ASSERT_TRUE(sbr.GetLayers());
  ASSERT_FALSE(sbr.GetLayers()->empty());
  ASSERT_EQ(2ull, sbr.GetLayers()->size());
  ASSERT_EQ("layer1", (*sbr.GetLayers())[0]);
  ASSERT_EQ("layer2", (*sbr.GetLayers())[1]);
  ASSERT_TRUE(sbr.GetVersionDependencies());
  ASSERT_FALSE(sbr.GetVersionDependencies()->empty());
  ASSERT_EQ(2ull, sbr.GetVersionDependencies()->size());
  ASSERT_FALSE((*sbr.GetVersionDependencies())[0].GetDirect());
  ASSERT_TRUE((*sbr.GetVersionDependencies())[1].GetDirect());
  ASSERT_EQ("hrn1", (*sbr.GetVersionDependencies())[0].GetHrn());
  ASSERT_EQ("hrn2", (*sbr.GetVersionDependencies())[1].GetHrn());
  ASSERT_EQ(0, (*sbr.GetVersionDependencies())[0].GetVersion());
  ASSERT_EQ(1, (*sbr.GetVersionDependencies())[1].GetVersion());
  ASSERT_TRUE(sbr.GetBillingTag());
  ASSERT_EQ("billingTag", (*sbr.GetBillingTag()));
}

}  // namespace
