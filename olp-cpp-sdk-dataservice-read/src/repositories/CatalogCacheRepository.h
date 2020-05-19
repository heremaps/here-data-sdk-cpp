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

#include <chrono>
#include <memory>

#include <olp/core/client/HRN.h>
#include <olp/dataservice/read/model/Catalog.h>
#include <olp/dataservice/read/model/VersionResponse.h>
#include <boost/optional.hpp>

namespace olp {
namespace cache {
class KeyValueCache;
}
namespace dataservice {
namespace read {
namespace repository {

class CatalogCacheRepository final {
 public:
  CatalogCacheRepository(
      const client::HRN& hrn, std::shared_ptr<cache::KeyValueCache> cache,
      std::chrono::seconds default_expiry = std::chrono::seconds::max());

  ~CatalogCacheRepository() = default;

  void Put(const model::Catalog& catalog);

  boost::optional<model::Catalog> Get();

  void PutVersion(const model::VersionResponse& version);

  boost::optional<model::VersionResponse> GetVersion();

  void Clear();

 private:
  client::HRN hrn_;
  std::shared_ptr<cache::KeyValueCache> cache_;
  time_t default_expiry_;
};
}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
