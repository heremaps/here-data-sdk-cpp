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

#include "repositories/CatalogCacheRepository.h"

#include <gmock/gmock.h>
#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/client/OlpClientSettingsFactory.h>

namespace {

namespace read = olp::dataservice::read;
namespace client = olp::client;
namespace cache = olp::cache;

constexpr auto kCatalog = "hrn:here:data::olp-here-test:catalog";

TEST(CatalogCacheRepositoryTest, DefaultExpiry) {
  const auto hrn = client::HRN::FromString(kCatalog);

  read::model::Catalog model_catalog;

  {
    SCOPED_TRACE("Disable expiration");

    const auto default_expiry = std::chrono::seconds::max();
    std::shared_ptr<cache::KeyValueCache> cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
    read::repository::CatalogCacheRepository repository(hrn, cache,
                                                        default_expiry);

    repository.Put(model_catalog);
    const auto result = repository.Get();

    EXPECT_TRUE(result);
  }

  {
    SCOPED_TRACE("Expired");

    const auto default_expiry = std::chrono::seconds(-1);
    std::shared_ptr<cache::KeyValueCache> cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
    read::repository::CatalogCacheRepository repository(hrn, cache,
                                                        default_expiry);

    repository.Put(model_catalog);
    const auto result = repository.Get();

    EXPECT_FALSE(result);
  }
}

TEST(CatalogCacheRepositoryTest, VersionsList) {
  const auto hrn = client::HRN::FromString(kCatalog);

  read::model::VersionInfos model_versions;
  model_versions.SetVersions(
      std::vector<olp::dataservice::read::model::VersionInfo>(1));
  const auto default_expiry = std::chrono::seconds::max();
  std::shared_ptr<cache::KeyValueCache> cache =
      olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
  read::repository::CatalogCacheRepository repository(hrn, cache,
                                                      default_expiry);

  {
    SCOPED_TRACE("Put/get versions list");

    repository.PutVersionInfos(3, 4, model_versions);
    const auto result = repository.GetVersionInfos(3, 4);

    EXPECT_TRUE(result);
    EXPECT_EQ(1u, result->GetVersions().size());
  }

  {
    SCOPED_TRACE("Get versions list wrong key");

    const auto result = repository.GetVersionInfos(300, 3001);

    EXPECT_FALSE(result);
  }

  {
    SCOPED_TRACE("List versions expired");

    const auto default_expiry = std::chrono::seconds(-1);
    std::shared_ptr<cache::KeyValueCache> cache_expiration =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
    read::repository::CatalogCacheRepository repository_expiration(
        hrn, cache_expiration, default_expiry);

    repository_expiration.PutVersionInfos(3, 4, model_versions);
    const auto result = repository_expiration.GetVersionInfos(3, 4);

    EXPECT_FALSE(result);
  }
}

}  // namespace
