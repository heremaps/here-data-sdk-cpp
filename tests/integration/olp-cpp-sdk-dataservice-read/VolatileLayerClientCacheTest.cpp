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
#include <thread>

#include <gmock/gmock.h>
#include <matchers/NetworkUrlMatchers.h>
#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/DefaultCache.h>
#include <olp/core/client/HRN.h>
#include <olp/core/http/HttpStatusCode.h>
#include <olp/core/porting/make_unique.h>
#include <olp/core/utils/Dir.h>
#include <olp/dataservice/read/CatalogRequest.h>
#include <olp/dataservice/read/CatalogVersionRequest.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/PartitionsRequest.h>
#include <olp/dataservice/read/VolatileLayerClient.h>
#include "HttpResponses.h"

namespace {
using testing::_;
namespace integration = olp::tests::integration;
namespace http = olp::http;

#ifdef _WIN32
constexpr auto kClientTestDir = "\\catalog_client_test";
constexpr auto kClientTestCacheDir = "\\catalog_client_test\\cache";
#else
constexpr auto kClientTestDir = "/catalog_client_test";
constexpr auto kClientTestCacheDir = "/cata.log_client_test/cache";
#endif

class VolatileLayerClientCacheTest : public integration::CatalogClientTestBase {
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
        ClearCache(*settings.disk_path_mutable);
        break;
      }
      case integration::CacheType::BOTH: {
        settings.disk_path_mutable =
            olp::utils::Dir::TempDirectory() + kClientTestCacheDir;
        ClearCache(*settings.disk_path_mutable);
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

TEST_P(VolatileLayerClientCacheTest, GetVolatilePartitionsExpiry) {
  olp::client::HRN hrn(GetTestCatalog());

  {
    EXPECT_CALL(
        *network_mock_,
        Send(IsGetRequest("https://metadata.data.api.platform.here.com/"
                          "metadata/v1/catalogs/hereos-internal-test-v2/"
                          "layers/testlayer_volatile/partitions"),
             _, _, _, _))
        .Times(1)
        .WillRepeatedly(ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(http::HttpStatusCode::OK),
            HTTP_RESPONSE_PARTITIONS_V2));
  }

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VolatileLayerClient>(
          hrn, "testlayer_volatile", settings_);

  auto request = olp::dataservice::read::PartitionsRequest();

  auto future = catalog_client->GetPartitions(request);

  auto partitions_response = future.GetFuture().get();

  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(1u, partitions_response.GetResult().GetPartitions().size());

  // hit the cache only, should be still be there
  request.WithFetchOption(olp::dataservice::read::CacheOnly);
  future = catalog_client->GetPartitions(request);
  partitions_response = future.GetFuture().get();
  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(1u, partitions_response.GetResult().GetPartitions().size());

  // wait for the layer to expire in cache
  bool expired = false;
  // Expiriation time in that HTTP_RESPONSE_CONFIG + 2 seconds
  const auto timeout = std::chrono::seconds(4);
  const auto end_time = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < end_time) {
    request.WithFetchOption(olp::dataservice::read::CacheOnly);
    future = catalog_client->GetPartitions(request);
    partitions_response = future.GetFuture().get();
    if (!partitions_response.IsSuccessful()) {
      expired = true;
      break;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  ASSERT_TRUE(expired);
}

INSTANTIATE_TEST_SUITE_P(, VolatileLayerClientCacheTest,
                         ::testing::Values(integration::CacheType::IN_MEMORY,
                                           integration::CacheType::DISK,
                                           integration::CacheType::BOTH,
                                           integration::CacheType::NONE));
}  // namespace
