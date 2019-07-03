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

#include "ApiCacheRepository.h"

#include <olp/core/cache/KeyValueCache.h>

namespace {
std::string CreateKey(const std::string& hrn, const std::string& service,
                      const std::string& serviceVersion) {
  return hrn + "::" + service + "::" + serviceVersion + "::api";
}
}  // namespace

namespace olp {
namespace dataservice {
namespace read {
namespace repository {
using namespace olp::client;
ApiCacheRepository::ApiCacheRepository(
    const HRN& hrn, std::shared_ptr<cache::KeyValueCache> cache)
    : hrn_(hrn), cache_(cache) {}

void ApiCacheRepository::Put(const std::string& service,
                             const std::string& serviceVersion,
                             const std::string& serviceUrl) {
  std::string hrn(hrn_.ToCatalogHRNString());
  cache_->Put(CreateKey(hrn, service, serviceVersion), serviceUrl,
              [serviceUrl]() { return serviceUrl; }, 3600);
}

boost::optional<std::string> ApiCacheRepository::Get(
    const std::string& service, const std::string& serviceVersion) {
  std::string hrn(hrn_.ToCatalogHRNString());
  auto url = cache_->Get(CreateKey(hrn, service, serviceVersion),
                         [](const std::string& value) { return value; });
  if (url.empty()) {
    return boost::none;
  }

  return boost::any_cast<std::string>(url);
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
