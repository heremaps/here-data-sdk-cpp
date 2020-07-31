/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
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

#include <future>
#include <memory>

#include <olp/core/client/ApiLookupClient.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include <olp/dataservice/read/CatalogRequest.h>
#include <olp/dataservice/read/CatalogVersionRequest.h>
#include <olp/dataservice/read/Types.h>
#include <olp/dataservice/read/VersionsRequest.h>

namespace olp {
namespace client {
class OlpClient;
struct OlpClientSettings;
class PendingRequests;
}  // namespace client

namespace dataservice {
namespace read {
namespace repository {
class CatalogRepository;
}  // namespace repository

class CatalogClientImpl final {
 public:
  CatalogClientImpl(client::HRN catalog, client::OlpClientSettings settings);

  ~CatalogClientImpl();

  bool CancelPendingRequests();

  client::CancellationToken GetCatalog(CatalogRequest request,
                                       CatalogResponseCallback callback);

  client::CancellableFuture<CatalogResponse> GetCatalog(CatalogRequest request);

  client::CancellationToken GetLatestVersion(CatalogVersionRequest request,
                                             CatalogVersionCallback callback);

  client::CancellableFuture<CatalogVersionResponse> GetLatestVersion(
      CatalogVersionRequest request);

  client::CancellationToken ListVersions(VersionsRequest request,
                                         VersionsResponseCallback callback);

  client::CancellableFuture<VersionsResponse> ListVersions(
      VersionsRequest request);

 private:
  client::HRN catalog_;
  client::OlpClientSettings settings_;
  std::shared_ptr<thread::TaskScheduler> task_scheduler_;
  std::shared_ptr<client::PendingRequests> pending_requests_;
  client::ApiLookupClient lookup_client_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
