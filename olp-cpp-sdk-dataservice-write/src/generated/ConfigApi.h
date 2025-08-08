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

#include <memory>
#include <string>

#include <olp/core/porting/optional.hpp>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/OlpClient.h>

#include "model/Catalog.h"

namespace olp {
namespace dataservice {
namespace write {
using CatalogResponse =
    client::ApiResponse<olp::dataservice::write::model::Catalog,
                        client::ApiError>;
using CatalogCallback = std::function<void(CatalogResponse)>;

/**
 * @brief Api to access catalogs.
 */
class ConfigApi {
 public:
  /**
   * @brief Call to asynchronously retrieve the configuration of a catalog.
   * @param client Instance of OlpClient used to make REST request.
   * @param catalog_hrn Full catalog name.
   * @param billing_tag An optional free-form tag which is used for grouping
   * billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters  [A-Za-z0-9].
   * @param callback
   * @param A callback function to invoke with the catalog configuration
   * response.
   *
   * @return The cancellation token.
   */
  static client::CancellationToken GetCatalog(
      std::shared_ptr<client::OlpClient> client, const std::string& catalog_hrn,
      porting::optional<std::string> billing_tag,
      const CatalogCallback& callback);

  /**
   * @brief Call to synchronously retrieve the configuration of a catalog.
   * @param client Instance of OlpClient used to make REST request.
   * @param catalog_hrn Full catalog name.
   * @param billing_tag An optional free-form tag which is used for grouping
   * billing records together. If supplied, it must be between 4 - 16
   * characters,
   * contain only alpha/numeric ASCII characters  [A-Za-z0-9].
   * @param context A CancellationContext instance which can be used to cancel
   * this method.
   * @return The result of operation as a client::ApiResponse object.
   */
  static CatalogResponse GetCatalog(const client::OlpClient& client,
                                    const std::string& catalog_hrn,
                                    porting::optional<std::string> billing_tag,
                                    client::CancellationContext context);
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
