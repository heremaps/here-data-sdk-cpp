/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
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

#include "CatalogClientTestBase.h"

#include <regex>

#include <gmock/gmock.h>
#include <matchers/NetworkUrlMatchers.h>
#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/DefaultCache.h>
#include <olp/core/client/HRN.h>
#include <olp/core/porting/make_unique.h>
#include <olp/core/utils/Dir.h>
#include <olp/dataservice/read/CatalogClient.h>
#include "HttpResponses.h"

namespace {

using namespace olp::dataservice::read;
using namespace testing;
using namespace olp::tests::common;
using namespace olp::tests::integration;

#ifdef _WIN32
constexpr auto kClientTestDir = "\\catalog_client_test";
constexpr auto kClientTestCacheDir = "\\catalog_client_test\\cache";
#else
constexpr auto kClientTestDir = "/catalog_client_test";
constexpr auto kClientTestCacheDir = "/cata.log_client_test/cache";
#endif

class CatalogClientCacheTest : public CatalogClientTestBase {
 protected:
  void SetUp() override {
    CatalogClientTestBase::SetUp();
    olp::cache::CacheSettings settings;
    switch (GetParam()) {
      case CacheType::IN_MEMORY: {
        // use the default value
        break;
      }
      case CacheType::DISK: {
        settings.max_memory_cache_size = 0;
        settings.disk_path_mutable =
            olp::utils::Dir::TempDirectory() + kClientTestCacheDir;
        ClearCache(settings.disk_path_mutable.get());
        break;
      }
      case CacheType::BOTH: {
        settings.disk_path_mutable =
            olp::utils::Dir::TempDirectory() + kClientTestCacheDir;
        ClearCache(settings.disk_path_mutable.get());
        break;
      }
      case CacheType::NONE: {
        // We don't create a cache here
        settings_.cache = nullptr;
        return;
      }
      default:
        // shouldn't get here
        break;
    }

    cache_ = std::make_shared<olp::cache::DefaultCache>(settings);
    ASSERT_EQ(olp::cache::DefaultCache::StorageOpenResult::Success,
              cache_->Open());
    settings_.cache = cache_;
  }

  void TearDown() override {
    if (cache_) {
      cache_->Close();
    }
    ClearCache(olp::utils::Dir::TempDirectory() + kClientTestDir);
    network_mock_.reset();
  }

 protected:
  void ClearCache(const std::string& path) { olp::utils::Dir::remove(path); }

  std::shared_ptr<olp::cache::DefaultCache> cache_;
};

TEST_P(CatalogClientCacheTest, GetApi) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .Times(1);
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(1);

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);

  auto request = CatalogVersionRequest().WithStartVersion(-1);

  auto future = catalog_client->GetLatestVersion(request);
  auto catalog_version_response = future.GetFuture().get();

  ASSERT_TRUE(catalog_version_response.IsSuccessful())
      << ApiErrorToString(catalog_version_response.GetError());
}

TEST_P(CatalogClientCacheTest, GetCatalog) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
      .Times(1);
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1);

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_);
  auto request = CatalogRequest();
  auto future = catalog_client->GetCatalog(request);
  CatalogResponse catalog_response = future.GetFuture().get();

  ASSERT_TRUE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());

  future = catalog_client->GetCatalog(request);
  CatalogResponse catalog_response2 = future.GetFuture().get();

  ASSERT_TRUE(catalog_response2.IsSuccessful())
      << ApiErrorToString(catalog_response2.GetError());
  ASSERT_EQ(catalog_response2.GetResult().GetName(),
            catalog_response.GetResult().GetName());
}

INSTANTIATE_TEST_SUITE_P(, CatalogClientCacheTest,
                         ::testing::Values(CacheType::IN_MEMORY,
                                           CacheType::DISK, CacheType::BOTH,
                                           CacheType::NONE));
}  // namespace
