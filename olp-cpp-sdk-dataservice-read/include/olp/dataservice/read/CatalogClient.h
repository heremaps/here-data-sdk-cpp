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

#include <functional>
#include <memory>

#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/geo/tiling/TileKey.h>
#include <olp/core/porting/deprecated.h>
#include <olp/dataservice/read/CatalogRequest.h>
#include <olp/dataservice/read/CatalogVersionRequest.h>
#include <olp/dataservice/read/DataRequest.h>
#include <olp/dataservice/read/DataServiceReadApi.h>
#include <olp/dataservice/read/PartitionsRequest.h>
#include <olp/dataservice/read/Types.h>

namespace olp {

namespace client {
class HRN;
}  // namespace client

namespace dataservice {
namespace read {

class CatalogClientImpl;

/**
 * @brief The CatalogClient class. Marshals Requests and their Results.
 */
class DATASERVICE_READ_API CatalogClient final {
 public:
  CatalogClient(client::HRN catalog, client::OlpClientSettings settings);

  // Movable, non-copyable
  CatalogClient(const CatalogClient& other) = delete;
  CatalogClient(CatalogClient&& other) noexcept;
  CatalogClient& operator=(const CatalogClient& other) = delete;
  CatalogClient& operator=(CatalogClient&& other) noexcept;

  ~CatalogClient();

  /**
   * @brief Cancel currently active requests.
   * @return True on success
   */
  bool CancelPendingRequests();

  /**
   * @brief fetches catalog configuration asynchronously.
   * @param request contains the complete set of request parameters.
   * @param callback will be invoked once the catalog configuration is
   * available, or an error is encountered.
   * @return A token that can be used to cancel this request
   */
  client::CancellationToken GetCatalog(CatalogRequest request,
                                       CatalogResponseCallback callback);

  /**
   * @brief fetches catalog configuration asynchronously.
   * @param request contains the complete set of request parameters.
   * @return CancellableFuture of type CatalogResponse, which when complete will
   * contain the catalog configuration or an error. Alternatively, the
   * CancellableFuture can be used to cancel this request.
   */
  client::CancellableFuture<CatalogResponse> GetCatalog(CatalogRequest request);

  /**
   * @brief fetches catalog version asynchronously.
   * @param request contains the complete set of request parameters.
   * @param callback will be invoked once the catalog version is
   * available, or an error is encountered.
   * @return A token that can be used to cancel this request
   */
  client::CancellationToken GetLatestVersion(CatalogVersionRequest request,
                                             CatalogVersionCallback callback);

  /**
   * @brief fetches catalog version asynchronously.
   * @param request contains the complete set of request parameters.
   * @return CancellableFuture of type CatalogVersionResponse, which when
   * complete will contain the catalog configuration or an error.
   * Alternatively, the CancellableFuture can be used to cancel this request.
   */
  client::CancellableFuture<CatalogVersionResponse> GetLatestVersion(
      CatalogVersionRequest request);

 private:
  std::unique_ptr<CatalogClientImpl> impl_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
