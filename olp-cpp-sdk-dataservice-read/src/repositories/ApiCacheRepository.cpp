/*
 * Copyright (C) 2019-2024 HERE Europe B.V.
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

#include "ApiCacheRepository.h"

#include <olp/core/cache/KeyGenerator.h>
#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/logging/Log.h>

namespace {
constexpr auto kLogTag = "ApiCacheRepository";
constexpr time_t kLookupApiExpiryTime = 3600;
}  // namespace

namespace olp {
namespace dataservice {
namespace read {
namespace repository {
ApiCacheRepository::ApiCacheRepository(
    const client::HRN& hrn, std::shared_ptr<cache::KeyValueCache> cache)
    : hrn_(hrn), cache_(cache) {}

void ApiCacheRepository::Put(const std::string& service,
                             const std::string& version,
                             const std::string& url) {
  const std::string hrn(hrn_.ToCatalogHRNString());
  const auto key = cache::KeyGenerator::CreateApiKey(hrn, service, version);
  OLP_SDK_LOG_TRACE_F(kLogTag, "Put -> '%s'", key.c_str());

  cache_->Put(key, url, [&]() { return url; }, kLookupApiExpiryTime);
}

porting::optional<std::string> ApiCacheRepository::Get(
    const std::string& service, const std::string& version) {
  const std::string hrn(hrn_.ToCatalogHRNString());
  const auto key = cache::KeyGenerator::CreateApiKey(hrn, service, version);
  OLP_SDK_LOG_TRACE_F(kLogTag, "Get -> '%s'", key.c_str());

  auto url = cache_->Get(key, [](const std::string& value) { return value; });
  if (url.empty()) {
    return olp::porting::none;
  }

  return olp::porting::any_cast<std::string>(url);
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
