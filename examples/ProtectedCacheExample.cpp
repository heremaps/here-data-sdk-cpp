/*
 * Copyright (C) 2020 HERE Europe B.V.
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
#include "ProtectedCacheExample.h"

#include <olp/authentication/TokenProvider.h>
#include <olp/core/cache/CacheSettings.h>
#include <olp/core/cache/DefaultCache.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/logging/Log.h>
#include <olp/core/utils/Dir.h>
#include <olp/dataservice/read/VersionedLayerClient.h>

#include <future>
#include <iostream>

namespace {

constexpr auto kLogTag = "protected-cache-example";
#ifdef _WIN32
constexpr auto kClientCacheDir = "\\catalog_client_example\\cache";
#else
constexpr auto kClientCacheDir = "/cata.log_client_example/cache";
#endif

std::string first_layer_id("versioned-world-layer");
std::string first_partition_id("1");

// handles data response
bool HandleDataResponse(
    const olp::dataservice::read::DataResponse& data_response) {
  if (data_response.IsSuccessful()) {
    const auto& response_result = data_response.GetResult();
    OLP_SDK_LOG_INFO_F(kLogTag,
                       "Request partition data - Success, data size - %ld",
                       response_result->size());
    return true;
  } else {
    OLP_SDK_LOG_ERROR_F(
        kLogTag, "Request partition data - Failure(%d): %s",
        static_cast<int>(data_response.GetError().GetErrorCode()),
        data_response.GetError().GetMessage().c_str());
    return false;
  }
}
}  // namespace

int RunExampleReadWithCache(const AccessKey& access_key,
                            const olp::cache::CacheSettings& cache_settings, const std::string& catalog) {
  OLP_SDK_LOG_INFO_F(
      kLogTag, "Mutable cache path is \"%s\"",
      cache_settings.disk_path_mutable.get_value_or("none").c_str());
  OLP_SDK_LOG_INFO_F(
      kLogTag, "Protected cache path is \"%s\"",
      cache_settings.disk_path_protected.get_value_or("none").c_str());
  // Create a task scheduler instance
  std::shared_ptr<olp::thread::TaskScheduler> task_scheduler =
      olp::client::OlpClientSettingsFactory::CreateDefaultTaskScheduler(1u);

  // Create a network client
  std::shared_ptr<olp::http::Network> http_client = olp::client::
      OlpClientSettingsFactory::CreateDefaultNetworkRequestHandler();

  // Get your access credentials from the **credentials.properties** file that
  // you can download from the Open Location Platform (OLP)
  // [website](https://developer.here.com/olp/documentation/access-control/user-guide/topics/get-credentials.html).
  const auto read_credentials_result =
      olp::authentication::AuthenticationCredentials::ReadFromFile();

  // Initialize authentication settings.
  olp::authentication::Settings settings{
      read_credentials_result.get_value_or({access_key.id, access_key.secret})};
  settings.task_scheduler = task_scheduler;
  settings.network_request_handler = http_client;

  // Setup AuthenticationSettings with a default token provider that will
  // retrieve an OAuth 2.0 token from OLP.
  olp::client::AuthenticationSettings auth_settings;
  auth_settings.provider =
      olp::authentication::TokenProviderDefault(std::move(settings));

  // Create and initialize cache
  auto cache = std::make_shared<olp::cache::DefaultCache>(cache_settings);
  cache->Open();

  // Setup OlpClientSettings and provide it to the CatalogClient.
  olp::client::OlpClientSettings client_settings;
  client_settings.authentication_settings = auth_settings;
  client_settings.task_scheduler = std::move(task_scheduler);
  client_settings.network_request_handler = std::move(http_client);
  client_settings.cache = cache;

  // Create appropriate layer client with HRN, layer name and settings.
  olp::dataservice::read::VersionedLayerClient layer_client(
      olp::client::HRN(catalog), first_layer_id, boost::none, client_settings);

  // Retrieve the partition data
  // Create a DataRequest with appropriate LayerId and PartitionId
  auto request = olp::dataservice::read::DataRequest()
                   .WithPartitionId(first_partition_id)
                   .WithBillingTag(boost::none);
  if (cache_settings.disk_path_protected.is_initialized()) {
    request.WithFetchOption(olp::dataservice::read::FetchOptions::CacheOnly);
  }

  // Run the DataRequest
  auto future = layer_client.GetData(request);

  // Wait for DataResponse
  olp::dataservice::read::DataResponse data_response =
    future.GetFuture().get();

  // Compact mutable cache, so it can be used as protected cache
  cache->Compact();

  // Retrieve data from the response
  return (HandleDataResponse(data_response) ? 0 : -1);
}

int RunExampleProtectedCache(const AccessKey& access_key, const std::string& catalog)
{
  // Read data with mutable cache.
  olp::cache::CacheSettings cache_settings;
  cache_settings.disk_path_mutable =
      olp::utils::Dir::TempDirectory() + kClientCacheDir;
  if (RunExampleReadWithCache(access_key, cache_settings, catalog)) {
    OLP_SDK_LOG_ERROR(kLogTag, "Error read data with mutable cache.");
    return -1;
  }
  // Read data with protected cache. Set mutable cache to none.
  cache_settings.disk_path_protected = cache_settings.disk_path_mutable;
  cache_settings.disk_path_mutable = boost::none;
  return RunExampleReadWithCache(access_key, cache_settings, catalog);
}
