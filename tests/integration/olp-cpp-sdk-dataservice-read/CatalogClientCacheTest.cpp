/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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
using testing::_;
namespace read = olp::dataservice::read;
namespace integration = olp::tests::integration;

#ifdef _WIN32
constexpr auto kClientTestDir = "\\catalog_client_test";
constexpr auto kClientTestCacheDir = "\\catalog_client_test\\cache";
#else
constexpr auto kClientTestDir = "/catalog_client_test";
constexpr auto kClientTestCacheDir = "/cata.log_client_test/cache";
#endif
constexpr auto kWaitTimeout = std::chrono::seconds(3);

class CatalogClientCacheTest : public integration::CatalogClientTestBase {
 protected:
  void SetUp() override {
    CatalogClientTestBase::SetUp();
    olp::cache::CacheSettings settings;
    switch (GetParam()) {
      case integration::CacheType::IN_MEMORY: {
        // use the default value
        break;
      }
      case integration::CacheType::DISK: {
        settings.max_memory_cache_size = 0;
        settings.disk_path_mutable =
            olp::utils::Dir::TempDirectory() + kClientTestCacheDir;
        ClearCache(settings.disk_path_mutable.get());
        break;
      }
      case integration::CacheType::BOTH: {
        settings.disk_path_mutable =
            olp::utils::Dir::TempDirectory() + kClientTestCacheDir;
        ClearCache(settings.disk_path_mutable.get());
        break;
      }
      case integration::CacheType::NONE: {
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
  void ClearCache(const std::string& path) { olp::utils::Dir::Remove(path); }

  std::shared_ptr<olp::cache::DefaultCache> cache_;
};

TEST_P(CatalogClientCacheTest, GetApi) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .Times(1);
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(1);

  auto catalog_client = std::make_unique<read::CatalogClient>(hrn, settings_);

  auto request = read::CatalogVersionRequest().WithStartVersion(-1);

  auto future = catalog_client->GetLatestVersion(request);
  auto catalog_version_response = future.GetFuture().get();

  ASSERT_TRUE(catalog_version_response.IsSuccessful())
      << ApiErrorToString(catalog_version_response.GetError());
}

TEST_P(CatalogClientCacheTest, GetApiInvalidJson) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .Times(1);
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(1)
      .WillRepeatedly(
          ReturnHttpResponse(GetResponse(olp::http::HttpStatusCode::OK),
                             R"jsonString({"version"4})jsonString"));

  auto catalog_client = std::make_unique<read::CatalogClient>(hrn, settings_);

  auto request = read::CatalogVersionRequest().WithStartVersion(-1);

  auto future = catalog_client->GetLatestVersion(request);
  auto catalog_version_response = future.GetFuture().get();

  ASSERT_FALSE(catalog_version_response.IsSuccessful());
  ASSERT_EQ(catalog_version_response.GetError().GetMessage(),
            "Fail parsing response.");
}

TEST_P(CatalogClientCacheTest, GetCatalog) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
      .Times(1);
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1);

  auto catalog_client = std::make_unique<read::CatalogClient>(hrn, settings_);
  auto request = read::CatalogRequest();
  auto future = catalog_client->GetCatalog(request);
  read::CatalogResponse catalog_response = future.GetFuture().get();

  ASSERT_TRUE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());

  future = catalog_client->GetCatalog(request);
  read::CatalogResponse catalog_response2 = future.GetFuture().get();

  ASSERT_TRUE(catalog_response2.IsSuccessful())
      << ApiErrorToString(catalog_response2.GetError());
  ASSERT_EQ(catalog_response2.GetResult().GetName(),
            catalog_response.GetResult().GetName());
}

TEST_P(CatalogClientCacheTest, GetCatalogUsingCatalogEndpointProvider) {
  olp::client::HRN hrn(GetTestCatalog());

  std::string service_name = "/config";
  std::string provider_url =
      "https://api-lookup.data.api.platform.here.com/lookup/v1";
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(provider_url + service_name + "/catalogs/" +
                                hrn.ToCatalogHRNString()),
                   _, _, _, _))
      .WillOnce(::testing::Return(
          olp::http::SendOutcome(olp::http::ErrorCode::SUCCESS)));

  settings_.api_lookup_settings.catalog_endpoint_provider =
      [&provider_url](const olp::client::HRN&) { return provider_url; };
  auto catalog_client = std::make_unique<read::CatalogClient>(hrn, settings_);

  auto request = read::CatalogRequest();
  auto future = catalog_client->GetCatalog(request).GetFuture();
  ASSERT_NE(future.wait_for(kWaitTimeout), std::future_status::timeout);
  future.get();
}

INSTANTIATE_TEST_SUITE_P(, CatalogClientCacheTest,
                         ::testing::Values(integration::CacheType::IN_MEMORY,
                                           integration::CacheType::DISK,
                                           integration::CacheType::BOTH,
                                           integration::CacheType::NONE));
}  // namespace
