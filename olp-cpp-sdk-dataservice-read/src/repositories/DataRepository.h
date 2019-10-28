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

#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include "MultiRequestContext.h"
#include "generated/api/BlobApi.h"
#include "olp/dataservice/read/Types.h"

namespace olp {
namespace dataservice {
namespace read {
class DataRequest;
namespace repository {
class ApiRepository;
class CatalogRepository;
class PartitionsRepository;
class PartitionsCacheRepository;
class DataCacheRepository;

class DataRepository final {
 public:
  DataRepository(const client::HRN& hrn, std::shared_ptr<ApiRepository> apiRepo,
                 std::shared_ptr<CatalogRepository> catalogRepo,
                 std::shared_ptr<PartitionsRepository> partitionsRepo,
                 std::shared_ptr<cache::KeyValueCache> cache);

  ~DataRepository() = default;

  client::CancellationToken GetData(const read::DataRequest& request,
                                    const read::DataResponseCallback& callback);

  void GetData(std::shared_ptr<client::CancellationContext> cancellationContext,
               const std::string& layerType, const read::DataRequest& request,
               const read::DataResponseCallback& callback);

  static DataResponse GetVersionedData(const client::HRN& catalog,
                                       const std::string& layer_id,
                                       DataRequest data_request,
                                       client::CancellationContext context,
                                       client::OlpClientSettings settings);

  static DataResponse GetVolatileData(const client::HRN& catalog,
                                      const std::string& layer_id,
                                      DataRequest request,
                                      client::CancellationContext context,
                                      client::OlpClientSettings settings);

  static DataResponse GetBlobData(
      const client::HRN& catalog, const std::string& layer,
      const std::string& service, const DataRequest& data_request,
      client::CancellationContext cancellation_context,
      client::OlpClientSettings settings);

  static bool IsInlineData(const std::string& dataHandle);

 private:
  client::HRN hrn_;
  std::shared_ptr<ApiRepository> apiRepo_;
  std::shared_ptr<CatalogRepository> catalogRepo_;
  std::shared_ptr<PartitionsRepository> partitionsRepo_;
  std::shared_ptr<DataCacheRepository> cache_;
  std::shared_ptr<PartitionsCacheRepository> partitionsCache_;
  std::shared_ptr<MultiRequestContext<read::DataResponse>> multiRequestContext_;
};
}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
