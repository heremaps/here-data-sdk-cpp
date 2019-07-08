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
#include <future>
#include <iostream>

#include <olp/authentication/TokenProvider.h>

#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>

#include <olp/dataservice/read/CatalogClient.h>
#include <olp/dataservice/read/CatalogRequest.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/PartitionsRequest.h>

namespace {
const std::string gKeyId("");      // your here.access.key.id
const std::string gKeySecret("");  // your here.access.key.secret
const std::string gCatalogHRN("hrn:here:data:::edge-example-catalog");
const size_t gMaxLayers(5);
const size_t gMaxPartitions(5);
}  // namespace

int main() {
  // Setup AuthenticationSettings with a default token provider that will
  // retrieve an OAuth 2.0 token from OLP.
  olp::client::AuthenticationSettings authSettings;
  authSettings.provider =
      olp::authentication::TokenProviderDefault(gKeyId, gKeySecret);

  // Setup OlpClientSettings and provide it to the CatalogClient.
  auto settings = std::make_shared<olp::client::OlpClientSettings>();
  settings->authentication_settings = authSettings;

  // Create a CatalogClient with appropriate HRN and settings.
  auto serviceClient = std::make_unique<olp::dataservice::read::CatalogClient>(
      olp::client::HRN(gCatalogHRN), settings);

  std::string firstPartitionId;
  {  // Retrieve the catalog metadata
    // Create CatalogRequest
    auto request =
        olp::dataservice::read::CatalogRequest().WithBillingTag(boost::none);

    // Run the CatalogRequest
    auto future = serviceClient->GetCatalog(request);

    // Wait for the CatalogResponse response
    olp::dataservice::read::CatalogResponse catalogResponse =
        future.GetFuture().get();

    // Check the response
    if (catalogResponse.IsSuccessful()) {
      const olp::dataservice::read::CatalogResult& responseResult =
          catalogResponse.GetResult();
      LOG_INFO_F("read-example", "Catalog description: %s",
                 responseResult.GetDescription().c_str());
      auto layers = catalogResponse.GetResult().GetLayers();
      if (!layers.empty()) {
        firstPartitionId = layers.front().GetId();
      }
      auto end = layers.size() <= gMaxLayers ? layers.end()
                                             : layers.begin() + gMaxLayers;
      for (auto it = layers.cbegin(); it != end; ++it) {
        LOG_INFO_F("read-example", "Layer '%s' (%s): %s", it->GetId().c_str(),
                   it->GetLayerType().c_str(), it->GetDescription().c_str());
      }
    } else {
      LOG_ERROR("read-example", "Request catalog metadata - Failure");
    }
  }

  std::string firstLayerId;
  if (!firstPartitionId.empty()) {
    // Retrieve the partitions metadata
    // Create a PartitionsRequest with appropriate LayerId
    auto request = olp::dataservice::read::PartitionsRequest()
                       .WithLayerId(firstPartitionId)
                       .WithBillingTag(boost::none);

    // Run the PartitionsRequest
    auto future = serviceClient->GetPartitions(request);

    // Wait for PartitionsResponse
    olp::dataservice::read::PartitionsResponse partitionsResponse =
        future.GetFuture().get();

    // Check the response
    if (partitionsResponse.IsSuccessful()) {
      const olp::dataservice::read::PartitionsResult& responseResult =
          partitionsResponse.GetResult();
      auto partitions = responseResult.GetPartitions();
      LOG_INFO_F("read-example", "Layer contains %ld partitions.",
                 partitions.size());
      if (!partitions.empty()) {
        firstLayerId = partitions.front().GetPartition();
      }
      auto end = partitions.size() <= gMaxPartitions
                     ? partitions.end()
                     : partitions.begin() + gMaxPartitions;
      for (auto it = partitions.cbegin(); it != end; ++it) {
        LOG_INFO_F("read-example", "Partition: %s", it->GetPartition().c_str());
      }
    } else {
      LOG_ERROR("read-example", "Request partition metadata - Failure");
    }
  }

  if (!firstLayerId.empty()) {
    // Retrieve the partition data
    // Create a DataRequest with appropriate LayerId and PartitionId
    auto request = olp::dataservice::read::DataRequest()
                       .WithLayerId(firstPartitionId)
                       .WithPartitionId(firstLayerId)
                       .WithBillingTag(boost::none);

    // Run the DataRequest
    auto future = serviceClient->GetData(request);

    // Wait for DataResponse
    olp::dataservice::read::DataResponse dataResponse =
        future.GetFuture().get();

    // Check the response
    if (dataResponse.IsSuccessful()) {
      const olp::dataservice::read::DataResult& responseResult =
          dataResponse.GetResult();
      LOG_INFO_F("read-example",
                 "Request partition data - Success, data size - %ld",
                 responseResult->size());
    } else {
      LOG_ERROR("read-example", "Request partition data - Failure");
    }
  }

  return 0;
}
