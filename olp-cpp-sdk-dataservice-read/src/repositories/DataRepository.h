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

#include <olp/core/client/ApiLookupClient.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClientSettings.h>
#include "olp/dataservice/read/DataRequest.h"
#include "olp/dataservice/read/Types.h"

#include "generated/api/BlobApi.h"

namespace olp {
namespace dataservice {
namespace read {
class TileRequest;
namespace repository {

class DataRepository final {
 public:
  DataRepository(client::HRN catalog, client::OlpClientSettings settings,
                 client::ApiLookupClient client);

  DataResponse GetVersionedTile(const std::string& layer_id,
                                const TileRequest& request, int64_t version,
                                client::CancellationContext context);

  BlobApi::DataResponse GetVersionedData(const std::string& layer_id,
                                         const DataRequest& data_request,
                                         int64_t version,
                                         client::CancellationContext context);

  DataResponse GetVolatileData(const std::string& layer_id,
                               const DataRequest& request,
                               client::CancellationContext context);

  BlobApi::DataResponse GetBlobData(const std::string& layer,
                                    const std::string& service,
                                    const DataRequest& data_request,
                                    client::CancellationContext context);

 private:
  client::HRN catalog_;
  client::OlpClientSettings settings_;
  client::ApiLookupClient lookup_client_;
};

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
