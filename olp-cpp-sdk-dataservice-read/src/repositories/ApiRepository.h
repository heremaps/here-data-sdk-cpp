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

#pragma once

#include "memory"

#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientSettings.h>

#include <olp/core/client/HRN.h>

#include "ApiClientLookup.h"
#include "MultiRequestContext.h"

namespace olp {
namespace cache {
class KeyValueCache;
}
namespace dataservice {
namespace read {
namespace repository {
class ApiCacheRepository;

using ApiClientResponse = ApiClientLookup::ApiClientResponse;
using ApiClientCallback = ApiClientLookup::ApiClientCallback;

class ApiRepository final {
 public:
  ApiRepository(const olp::client::HRN& hrn,
                std::shared_ptr<olp::client::OlpClientSettings> settings,
                std::shared_ptr<cache::KeyValueCache> cache);

  ~ApiRepository() = default;

  olp::client::CancellationToken getApiClient(
      const std::string& service, const std::string& serviceVersion,
      const ApiClientCallback& callback);

  const client::OlpClientSettings* GetOlpClientSettings() const;

 private:
  olp::client::HRN hrn_;
  std::shared_ptr<client::OlpClientSettings> settings_;
  std::shared_ptr<ApiCacheRepository> cache_;
  std::shared_ptr<MultiRequestContext<ApiClientResponse, ApiClientCallback>>
      multiRequestContext_;
};
}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
