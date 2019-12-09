/*
 * Copyright (C) 2019 HERE Europe B.V.
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
#include <string>

#include <gtest/gtest.h>

#include <testutils/CustomParameters.hpp>

#include <olp/authentication/AuthenticationCredentials.h>
#include <olp/authentication/Settings.h>
#include <olp/authentication/TokenEndpoint.h>
#include <olp/authentication/TokenProvider.h>
#include <olp/authentication/TokenResult.h>
#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/client/ApiError.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientFactory.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/porting/make_unique.h>

#include "ApiClientLookup.h"
#include "generated/api/BlobApi.h"
#include "generated/api/ConfigApi.h"
#include "generated/api/MetadataApi.h"
#include "generated/api/PlatformApi.h"
#include "generated/api/QueryApi.h"
#include "generated/api/VolatileBlobApi.h"

namespace {

class ApiTest : public ::testing::Test {
 protected:
  ApiTest() {
    auto network = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();

    const auto key_id =
        CustomParameters::getArgument("dataservice_read_test_appid");
    const auto secret =
        CustomParameters::getArgument("dataservice_read_test_secret");

    olp::authentication::Settings authentication_settings({key_id, secret});
    authentication_settings.network_request_handler = network;

    olp::authentication::TokenProviderDefault provider(authentication_settings);
    olp::client::AuthenticationSettings auth_client_settings;
    auth_client_settings.provider = provider;

    settings_ = std::make_shared<olp::client::OlpClientSettings>();
    settings_->authentication_settings = auth_client_settings;
    settings_->network_request_handler = network;
    settings_->cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache({});

    client_ = olp::client::OlpClientFactory::Create(*settings_);
  }
  std::string GetTestCatalog() {
    return CustomParameters::getArgument("dataservice_read_test_catalog");
  }

  static std::string ApiErrorToString(const olp::client::ApiError& error) {
    std::ostringstream resultStream;
    resultStream << "ERROR: status: " << error.GetHttpStatusCode()
                 << ", message: " << error.GetMessage();
    return resultStream.str();
  }

 protected:
  std::shared_ptr<olp::client::OlpClientSettings> settings_;
  std::shared_ptr<olp::client::OlpClient> client_;
};

TEST_F(ApiTest, GetCatalog) {
  olp::client::HRN hrn(GetTestCatalog());

  auto client_response = olp::dataservice::read::ApiClientLookup::LookupApi(
      hrn, {}, "config", "v1",
      olp::dataservice::read::FetchOptions::OnlineIfNotFound, *settings_);

  ASSERT_TRUE(client_response.IsSuccessful())
      << ApiErrorToString(client_response.GetError());
  auto config_client = client_response.GetResult();

  std::promise<olp::dataservice::read::ConfigApi::CatalogResponse>
      catalog_promise;
  auto catalog_callback =
      [&catalog_promise](
          olp::dataservice::read::ConfigApi::CatalogResponse catalog_response) {
        catalog_promise.set_value(catalog_response);
      };

  auto start_time = std::chrono::high_resolution_clock::now();
  auto catalog_response = olp::dataservice::read::ConfigApi::GetCatalog(
      config_client, GetTestCatalog(), boost::none,
      olp::client::CancellationContext{});
  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> time = end - start_time;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
  ASSERT_TRUE(catalog_response.IsSuccessful())
      << ApiErrorToString(catalog_response.GetError());
  ASSERT_EQ(GetTestCatalog(), catalog_response.GetResult().GetHrn());
}

TEST_F(ApiTest, GetPartitions) {
  olp::client::HRN hrn(GetTestCatalog());

  auto client_response = olp::dataservice::read::ApiClientLookup::LookupApi(
      hrn, {}, "metadata", "v1",
      olp::dataservice::read::FetchOptions::OnlineIfNotFound, *settings_);

  ASSERT_TRUE(client_response.IsSuccessful())
      << ApiErrorToString(client_response.GetError());
  auto metadata_client = client_response.GetResult();
  olp::client::CancellationContext context;

  auto start_time = std::chrono::high_resolution_clock::now();
  auto partitions_response = olp::dataservice::read::MetadataApi::GetPartitions(
      metadata_client, "testlayer", 1, boost::none, boost::none, boost::none,
      context);

  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> time = end - start_time;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
  ASSERT_TRUE(partitions_response.IsSuccessful())
      << ApiErrorToString(partitions_response.GetError());
  ASSERT_EQ(3u, partitions_response.GetResult().GetPartitions().size());
}

TEST_F(ApiTest, GetPartitionById) {
  olp::client::HRN hrn(GetTestCatalog());

  auto client_response = olp::dataservice::read::ApiClientLookup::LookupApi(
      hrn, {}, "query", "v1",
      olp::dataservice::read::FetchOptions::OnlineIfNotFound, *settings_);

  ASSERT_TRUE(client_response.IsSuccessful())
      << ApiErrorToString(client_response.GetError());
  auto query_client = client_response.GetResult();

  {
    SCOPED_TRACE("Test with 2 partitions");

    auto start_time = std::chrono::high_resolution_clock::now();
    std::vector<std::string> partitions = {"269", "270"};
    auto partitions_response =
        olp::dataservice::read::QueryApi::GetPartitionsbyId(
            query_client, "testlayer", partitions, 1, boost::none, boost::none,
            olp::client::CancellationContext{});
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> time = end - start_time;
    std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
    ASSERT_TRUE(partitions_response.IsSuccessful())
        << ApiErrorToString(partitions_response.GetError());
    ASSERT_EQ(2u, partitions_response.GetResult().GetPartitions().size());
    for (auto& partition : partitions_response.GetResult().GetPartitions()) {
      if (partition.GetPartition() != "269" &&
          partition.GetPartition() != "270") {
        FAIL() << "Partition IDs don't match";
      }
    }
  }

  {
    SCOPED_TRACE("Test with single partition");

    auto start_time = std::chrono::high_resolution_clock::now();
    std::vector<std::string> partitions = {"270"};
    std::vector<std::string> additional_fields = {"checksum", "dataSize"};
    auto partitions_response =
        olp::dataservice::read::QueryApi::GetPartitionsbyId(
            query_client, "testlayer", partitions, 1, additional_fields,
            boost::none, olp::client::CancellationContext{});
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> time = end - start_time;
    std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
    ASSERT_TRUE(partitions_response.IsSuccessful())
        << ApiErrorToString(partitions_response.GetError());
    ASSERT_EQ(1u, partitions_response.GetResult().GetPartitions().size());
    EXPECT_EQ(
        "270",
        partitions_response.GetResult().GetPartitions().at(0).GetPartition());
    EXPECT_TRUE(
        partitions_response.GetResult().GetPartitions().at(0).GetVersion());
    EXPECT_EQ(
        1, *partitions_response.GetResult().GetPartitions().at(0).GetVersion());
    EXPECT_FALSE(
        partitions_response.GetResult().GetPartitions().at(0).GetChecksum());
    EXPECT_FALSE(
        partitions_response.GetResult().GetPartitions().at(0).GetDataSize());
  }
}

TEST_F(ApiTest, GetCatalogVersion) {
  olp::client::HRN hrn(GetTestCatalog());

  auto client_response = olp::dataservice::read::ApiClientLookup::LookupApi(
      hrn, {}, "metadata", "v1",
      olp::dataservice::read::FetchOptions::OnlineIfNotFound, *settings_);

  ASSERT_TRUE(client_response.IsSuccessful())
      << ApiErrorToString(client_response.GetError());
  auto metadata_client = client_response.GetResult();
  olp::client::CancellationContext context;

  auto start_time = std::chrono::high_resolution_clock::now();
  auto version_response = olp::dataservice::read::MetadataApi::GetLatestCatalogVersion(
      metadata_client, -1, boost::none, context);
  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> time = end - start_time;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
  ASSERT_TRUE(version_response.IsSuccessful())
      << ApiErrorToString(version_response.GetError());
  ASSERT_LT(0, version_response.GetResult().GetVersion());
}

TEST_F(ApiTest, GetLayerVersions) {
  olp::client::HRN hrn(GetTestCatalog());

  auto client_response = olp::dataservice::read::ApiClientLookup::LookupApi(
      hrn, {}, "metadata", "v1",
      olp::dataservice::read::FetchOptions::OnlineIfNotFound, *settings_);

  ASSERT_TRUE(client_response.IsSuccessful())
      << ApiErrorToString(client_response.GetError());
  auto metadata_client = client_response.GetResult();
  olp::client::CancellationContext context;

  auto start_time = std::chrono::high_resolution_clock::now();
  auto layer_versions_response = olp::dataservice::read::MetadataApi::GetLayerVersions(
      metadata_client, 1, boost::none, context);
  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> time = end - start_time;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
  ASSERT_TRUE(layer_versions_response.IsSuccessful())
      << ApiErrorToString(layer_versions_response.GetError());
  ASSERT_EQ(1, layer_versions_response.GetResult().GetVersion());
  ASSERT_LT(0, layer_versions_response.GetResult().GetLayerVersions().size());
}

TEST_F(ApiTest, GetBlob) {
  olp::client::HRN hrn(GetTestCatalog());

  auto client_response = olp::dataservice::read::ApiClientLookup::LookupApi(
      hrn, {}, "blob", "v1",
      olp::dataservice::read::FetchOptions::OnlineIfNotFound, *settings_);

  ASSERT_TRUE(client_response.IsSuccessful())
      << ApiErrorToString(client_response.GetError());
  auto blob_client = client_response.GetResult();

  olp::client::CancellationContext context;

  auto start_time = std::chrono::high_resolution_clock::now();
  auto data_response = olp::dataservice::read::BlobApi::GetBlob(
      blob_client, "testlayer", "d5d73b64-7365-41c3-8faf-aa6ad5bab135",
      boost::none, boost::none, context);
  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> time = end - start_time;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_LT(0, data_response.GetResult()->size());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ("DT_2_0031", data_string);
}

TEST_F(ApiTest, DISABLED_GetVolatileBlob) {
  olp::client::HRN hrn(GetTestCatalog());

  auto client_response = olp::dataservice::read::ApiClientLookup::LookupApi(
      hrn, {}, "volatile-blob", "v1",
      olp::dataservice::read::FetchOptions::OnlineIfNotFound, *settings_);

  ASSERT_TRUE(client_response.IsSuccessful())
      << ApiErrorToString(client_response.GetError());
  auto volatile_blob_client = client_response.GetResult();
  auto start_time = std::chrono::high_resolution_clock::now();
  auto data_response = olp::dataservice::read::VolatileBlobApi::GetVolatileBlob(
      volatile_blob_client, "testlayer", "d5d73b64-7365-41c3-8faf-aa6ad5bab135",
      boost::none, {});
  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> time = end - start_time;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
  ASSERT_TRUE(data_response.IsSuccessful())
      << ApiErrorToString(data_response.GetError());
  ASSERT_LT(0, data_response.GetResult()->size());
  std::string data_string(data_response.GetResult()->begin(),
                          data_response.GetResult()->end());
  ASSERT_EQ("DT_2_0032", data_string);
}

TEST_F(ApiTest, QuadTreeIndex) {
  olp::client::HRN hrn(GetTestCatalog());

  auto client_response = olp::dataservice::read::ApiClientLookup::LookupApi(
      hrn, {}, "query", "v1",
      olp::dataservice::read::FetchOptions::OnlineIfNotFound, *settings_);

  ASSERT_TRUE(client_response.IsSuccessful())
      << ApiErrorToString(client_response.GetError());
  auto query_client = client_response.GetResult();

  std::string layer_id("hype-test-prefetch");
  int64_t version = 3;
  std::string quad_key("5904591");
  int32_t depth = 2;

  auto start_time = std::chrono::high_resolution_clock::now();
  auto index_response = olp::dataservice::read::QueryApi::QuadTreeIndex(
      query_client, layer_id, version, quad_key, depth, boost::none,
      boost::none, olp::client::CancellationContext{});
  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> time = end - start_time;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
  ASSERT_TRUE(index_response.IsSuccessful())
      << ApiErrorToString(index_response.GetError());
}

}  // namespace
