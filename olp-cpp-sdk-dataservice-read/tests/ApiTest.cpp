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
#include <olp/authentication/AuthenticationCredentials.h>
#include <olp/authentication/Settings.h>
#include <olp/authentication/TokenEndpoint.h>
#include <olp/authentication/TokenProvider.h>
#include <olp/authentication/TokenResult.h>
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
#include "testutils/CustomParameters.hpp"

// Mark tests with need for connectivity as TestOnline, this is just for
// convenience of filtering
class ApiTest : public ::testing::TestWithParam<bool> {
 protected:
  ApiTest() {
    auto network = olp::client::OlpClientSettingsFactory::
        CreateDefaultNetworkRequestHandler();

    olp::authentication::Settings authentication_settings;
    authentication_settings.network_request_handler = network;

    olp::authentication::TokenProviderDefault provider(
        CustomParameters::getArgument("appid"),
        CustomParameters::getArgument("secret"), authentication_settings);
    olp::client::AuthenticationSettings auth_client_settings;
    auth_client_settings.provider = provider;

    settings_ = std::make_shared<olp::client::OlpClientSettings>();
    settings_->authentication_settings = auth_client_settings;
    settings_->network_request_handler = network;

    client_ = olp::client::OlpClientFactory::Create(*settings_);
  }

  std::string GetTestCatalog() {
    return CustomParameters::getArgument("catalog");
  }

  std::string PrintError(const olp::client::ApiError& error) {
    std::ostringstream resultStream;
    resultStream << "ERROR: status: " << error.GetHttpStatusCode()
                 << ", message: " << error.GetMessage();
    return resultStream.str();
  }

 protected:
  std::shared_ptr<olp::client::OlpClientSettings> settings_;
  std::shared_ptr<olp::client::OlpClient> client_;
};
INSTANTIATE_TEST_SUITE_P(TestOnline, ApiTest, ::testing::Values(true));

TEST_P(ApiTest, GetCatalog) {
  olp::client::HRN hrn(GetTestCatalog());

  std::promise<olp::dataservice::read::ApiClientLookup::ApiClientResponse>
      configClientPromise;
  olp::dataservice::read::ApiClientLookup::LookupApiClient(
      olp::client::OlpClientFactory::Create(*settings_), "config", "v1", hrn,
      [&configClientPromise](
          olp::dataservice::read::ApiClientLookup::ApiClientResponse response) {
        configClientPromise.set_value(response);
      });

  auto clientResponse = configClientPromise.get_future().get();
  ASSERT_TRUE(clientResponse.IsSuccessful())
      << PrintError(clientResponse.GetError());
  auto configClient = clientResponse.GetResult();

  std::promise<olp::dataservice::read::ConfigApi::CatalogResponse>
      catalogPromise;
  auto catalogCallback =
      [&catalogPromise](
          olp::dataservice::read::ConfigApi::CatalogResponse catalogResponse) {
        catalogPromise.set_value(catalogResponse);
      };

  auto startTime = std::chrono::high_resolution_clock::now();
  auto cancelFn = olp::dataservice::read::ConfigApi::GetCatalog(
      configClient, GetTestCatalog(), boost::none, catalogCallback);
  auto catalogResponse = catalogPromise.get_future().get();
  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> time = end - startTime;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
  ASSERT_TRUE(catalogResponse.IsSuccessful())
      << PrintError(catalogResponse.GetError());
  ASSERT_EQ(GetTestCatalog(), catalogResponse.GetResult().GetHrn());
}

TEST_P(ApiTest, GetPartitions) {
  olp::client::HRN hrn(GetTestCatalog());

  std::promise<olp::dataservice::read::ApiClientLookup::ApiClientResponse>
      metadataClientPromise;
  olp::dataservice::read::ApiClientLookup::LookupApiClient(
      olp::client::OlpClientFactory::Create(*settings_), "metadata", "v1", hrn,
      [&metadataClientPromise](
          olp::dataservice::read::ApiClientLookup::ApiClientResponse response) {
        metadataClientPromise.set_value(response);
      });

  auto clientResponse = metadataClientPromise.get_future().get();
  ASSERT_TRUE(clientResponse.IsSuccessful())
      << PrintError(clientResponse.GetError());
  auto metadataClient = clientResponse.GetResult();

  std::promise<olp::dataservice::read::MetadataApi::PartitionsResponse>
      partitionsPromise;
  auto partitionsCallback =
      [&partitionsPromise](
          olp::dataservice::read::MetadataApi::PartitionsResponse
              partitionsResponse) {
        partitionsPromise.set_value(partitionsResponse);
      };

  auto startTime = std::chrono::high_resolution_clock::now();
  olp::dataservice::read::MetadataApi::GetPartitions(
      metadataClient, "testlayer", 1, boost::none, boost::none, boost::none,
      partitionsCallback);
  auto partitionsResponse = partitionsPromise.get_future().get();
  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> time = end - startTime;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
  ASSERT_TRUE(partitionsResponse.IsSuccessful())
      << PrintError(partitionsResponse.GetError());
  ASSERT_EQ(3u, partitionsResponse.GetResult().GetPartitions().size());
}

