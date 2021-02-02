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

#include <gmock/gmock.h>
#include <olp/core/client/ApiLookupClient.h>

namespace {
namespace client = olp::client;

TEST(DefaultLookupEndpointProviderTest, ParenthesisOperator) {
  const struct {
    const std::string partition;
    const std::string url;
  } valid_apis[] = {
      {"here", "https://api-lookup.data.api.platform.here.com/lookup/v1"},
      {"here-dev",
       "https://api-lookup.data.api.platform.sit.here.com/lookup/v1"},
      {"here-cn", "https://api-lookup.data.api.platform.hereolp.cn/lookup/v1"},
      {"here-cn-dev",
       "https://api-lookup.data.api.platform.in.hereolp.cn/lookup/v1"}};

  auto provider = client::DefaultLookupEndpointProvider();

  {
    SCOPED_TRACE("Valid usage");
    for (const auto& api : valid_apis) {
      ASSERT_EQ(api.url, provider(api.partition));
    }
  }

  {
    SCOPED_TRACE("Not found partition");
    const std::string unknown_partition = "unknown";
    ASSERT_TRUE(provider(unknown_partition).empty());
  }

  {
    SCOPED_TRACE("Empty partition");
    ASSERT_TRUE(provider({}).empty());
  }
}

}  // namespace
