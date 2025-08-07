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

#pragma once

#include <string>
#include <unordered_map>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiLookupClient.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/FetchOptions.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/client/model/Api.h>

namespace olp {
namespace client {

class ApiLookupClientImpl {
 public:
  ApiLookupClientImpl(const HRN& catalog, const OlpClientSettings& settings);

  ApiLookupClient::LookupApiResponse LookupApi(
      const std::string& service, const std::string& service_version,
      FetchOptions options, CancellationContext context);

  CancellationToken LookupApi(const std::string& service,
                              const std::string& service_version,
                              FetchOptions options,
                              ApiLookupClient::LookupApiCallback callback);

 protected:
  using ApisResult = std::pair<Apis, porting::optional<time_t>>;

  struct ClientWithExpiration {
    OlpClient client;
    std::chrono::steady_clock::time_point expire_at;
  };

  OlpClient CreateAndCacheClient(const std::string& base_url,
                                 const std::string& cache_key,
                                 porting::optional<time_t> expiration);

  porting::optional<OlpClient> GetCachedClient(
      const std::string& service, const std::string& service_version);

  void PutToDiskCache(const ApisResult& available_services);

  const HRN& catalog_;
  const std::string catalog_string_;
  const OlpClientSettings& settings_;
  OlpClient lookup_client_;

  std::mutex cached_clients_mutex_;
  std::unordered_map<std::string, ClientWithExpiration> cached_clients_;
};

}  // namespace client
}  // namespace olp
