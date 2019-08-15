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
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/logging/Log.h>
#include <olp/core/porting/make_unique.h>

#include <olp/dataservice/read/CatalogClient.h>
#include <olp/dataservice/read/CatalogRequest.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/PartitionsRequest.h>

namespace {
const std::string kKeyId("");      // your here.access.key.id
const std::string kKeySecret("");  // your here.access.key.secret
const std::string kCatalogHRN("hrn:here:data:::edge-example-catalog");
constexpr size_t kMaxLayers(5);
constexpr size_t kMaxPartitions(5);
constexpr auto kLogTag = "read-example";

// handles catalog response and returns a partition ID from it to be processed
// in later requests
std::string HandleCatalogResponse(
    const olp::dataservice::read::CatalogResponse& catalog_response) {
  std::string first_layer_id;
  if (catalog_response.IsSuccessful()) {
    const auto& response_result = catalog_response.GetResult();
    EDGE_SDK_LOG_INFO_F(kLogTag, "Catalog description: %s",
                        response_result.GetDescription().c_str());

    auto layers = response_result.GetLayers();
    if (!layers.empty()) {
      first_layer_id = layers.front().GetId();
    }

    auto end = layers.size() <= kMaxLayers ? layers.end()
                                           : layers.begin() + kMaxLayers;
    for (auto it = layers.cbegin(); it != end; ++it) {
      EDGE_SDK_LOG_INFO_F(kLogTag, "Layer '%s' (%s): %s", it->GetId().c_str(),
                          it->GetLayerType().c_str(),
                          it->GetDescription().c_str());
    }
  } else {
    EDGE_SDK_LOG_ERROR_F(
        kLogTag, "Request catalog metadata - Failure(%d): %s",
        static_cast<int>(catalog_response.GetError().GetErrorCode()),
        catalog_response.GetError().GetMessage().c_str());
  }
  return first_layer_id;
}

// handles partitions response and returns a layer ID from it to be processed
// in later requests
std::string HandlePartitionsResponse(
    const olp::dataservice::read::PartitionsResponse& partitions_response) {
  std::string first_partition_id;
  if (partitions_response.IsSuccessful()) {
    const auto& response_result = partitions_response.GetResult();
    auto partitions = response_result.GetPartitions();
    EDGE_SDK_LOG_INFO_F(kLogTag, "Layer contains %ld partitions.",
                        partitions.size());

    if (!partitions.empty()) {
      first_partition_id = partitions.front().GetPartition();
    }

    auto end = partitions.size() <= kMaxPartitions
                   ? partitions.end()
                   : partitions.begin() + kMaxPartitions;
    for (auto it = partitions.cbegin(); it != end; ++it) {
      EDGE_SDK_LOG_INFO_F(kLogTag, "Partition: %s", it->GetPartition().c_str());
    }
  } else {
    EDGE_SDK_LOG_ERROR_F(
        kLogTag, "Request partition metadata - Failure(%d): %s",
        static_cast<int>(partitions_response.GetError().GetErrorCode()),
        partitions_response.GetError().GetMessage().c_str());
  }
  return first_partition_id;
}

// handles data response
bool HandleDataResponse(
    const olp::dataservice::read::DataResponse& data_response) {
  if (data_response.IsSuccessful()) {
    const auto& response_result = data_response.GetResult();
    EDGE_SDK_LOG_INFO_F(kLogTag,
                        "Request partition data - Success, data size - %ld",
                        response_result->size());
    return true;
  } else {
    EDGE_SDK_LOG_ERROR_F(
        kLogTag, "Request partition data - Failure(%d): %s",
        static_cast<int>(data_response.GetError().GetErrorCode()),
        data_response.GetError().GetMessage().c_str());
    return false;
  }
}
}  // namespace

int RunExample() {
  // Setup AuthenticationSettings with a default token provider that will
  // retrieve an OAuth 2.0 token from OLP.
  olp::client::AuthenticationSettings auth_settings;
  auth_settings.provider =
      olp::authentication::TokenProviderDefault(kKeyId, kKeySecret);

  // Setup OlpClientSettings and provide it to the CatalogClient.
  auto settings = std::make_shared<olp::client::OlpClientSettings>();
  settings->authentication_settings = auth_settings;
  settings->task_scheduler = std::move(
      olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1u));

  // Create a CatalogClient with appropriate HRN and settings.
  auto service_client = std::make_unique<olp::dataservice::read::CatalogClient>(
      olp::client::HRN(kCatalogHRN), settings);

  std::string first_layer_id;
  {  // Retrieve the catalog metadata
    // Create CatalogRequest
    auto request =
        olp::dataservice::read::CatalogRequest().WithBillingTag(boost::none);

    // Run the CatalogRequest
    auto future = service_client->GetCatalog(request);

    // Wait for the CatalogResponse response
    olp::dataservice::read::CatalogResponse catalog_response =
        future.GetFuture().get();

    // Retrieve data from the response
    first_layer_id = HandleCatalogResponse(catalog_response);
  }

  std::string first_partition_id;
  if (!first_layer_id.empty()) {
    // Retrieve the partitions metadata
    // Create a PartitionsRequest with appropriate LayerId
    auto request = olp::dataservice::read::PartitionsRequest()
                       .WithLayerId(first_layer_id)
                       .WithBillingTag(boost::none);

    // Run the PartitionsRequest
    auto future = service_client->GetPartitions(request);

    // Wait for PartitionsResponse
    olp::dataservice::read::PartitionsResponse partitions_response =
        future.GetFuture().get();

    // Retrieve data from the response
    first_partition_id = HandlePartitionsResponse(partitions_response);
  } else {
    EDGE_SDK_LOG_WARNING(kLogTag, "Request partition metadata is not present!");
  }

  int return_code = -1;
  if (!first_partition_id.empty()) {
    // Retrieve the partition data
    // Create a DataRequest with appropriate LayerId and PartitionId
    auto request = olp::dataservice::read::DataRequest()
                       .WithLayerId(first_layer_id)
                       .WithPartitionId(first_partition_id)
                       .WithBillingTag(boost::none);

    // Run the DataRequest
    auto future = service_client->GetData(request);

    // Wait for DataResponse
    olp::dataservice::read::DataResponse data_response =
        future.GetFuture().get();

    // Retrieve data from the response
    return_code = HandleDataResponse(data_response) ? 0 : -1;
  } else {
    EDGE_SDK_LOG_WARNING(kLogTag, "Request partition data is not present!");
  }

  return return_code;
}
