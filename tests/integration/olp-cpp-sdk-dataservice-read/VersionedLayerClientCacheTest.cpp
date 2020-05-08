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
#include <olp/dataservice/read/CatalogRequest.h>
#include <olp/dataservice/read/CatalogVersionRequest.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/PartitionsRequest.h>
#include <olp/dataservice/read/VersionedLayerClient.h>
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

class VersionedLayerClientCacheTest : public CatalogClientTestBase {
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

TEST_P(VersionedLayerClientCacheTest, GetDataWithPartitionId) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_QUERY_PARTITION_269), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .Times(1);

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "testlayer", boost::none, settings_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithPartitionId("269");
  auto future = catalog_client->GetData(request);

  auto data_response = future.GetFuture().get();

  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_LT(0, data_response.GetResult()->size());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ("DT_2_0031", data_string);

  future = catalog_client->GetData(request);

  data_response = future.GetFuture().get();

  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_LT(0, data_response.GetResult()->size());
  std::string data_string_duplicate(data_response.GetResult()->begin(),
                                    data_response.GetResult()->end());
  ASSERT_EQ("DT_2_0031", data_string_duplicate);
}

TEST_P(VersionedLayerClientCacheTest, GetPartitionsLayerVersions) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .Times(1);

  std::string url_testlayer_res = std::regex_replace(
      URL_PARTITIONS, std::regex("testlayer"), "testlayer_res");
  std::string http_response_testlayer_res = std::regex_replace(
      HTTP_RESPONSE_PARTITIONS, std::regex("testlayer"), "testlayer_res");

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "testlayer", boost::none, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  auto future = catalog_client->GetPartitions(request);
  auto partitions_response = future.GetFuture().get();

  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(4u, partitions_response.GetResult().GetPartitions().size());
}

TEST_P(VersionedLayerClientCacheTest, GetPartitions) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .Times(1);

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "testlayer", boost::none, settings_);

  auto request = olp::dataservice::read::PartitionsRequest();
  auto future = catalog_client->GetPartitions(request);
  auto partitions_response = future.GetFuture().get();

  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(4u, partitions_response.GetResult().GetPartitions().size());

  future = catalog_client->GetPartitions(request);
  partitions_response = future.GetFuture().get();

  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(4u, partitions_response.GetResult().GetPartitions().size());
}

TEST_P(VersionedLayerClientCacheTest, GetDataWithPartitionIdDifferentVersions) {
  olp::client::HRN hrn(GetTestCatalog());

  // Test with this parameter is not relevant anymore for cache type none since
  // we cannot query 2 versions from one instance of versioned layer client and
  // we cannot share default memory created cache between 2 instances.
  if (GetParam() == CacheType::NONE) {
    return;
  }

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_API), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_QUERY_PARTITION_269), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_QUERY_PARTITION_269_V2), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_BLOB_DATA_269_V2), _, _, _, _))
      .Times(1);

  auto catalog_client_1 =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "testlayer", boost::none, settings_);

  auto catalog_client_2 =
      std::make_unique<olp::dataservice::read::VersionedLayerClient>(
          hrn, "testlayer", 2, settings_);

  auto request = olp::dataservice::read::DataRequest();
  {
    request.WithPartitionId("269");
    auto data_response = catalog_client_1->GetData(request).GetFuture().get();

    ASSERT_TRUE(data_response.IsSuccessful())
        << ApiErrorToString(data_response.GetError());
    ASSERT_LT(0, data_response.GetResult()->size());
    std::string data_string(data_response.GetResult()->begin(),
                            data_response.GetResult()->end());
    ASSERT_EQ("DT_2_0031", data_string);
  }

  {
    auto data_response = catalog_client_2->GetData(request).GetFuture().get();

    ASSERT_TRUE(data_response.IsSuccessful())
        << ApiErrorToString(data_response.GetError());
    ASSERT_LT(0, data_response.GetResult()->size());
    std::string data_string(data_response.GetResult()->begin(),
                            data_response.GetResult()->end());
    ASSERT_EQ("DT_2_0031_V2", data_string);
  }

  {
    auto data_response = catalog_client_1->GetData(request).GetFuture().get();

    ASSERT_TRUE(data_response.IsSuccessful())
        << ApiErrorToString(data_response.GetError());
    ASSERT_LT(0, data_response.GetResult()->size());
    std::string data_string(data_response.GetResult()->begin(),
                            data_response.GetResult()->end());
    ASSERT_EQ("DT_2_0031", data_string);
  }

  {
    auto data_response = catalog_client_2->GetData(request).GetFuture().get();

    ASSERT_TRUE(data_response.IsSuccessful())
        << ApiErrorToString(data_response.GetError());
    ASSERT_LT(0, data_response.GetResult()->size());
    std::string data_string(data_response.GetResult()->begin(),
                            data_response.GetResult()->end());
    ASSERT_EQ("DT_2_0031_V2", data_string);
  }
}

INSTANTIATE_TEST_SUITE_P(, VersionedLayerClientCacheTest,
                         ::testing::Values(CacheType::IN_MEMORY,
                                           CacheType::DISK, CacheType::BOTH,
                                           CacheType::NONE));
}  // namespace
