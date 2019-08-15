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

#include "ApiRepository.h"
#include <olp/core/client/OlpClientFactory.h>
#include "ApiCacheRepository.h"

#include <olp/core/logging/Log.h>

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

namespace {
constexpr auto kLogTag = "ApiRepository";
} // namespace

using namespace olp::client;

ApiRepository::ApiRepository(
    const olp::client::HRN& hrn,
    std::shared_ptr<olp::client::OlpClientSettings> settings,
    std::shared_ptr<cache::KeyValueCache> cache)
    : hrn_(hrn),
      settings_(settings),
      cache_(std::make_shared<ApiCacheRepository>(hrn, cache)) {
  ApiClientResponse cancelledResponse{
      {olp::network::Network::ErrorCode::Cancelled, "Operation cancelled."}};
  multiRequestContext_ = std::make_shared<
      MultiRequestContext<ApiClientResponse, ApiClientCallback>>(
      cancelledResponse);
}

olp::client::CancellationToken ApiRepository::getApiClient(
    const std::string& service, const std::string& serviceVersion,
    const ApiClientCallback& callback) {
  EDGE_SDK_LOG_TRACE_F(kLogTag, "getApiClient(%s, %s)", service.c_str(),
                       serviceVersion.c_str());

  auto url = cache_->Get(service, serviceVersion);
  if (url) {
    EDGE_SDK_LOG_INFO_F(kLogTag, "getApiClient(%s, %s) -> from cache",
                        service.c_str(), serviceVersion.c_str());

    auto client = olp::client::OlpClientFactory::Create(*settings_);
    client->SetBaseUrl(*url);
    std::thread([=] { callback(*client); }).detach();
    return CancellationToken();
  }

  auto cache = cache_;
  auto hrn = hrn_;
  auto settings = settings_;
  MultiRequestContext<ApiClientResponse, ApiClientCallback>::ExecuteFn
      executeFn = [cache, hrn, settings, service,
                   serviceVersion](ApiClientCallback contextCallback) {
        EDGE_SDK_LOG_INFO_F(kLogTag, "getApiClient(%s, %s) -> execute",
                            service.c_str(), serviceVersion.c_str());

        auto cacheApiResponseCallback = [cache, hrn, settings, service,
                                         serviceVersion, contextCallback](
                                            ApiClientResponse response) {
          if (response.IsSuccessful()) {
            EDGE_SDK_LOG_INFO_F(kLogTag, "getApiClient(%s, %s) -> into cache",
                                service.c_str(), serviceVersion.c_str());
            cache->Put(service, serviceVersion,
                       response.GetResult().GetBaseUrl());
          }
          contextCallback(response);
        };

        return olp::dataservice::read::ApiClientLookup::LookupApiClient(
            olp::client::OlpClientFactory::Create(*settings), service,
            serviceVersion, hrn,
            [cacheApiResponseCallback](
                olp::dataservice::read::ApiClientLookup::ApiClientResponse
                    response) { cacheApiResponseCallback(response); });
      };
  std::string requestKey = service + "@" + serviceVersion;
  return multiRequestContext_->ExecuteOrAssociate(requestKey, executeFn,
                                                  callback);
}

const client::OlpClientSettings* ApiRepository::GetOlpClientSettings() const {
  return settings_ ? settings_.get() : nullptr;
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