TEST_P(ApiTest, GetPartitionById) {
  olp::client::HRN hrn(GetTestCatalog());

  std::promise<olp::dataservice::read::ApiClientLookup::ApiClientResponse>
      metadataClientPromise;
  olp::dataservice::read::ApiClientLookup::LookupApiClient(
      olp::client::OlpClientFactory::Create(*settings_), "query", "v1", hrn,
      [&metadataClientPromise](
          olp::dataservice::read::ApiClientLookup::ApiClientResponse response) {
        metadataClientPromise.set_value(response);
      });

  auto clientResponse = metadataClientPromise.get_future().get();
  ASSERT_TRUE(clientResponse.IsSuccessful())
      << PrintError(clientResponse.GetError());
  auto queryClient = clientResponse.GetResult();

  // two partitions
  {
    std::promise<olp::dataservice::read::MetadataApi::PartitionsResponse>
        partitionsPromise;
    auto partitionsCallback =
        [&partitionsPromise](
            olp::dataservice::read::MetadataApi::PartitionsResponse
                partitionsResponse) {
          partitionsPromise.set_value(partitionsResponse);
        };

    auto startTime = std::chrono::high_resolution_clock::now();
    std::vector<std::string> partitions = {"269", "270"};
    olp::dataservice::read::QueryApi::GetPartitionsbyId(
        queryClient, "testlayer", partitions, 1, boost::none, boost::none,
        partitionsCallback);
    auto partitionsResponse = partitionsPromise.get_future().get();
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> time = end - startTime;
    std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
    ASSERT_TRUE(partitionsResponse.IsSuccessful())
        << PrintError(partitionsResponse.GetError());
    ASSERT_EQ(2u, partitionsResponse.GetResult().GetPartitions().size());
    for (auto& partition : partitionsResponse.GetResult().GetPartitions()) {
      if (partition.GetPartition() != "269" &&
          partition.GetPartition() != "270") {
        ASSERT_TRUE(false);
      }
    }
  }

  // single partition
  {
    std::promise<olp::dataservice::read::MetadataApi::PartitionsResponse>
        partitionsPromise;
    auto partitionsCallback =
        [&partitionsPromise](
            olp::dataservice::read::MetadataApi::PartitionsResponse
                partitionsResponse) {
          partitionsPromise.set_value(partitionsResponse);
        };

    auto startTime = std::chrono::high_resolution_clock::now();
    std::vector<std::string> partitions = {"270"};
    std::vector<std::string> additionalFields = {"checksum", "dataSize"};
    olp::dataservice::read::QueryApi::GetPartitionsbyId(
        queryClient, "testlayer", partitions, 1, additionalFields, boost::none,
        partitionsCallback);
    auto partitionsResponse = partitionsPromise.get_future().get();
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> time = end - startTime;
    std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
    ASSERT_TRUE(partitionsResponse.IsSuccessful())
        << PrintError(partitionsResponse.GetError());
    ASSERT_EQ(1u, partitionsResponse.GetResult().GetPartitions().size());
    ASSERT_EQ(
        "270",
        partitionsResponse.GetResult().GetPartitions().at(0).GetPartition());
    ASSERT_TRUE(
        partitionsResponse.GetResult().GetPartitions().at(0).GetVersion());
    ASSERT_EQ(
        1, *partitionsResponse.GetResult().GetPartitions().at(0).GetVersion());
    ASSERT_FALSE(
        partitionsResponse.GetResult().GetPartitions().at(0).GetChecksum());
    ASSERT_FALSE(
        partitionsResponse.GetResult().GetPartitions().at(0).GetDataSize());
  }
}

