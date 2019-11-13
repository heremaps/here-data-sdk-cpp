#include "CatalogClientTestBase.h"

#include <regex>

#include <gmock/gmock.h>
#include <matchers/NetworkUrlMatchers.h>
#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/DefaultCache.h>
#include <olp/core/client/HRN.h>
#include <olp/core/porting/make_unique.h>
#include <olp/core/utils/Dir.h>
#include <olp/dataservice/read/VolatileLayerClient.h>
#include <olp/dataservice/read/CatalogRequest.h>
#include <olp/dataservice/read/CatalogVersionRequest.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/PartitionsRequest.h>
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

class VolatileLayerClientCacheTest : public CatalogClientTestBase {
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
        settings.disk_path =
            olp::utils::Dir::TempDirectory() + kClientTestCacheDir;
        ClearCache(settings.disk_path.get());
        break;
      }
      case CacheType::BOTH: {
        settings.disk_path =
            olp::utils::Dir::TempDirectory() + kClientTestCacheDir;
        ClearCache(settings.disk_path.get());
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

TEST_P(VolatileLayerClientCacheTest, GetVolatilePartitionsExpiry) {
  olp::client::HRN hrn(GetTestCatalog());

  {
    testing::InSequence s;

    EXPECT_CALL(
        *network_mock_,
        Send(IsGetRequest("https://metadata.data.api.platform.here.com/"
                          "metadata/v1/catalogs/hereos-internal-test-v2/"
                          "layers/testlayer_volatile/partitions"),
             _, _, _, _))
        .Times(1)
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                               HTTP_RESPONSE_PARTITIONS_V2));

    EXPECT_CALL(
        *network_mock_,
        Send(IsGetRequest("https://metadata.data.api.platform.here.com/"
                          "metadata/v1/catalogs/hereos-internal-test-v2/"
                          "layers/testlayer_volatile/partitions"),
             _, _, _, _))
        .Times(1)
        .WillRepeatedly(
            ReturnHttpResponse(olp::http::NetworkResponse().WithStatus(200),
                               HTTP_RESPONSE_EMPTY_PARTITIONS));
  }

  auto catalog_client =
      std::make_unique<olp::dataservice::read::VolatileLayerClient>(hrn, "testlayer_volatile", settings_);

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
  std::this_thread::sleep_for(std::chrono::seconds(2));
  request.WithFetchOption(olp::dataservice::read::OnlineIfNotFound);
  future = catalog_client->GetPartitions(request);
  partitions_response = future.GetFuture().get();

  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(0u, partitions_response.GetResult().GetPartitions().size());
}

INSTANTIATE_TEST_SUITE_P(, VolatileLayerClientCacheTest,
                         ::testing::Values(CacheType::IN_MEMORY,
                                           CacheType::DISK, CacheType::BOTH,
                                           CacheType::NONE));
}  // namespace
