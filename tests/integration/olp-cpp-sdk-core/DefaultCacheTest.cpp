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

#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include <gtest/gtest.h>
#include <olp/core/cache/DefaultCache.h>
#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/utils/Dir.h>

namespace {
namespace cache = olp::cache;
namespace client = olp::client;
namespace utils = olp::utils;

using TestFunction = std::function<void(uint8_t thread_id)>;

TEST(DefaultCacheTest, DataExpiration) {
  const std::string content_key = "test_key";
  cache::CacheSettings settings;
  settings.max_memory_cache_size = 5;  // bytes
  settings.disk_path_mutable =
      olp::utils::Dir::TempDirectory() + "/DataExpiration";
  const auto expire_time = 1;

  {
    SCOPED_TRACE("Create a disk cache, write data.");
    cache::DefaultCache cache(settings);
    EXPECT_EQ(cache.Open(), cache::DefaultCache::StorageOpenResult::Success);

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
    cache::DefaultCache cache(settings);
    EXPECT_EQ(cache.Open(), cache::DefaultCache::StorageOpenResult::Success);

    auto buffer = cache.Get(content_key);
    ASSERT_EQ(buffer, nullptr);

    cache.Close();
  }
}

void StartThreads(std::vector<std::thread>& threads, TestFunction func) {}

TEST(DefaultCacheTest, ConcurrencyWithEviction) {
  cache::CacheSettings cache_settings;
  cache_settings.compression = cache::CompressionType::kNoCompression;
  cache_settings.eviction_policy = cache::EvictionPolicy::kLeastRecentlyUsed;
  cache_settings.disk_path_mutable =
      utils::Dir::TempDirectory() + "/Concurrency";
  cache_settings.max_disk_storage = 1024ull * 1024ull * 1024ull * 5ull;
  cache_settings.max_memory_cache_size = 0u;
  cache_settings.max_chunk_size = 1024u * 1024u * 8u;

  // Data to be added is 500kBytes
  constexpr size_t kValueSize = 1024u * 250u;
  constexpr size_t kLoops = 3000u;
  constexpr size_t kThreadsCount = 3u;

  auto now = std::chrono::system_clock::now();
  const std::string kKeySuffix =
      std::to_string(std::chrono::system_clock::to_time_t(now));

  const auto value =
      std::make_shared<cache::KeyValueCache::ValueType>(kValueSize, 'x');
  const std::string metadata =
      R"jsonString({ "partitions": [{"version":4,"partition":"269","layer":"testlayer","dataHandle":"4eed6ed1-0d32-43b9-ae79-043cb4256432"}]})jsonString";
  std::vector<std::thread> threads;

  // Open shareable cache
  std::shared_ptr<cache::KeyValueCache> cache =
      client::OlpClientSettingsFactory::CreateDefaultCache(cache_settings);
  ASSERT_TRUE(cache);

  // Start 3 threads all writing exactly kLoops times metadata and data.
  for (size_t index = 0; index < kThreadsCount; ++index) {
    threads.emplace_back(std::thread([=] {
      for (size_t loop = 0; loop < kLoops; ++loop) {
        const std::string key = "data::key::" + kKeySuffix +
                                "::" + std::to_string(index) +
                                "::" + std::to_string(loop);
        const std::string key_meta = key + "meta";

        SCOPED_TRACE(::testing::Message() << "Key=" << key);

        // Put and Get to verify it worked.
        // We should always be able to put data into cache even with LRU
        EXPECT_TRUE(cache->Put(key, value, 1000));
        EXPECT_TRUE(
            cache->Put(key_meta, metadata, [&] { return metadata; }, 1000));

        auto get_value = cache->Get(key);
        EXPECT_TRUE(get_value);
        if (get_value) {
          EXPECT_EQ(*value, *get_value);
        }

        auto get_meta = cache->Get(
            key_meta, [](const std::string& value) { return value; });
        EXPECT_FALSE(get_meta.empty());

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
      }
    }));
  }

  // Wait for the threads to finish
  for (auto& thread : threads) {
    thread.join();
  }

  // Wait for 10 seconds and give leveldb a chance to compact.
  std::cout << "Test finished -> waiting for leveldb" << std::endl;
  cache.reset();
}

}  // namespace
