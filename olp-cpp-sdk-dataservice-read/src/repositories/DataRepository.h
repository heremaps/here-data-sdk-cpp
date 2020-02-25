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
#include "olp/dataservice/read/DataRequest.h"
#include "olp/dataservice/read/Types.h"
#include "olp/dataservice/read/TileRequest.h"

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

class DataRepository final {
 public:
  static DataResponse GetVersionedTile(const client::HRN& catalog,
                                       const std::string& layer_id,
                                       TileRequest request, int64_t version,
                                       client::CancellationContext context,
                                       client::OlpClientSettings settings);

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
};
}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
