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

#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include "MultiRequestContext.h"
#include "generated/api/MetadataApi.h"
#include "olp/dataservice/read/DataRequest.h"
#include "olp/dataservice/read/Types.h"

namespace olp {
namespace dataservice {
namespace read {

class CatalogRequest;
class CatalogVersionRequest;

namespace repository {
class ApiRepository;
class CatalogCacheRepository;

class CatalogRepository final {
 public:
  CatalogRepository(const client::HRN& hrn,
                    std::shared_ptr<ApiRepository> apiRepo,
                    std::shared_ptr<cache::KeyValueCache> cache);

  ~CatalogRepository() = default;

  client::CancellationToken getCatalog(
      const read::CatalogRequest& request,
      const read::CatalogResponseCallback& callback);

  static read::CatalogResponse GetCatalog(client::HRN catalog,
                                          client::CancellationContext context,
                                          read::CatalogRequest request,
                                          client::OlpClientSettings settings);

  client::CancellationToken getLatestCatalogVersion(
      const read::CatalogVersionRequest& request,
      const read::CatalogVersionCallback& callback);

  static MetadataApi::CatalogVersionResponse GetLatestVersion(
      client::HRN catalog, client::CancellationContext cancellation_context,
      CatalogVersionRequest request, client::OlpClientSettings settings);

 private:
  client::HRN hrn_;
  std::shared_ptr<ApiRepository> apiRepo_;
  std::shared_ptr<CatalogCacheRepository> cache_;
  std::shared_ptr<MultiRequestContext<read::CatalogResponse>>
      multiRequestContext_;
};
}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
