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

#include <boost/optional.hpp>
#include <memory>
#include <string>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationContext.h>
#include "olp/dataservice/read/model/Catalog.h"

namespace olp {
namespace client {
class OlpClient;
}

namespace dataservice {
namespace read {
/**
 * @brief Api to access catalogs.
 */
class ConfigApi {
 public:
  using CatalogResponse = client::ApiResponse<model::Catalog, client::ApiError>;

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
                                    boost::optional<std::string> billing_tag,
                                    client::CancellationContext context);
};

}  // namespace read

}  // namespace dataservice
}  // namespace olp
