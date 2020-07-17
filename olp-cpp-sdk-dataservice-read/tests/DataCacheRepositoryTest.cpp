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

#include "repositories/DataCacheRepository.h"

#include <gmock/gmock.h>
#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/client/OlpClientSettingsFactory.h>

namespace {
namespace read = olp::dataservice::read;
namespace repository = olp::dataservice::read::repository;
namespace client = olp::client;
namespace cache = olp::cache;

constexpr auto kCatalog = "hrn:here:data::olp-here-test:catalog";
constexpr auto kDataHandle = "4eed6ed1-0d32-43b9-ae79-043cb4256432";

TEST(PartitionsCacheRepositoryTest, DefaultExpiry) {
  const auto hrn = client::HRN::FromString(kCatalog);
  const auto layer = "layer";

  const auto data = std::vector<unsigned char>{1, 2, 3};
  const auto model_data = std::make_shared<std::vector<unsigned char>>(data);

  {
    SCOPED_TRACE("Disable expiration");

    const auto default_expiry = std::chrono::seconds::max();
    std::shared_ptr<cache::KeyValueCache> cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
    repository::DataCacheRepository repository(hrn, cache, default_expiry);

    repository.Put(model_data, layer, kDataHandle);
    const auto result = repository.Get(layer, kDataHandle);

    EXPECT_TRUE(result);
  }

  {
    SCOPED_TRACE("Expired");

    const auto default_expiry = std::chrono::seconds(-1);
    std::shared_ptr<cache::KeyValueCache> cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
    repository::DataCacheRepository repository(hrn, cache, default_expiry);

    repository.Put(model_data, layer, kDataHandle);
    const auto result = repository.Get(layer, kDataHandle);

    EXPECT_FALSE(result);
  }
}

TEST(PartitionsCacheRepositoryTest, IsCached) {
  const auto hrn = client::HRN::FromString(kCatalog);
  const auto layer = "layer";

  const auto data = std::vector<unsigned char>{1, 2, 3};
  const auto model_data = std::make_shared<std::vector<unsigned char>>(data);

  {
    SCOPED_TRACE("Is cached");

    std::shared_ptr<cache::KeyValueCache> cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
    repository::DataCacheRepository repository(hrn, cache);

    repository.Put(model_data, layer, kDataHandle);
    const auto result = repository.IsCached(layer, kDataHandle);

    EXPECT_TRUE(result);
  }

  {
    SCOPED_TRACE("Is not cached");

    std::shared_ptr<cache::KeyValueCache> cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
    repository::DataCacheRepository repository(hrn, cache);

    const auto result = repository.IsCached(layer, kDataHandle);

    EXPECT_FALSE(result);
  }
}

}  // namespace