TEST_P(ApiTest, GetCatalogVersion) {
  olp::client::HRN hrn(GetTestCatalog());

  std::promise<olp::dataservice::read::ApiClientLookup::ApiClientResponse>
      metadataClientPromise;
  olp::dataservice::read::ApiClientLookup::LookupApiClient(
      olp::client::OlpClientFactory::Create(*settings_), "metadata", "v1", hrn,
      [&metadataClientPromise](
          olp::dataservice::read::ApiClientLookup::ApiClientResponse response) {
        metadataClientPromise.set_value(response);
      });

  auto clientResponse = metadataClientPromise.get_future().get();
  ASSERT_TRUE(clientResponse.IsSuccessful())
      << PrintError(clientResponse.GetError());
  auto metadataClient = clientResponse.GetResult();

  std::promise<olp::dataservice::read::MetadataApi::CatalogVersionResponse>
      versionResponsePromise;
  auto versionResponseCallback =
      [&versionResponsePromise](
          olp::dataservice::read::MetadataApi::CatalogVersionResponse
              partitionsResponse) {
        versionResponsePromise.set_value(partitionsResponse);
      };

  auto startTime = std::chrono::high_resolution_clock::now();
  olp::dataservice::read::MetadataApi::GetLatestCatalogVersion(
      metadataClient, -1, boost::none, versionResponseCallback);
  auto versionResponse = versionResponsePromise.get_future().get();
  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> time = end - startTime;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
  ASSERT_TRUE(versionResponse.IsSuccessful())
      << PrintError(versionResponse.GetError());
  ASSERT_LT(0, versionResponse.GetResult().GetVersion());
}

TEST_P(ApiTest, GetLayerVersions) {
  olp::client::HRN hrn(GetTestCatalog());

  std::promise<olp::dataservice::read::ApiClientLookup::ApiClientResponse>
      metadataClientPromise;
  olp::dataservice::read::ApiClientLookup::LookupApiClient(
      olp::client::OlpClientFactory::Create(*settings_), "metadata", "v1", hrn,
      [&metadataClientPromise](
          olp::dataservice::read::ApiClientLookup::ApiClientResponse response) {
        metadataClientPromise.set_value(response);
      });

  auto clientResponse = metadataClientPromise.get_future().get();
  ASSERT_TRUE(clientResponse.IsSuccessful())
      << PrintError(clientResponse.GetError());
  auto metadataClient = clientResponse.GetResult();

  std::promise<olp::dataservice::read::MetadataApi::LayerVersionsResponse>
      layerVersionsPromise;
  auto layerVersionsCallback =
      [&layerVersionsPromise](
          olp::dataservice::read::MetadataApi::LayerVersionsResponse
              partitionsResponse) {
        layerVersionsPromise.set_value(partitionsResponse);
      };

  auto startTime = std::chrono::high_resolution_clock::now();
  olp::dataservice::read::MetadataApi::GetLayerVersions(
      metadataClient, 1, boost::none, layerVersionsCallback);
  auto layerVersionsResponse = layerVersionsPromise.get_future().get();
  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> time = end - startTime;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
  ASSERT_TRUE(layerVersionsResponse.IsSuccessful())
      << PrintError(layerVersionsResponse.GetError());
  ASSERT_EQ(1, layerVersionsResponse.GetResult().GetVersion());
  ASSERT_LT(0, layerVersionsResponse.GetResult().GetLayerVersions().size());
}

TEST_P(ApiTest, GetBlob) {
  olp::client::HRN hrn(GetTestCatalog());

  std::promise<olp::dataservice::read::ApiClientLookup::ApiClientResponse>
      bloblClientPromise;
  olp::dataservice::read::ApiClientLookup::LookupApiClient(
      olp::client::OlpClientFactory::Create(*settings_), "blob", "v1", hrn,
      [&bloblClientPromise](
          olp::dataservice::read::ApiClientLookup::ApiClientResponse response) {
        bloblClientPromise.set_value(response);
      });

  auto clientResponse = bloblClientPromise.get_future().get();
  ASSERT_TRUE(clientResponse.IsSuccessful())
      << PrintError(clientResponse.GetError());
  auto blobClient = clientResponse.GetResult();

  std::promise<olp::dataservice::read::BlobApi::DataResponse> dataPromise;
  auto dataCallback =
      [&dataPromise](
          olp::dataservice::read::BlobApi::DataResponse dataResponse) {
        dataPromise.set_value(dataResponse);
      };

  auto startTime = std::chrono::high_resolution_clock::now();
  olp::dataservice::read::BlobApi::GetBlob(
      blobClient, "testlayer", "d5d73b64-7365-41c3-8faf-aa6ad5bab135",
      boost::none, boost::none, dataCallback);
  auto dataResponse = dataPromise.get_future().get();
  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> time = end - startTime;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
  ASSERT_TRUE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_LT(0, dataResponse.GetResult()->size());
  std::string dataStr(dataResponse.GetResult()->begin(),
                      dataResponse.GetResult()->end());
  ASSERT_EQ("DT_2_0031", dataStr);
}

