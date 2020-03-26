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
#include <thread>

#include <olp/core/cache/DefaultCache.h>

namespace {
using namespace olp::cache;

TEST(CacheTest, DataExpiration) {
  const std::string content_key = "test_key";
  CacheSettings settings;
  settings.max_memory_cache_size = 5;  // bytes
  settings.disk_path_mutable = "./cache";
  const auto expire_time = 1;

  {
    SCOPED_TRACE("Create a disk cache, write data.");
    DefaultCache cache(settings);
    EXPECT_EQ(cache.Open(), DefaultCache::StorageOpenResult::Success);

    const std::string content = "12345";
    auto buffer = std::make_shared<std::vector<unsigned char>>(
        std::begin(content), std::end(content));
    EXPECT_TRUE(cache.Put(content_key, buffer, expire_time));
    // check if data is in cache
    auto buffer_out_1 = cache.Get(content_key);
    ASSERT_NE(buffer_out_1, nullptr);
    cache.Close();
  }

  std::this_thread::sleep_for(std::chrono::seconds(expire_time + 1));

  {
    SCOPED_TRACE("Check that data is expired after timeout.");
    DefaultCache cache(settings);
    EXPECT_EQ(cache.Open(), DefaultCache::StorageOpenResult::Success);

    auto buffer = cache.Get(content_key);
    ASSERT_EQ(buffer, nullptr);

    cache.Close();
  }
}

}  // namespace
