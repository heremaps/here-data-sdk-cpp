/*
 * Copyright (C) 2020-2024 HERE Europe B.V.
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

constexpr auto kCatalog = "hrn:here:data::olp-here-test:catalog";

TEST(CatalogCacheRepositoryTest, DefaultExpiry) {
  const auto hrn = olp::client::HRN::FromString(kCatalog);

  olp::dataservice::read::model::Catalog model_catalog;

  {
    SCOPED_TRACE("Disable expiration");

    const auto default_expiry = std::chrono::seconds::max();
    std::shared_ptr<olp::cache::KeyValueCache> cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
    olp::dataservice::read::repository::CatalogCacheRepository repository(
        hrn, cache, default_expiry);

    repository.Put(model_catalog);
    const auto result = repository.Get();

    EXPECT_TRUE(result);
  }

  {
    SCOPED_TRACE("Expired");

    const auto default_expiry = std::chrono::seconds(-1);
    std::shared_ptr<olp::cache::KeyValueCache> cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
    olp::dataservice::read::repository::CatalogCacheRepository repository(
        hrn, cache, default_expiry);

    repository.Put(model_catalog);
    const auto result = repository.Get();

    EXPECT_FALSE(result);
  }
}

}  // namespace
