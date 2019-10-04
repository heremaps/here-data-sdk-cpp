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
#include <olp/dataservice/read/CatalogRequest.h>
#include <olp/dataservice/read/CatalogVersionRequest.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/PartitionsRequest.h>
#include "HttpResponses.h"

namespace {

using namespace olp::dataservice::read;
using namespace testing;

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
      default:
        // shouldn't get here
        break;
    }

    cache_ = std::make_shared<olp::cache::DefaultCache>(settings);
    ASSERT_EQ(olp::cache::DefaultCache::StorageOpenResult::Success,
              cache_->Open());
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
              Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
      .Times(1);
  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(2);

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_, cache_);

  auto request = CatalogVersionRequest().WithStartVersion(-1);

  auto future = catalog_client->GetCatalogMetadataVersion(request);
  auto catalog_version_response = future.GetFuture().get();

  ASSERT_TRUE(catalog_version_response.IsSuccessful())
      << ApiErrorToString(catalog_version_response.GetError());

  auto partitions_request = olp::dataservice::read::PartitionsRequest();
  partitions_request.WithLayerId("testlayer");
  auto partitions_future = catalog_client->GetPartitions(partitions_request);
  auto partitions_response = partitions_future.GetFuture().get();

  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(4u, partitions_response.GetResult().GetPartitions().size());
}

TEST_P(CatalogClientCacheTest, GetCatalog) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
      .Times(1);
  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1);

  auto catalog_client = std::make_unique<CatalogClient>(hrn, settings_, cache_);
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

TEST_P(CatalogClientCacheTest, GetDataWithPartitionId) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(2);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_QUERY), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_QUERY_PARTITION_269), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_BLOB_DATA_269), _, _, _, _))
      .Times(1);

  auto catalog_client = std::make_unique<olp::dataservice::read::CatalogClient>(
      hrn, settings_, cache_);

  auto request = olp::dataservice::read::DataRequest();
  request.WithLayerId("testlayer").WithPartitionId("269");
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

TEST_P(CatalogClientCacheTest, GetPartitionsLayerVersions) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(2);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LAYER_VERSIONS), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .Times(1);

  std::string url_testlayer_res = std::regex_replace(
      URL_PARTITIONS, std::regex("testlayer"), "testlayer_res");
  std::string http_response_testlayer_res = std::regex_replace(
      HTTP_RESPONSE_PARTITIONS, std::regex("testlayer"), "testlayer_res");

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(url_testlayer_res), _, _, _, _))
      .Times(1)
      .WillOnce(NetworkMock::ReturnHttpResponse(
          olp::http::NetworkResponse().WithStatus(200),
          http_response_testlayer_res));

  auto catalog_client = std::make_unique<olp::dataservice::read::CatalogClient>(
      hrn, settings_, cache_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer");
  auto future = catalog_client->GetPartitions(request);
  auto partitions_response = future.GetFuture().get();

  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(4u, partitions_response.GetResult().GetPartitions().size());
  request.WithLayerId("testlayer_res");

  future = catalog_client->GetPartitions(request);

  partitions_response = future.GetFuture().get();

  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(4u, partitions_response.GetResult().GetPartitions().size());
}

TEST_P(CatalogClientCacheTest, GetPartitions) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(2);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LAYER_VERSIONS), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_PARTITIONS), _, _, _, _))
      .Times(1);

  auto catalog_client = std::make_unique<olp::dataservice::read::CatalogClient>(
      hrn, settings_, cache_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer");
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

TEST_P(CatalogClientCacheTest, GetDataWithPartitionIdDifferentVersions) {
  olp::client::HRN hrn(GetTestCatalog());

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LOOKUP_METADATA), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_,
              Send(IsGetRequest(URL_LATEST_CATALOG_VERSION), _, _, _, _))
      .Times(2);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_CONFIG), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_CONFIG), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_BLOB), _, _, _, _))
      .Times(1);

  EXPECT_CALL(*network_mock_, Send(IsGetRequest(URL_LOOKUP_QUERY), _, _, _, _))
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

  auto catalog_client =
      std::make_unique<olp::dataservice::read::CatalogClient>(hrn, settings_);

  auto request = olp::dataservice::read::DataRequest();
  {
    request.WithLayerId("testlayer").WithPartitionId("269");
    auto data_response = catalog_client->GetData(request).GetFuture().get();

    ASSERT_TRUE(data_response.IsSuccessful())
        << ApiErrorToString(data_response.GetError());
    ASSERT_LT(0, data_response.GetResult()->size());
    std::string data_string(data_response.GetResult()->begin(),
                            data_response.GetResult()->end());
    ASSERT_EQ("DT_2_0031", data_string);
  }

  {
    request.WithVersion(2);
    auto data_response = catalog_client->GetData(request).GetFuture().get();

    ASSERT_TRUE(data_response.IsSuccessful())
        << ApiErrorToString(data_response.GetError());
    ASSERT_LT(0, data_response.GetResult()->size());
    std::string data_string(data_response.GetResult()->begin(),
                            data_response.GetResult()->end());
    ASSERT_EQ("DT_2_0031_V2", data_string);
  }

  {
    request.WithVersion(boost::none);
    auto data_response = catalog_client->GetData(request).GetFuture().get();

    ASSERT_TRUE(data_response.IsSuccessful())
        << ApiErrorToString(data_response.GetError());
    ASSERT_LT(0, data_response.GetResult()->size());
    std::string data_string(data_response.GetResult()->begin(),
                            data_response.GetResult()->end());
    ASSERT_EQ("DT_2_0031", data_string);
  }

  {
    request.WithVersion(2);
    auto data_response = catalog_client->GetData(request).GetFuture().get();

    ASSERT_TRUE(data_response.IsSuccessful())
        << ApiErrorToString(data_response.GetError());
    ASSERT_LT(0, data_response.GetResult()->size());
    std::string data_string(data_response.GetResult()->begin(),
                            data_response.GetResult()->end());
    ASSERT_EQ("DT_2_0031_V2", data_string);
  }
}

TEST_P(CatalogClientCacheTest, GetVolatilePartitionsExpiry) {
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
        .WillRepeatedly(NetworkMock::ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(200),
            HTTP_RESPONSE_PARTITIONS_V2));

    EXPECT_CALL(
        *network_mock_,
        Send(IsGetRequest("https://metadata.data.api.platform.here.com/"
                          "metadata/v1/catalogs/hereos-internal-test-v2/"
                          "layers/testlayer_volatile/partitions"),
             _, _, _, _))
        .Times(1)
        .WillRepeatedly(NetworkMock::ReturnHttpResponse(
            olp::http::NetworkResponse().WithStatus(200),
            HTTP_RESPONSE_EMPTY_PARTITIONS));
  }

  auto catalog_client = std::make_unique<olp::dataservice::read::CatalogClient>(
      hrn, settings_, cache_);

  auto request = olp::dataservice::read::PartitionsRequest();
  request.WithLayerId("testlayer_volatile");

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

INSTANTIATE_TEST_SUITE_P(, CatalogClientCacheTest,
                         ::testing::Values(CacheType::IN_MEMORY,
                                           CacheType::DISK, CacheType::BOTH));
}  // namespace