TEST_P(ApiTest, DISABLED_GetVolatileBlob) {
  olp::client::HRN hrn(GetTestCatalog());

  std::promise<olp::dataservice::read::ApiClientLookup::ApiClientResponse>
      volatileBloblClientPromise;
  olp::dataservice::read::ApiClientLookup::LookupApiClient(
      olp::client::OlpClientFactory::Create(*settings_), "volatile-blob", "v1",
      hrn,
      [&volatileBloblClientPromise](
          olp::dataservice::read::ApiClientLookup::ApiClientResponse response) {
        volatileBloblClientPromise.set_value(response);
      });

  auto clientResponse = volatileBloblClientPromise.get_future().get();
  ASSERT_TRUE(clientResponse.IsSuccessful())
      << PrintError(clientResponse.GetError());
  auto volatileBlobClient = clientResponse.GetResult();

  std::promise<olp::dataservice::read::BlobApi::DataResponse> dataPromise;
  auto dataCallback =
      [&dataPromise](
          olp::dataservice::read::BlobApi::DataResponse dataResponse) {
        dataPromise.set_value(dataResponse);
      };

  auto startTime = std::chrono::high_resolution_clock::now();
  olp::dataservice::read::VolatileBlobApi::GetVolatileBlob(
      volatileBlobClient, "testlayer", "d5d73b64-7365-41c3-8faf-aa6ad5bab135",
      boost::none, dataCallback);
  auto dataResponse = dataPromise.get_future().get();
  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> time = end - startTime;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
  ASSERT_TRUE(dataResponse.IsSuccessful())
      << PrintError(dataResponse.GetError());
  ASSERT_LT(0, dataResponse.GetResult()->size());
  std::string dataStr(dataResponse.GetResult()->begin(),
                      dataResponse.GetResult()->end());
  ASSERT_EQ("DT_2_0032", dataStr);
}

TEST_P(ApiTest, QuadTreeIndex) {
  olp::client::HRN hrn(GetTestCatalog());

  // get the query client
  std::promise<olp::dataservice::read::ApiClientLookup::ApiClientResponse>
      queryClientPromise;
  olp::dataservice::read::ApiClientLookup::LookupApiClient(
      olp::client::OlpClientFactory::Create(*settings_), "query", "v1", hrn,
      [&queryClientPromise](
          olp::dataservice::read::ApiClientLookup::ApiClientResponse response) {
        queryClientPromise.set_value(response);
      });

  auto clientResponse = queryClientPromise.get_future().get();
  ASSERT_TRUE(clientResponse.IsSuccessful())
      << PrintError(clientResponse.GetError());
  auto queryClient = clientResponse.GetResult();

  std::string layerId("hype-test-prefetch");
  int64_t version = 3;
  std::string quadKey("5904591");
  int32_t depth = 2;

  std::promise<olp::dataservice::read::QueryApi::QuadTreeIndexResponse>
      indexPromise;
  auto quadTreeIndexCallback =
      [&indexPromise](
          olp::dataservice::read::QueryApi::QuadTreeIndexResponse response) {
        indexPromise.set_value(response);
      };

  auto startTime = std::chrono::high_resolution_clock::now();
  auto cancelFn = olp::dataservice::read::QueryApi::QuadTreeIndex(
      queryClient, layerId, version, quadKey, depth, boost::none, boost::none,
      quadTreeIndexCallback);
  auto indexResponse = indexPromise.get_future().get();
  auto end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> time = end - startTime;
  std::cout << "duration: " << time.count() * 1000000 << " us" << std::endl;
  ASSERT_TRUE(indexResponse.IsSuccessful())
      << PrintError(indexResponse.GetError());
}
