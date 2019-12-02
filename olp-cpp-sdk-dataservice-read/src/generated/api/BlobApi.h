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

#include <string>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationContext.h>
#include <boost/optional.hpp>
#include "olp/dataservice/read/model/Data.h"

namespace olp {
namespace client {
class OlpClient;
}

namespace dataservice {
namespace read {
/**
 * @brief Api to upload and retrieve large volumes of data.
 */
class BlobApi {
 public:
  using DataResponse = client::ApiResponse<model::Data, client::ApiError>;
  using DataResponseCallback = std::function<void(DataResponse)>;

  /**
   * @brief Call to asynchronously retrieve a data blob for specified handle.
   * @param client Instance of OlpClient used to make REST request.
   * @param layerId Layer id.
   * @param dataHandle Indentifies a specific blob.
   * @param billingTag An optional free-form tag which is used for grouping
   * billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters  [A-Za-z0-9].
   * @param range Use this parameter to resume download of a large response for
   * versioned layers when there is a connection issue between the client and
   * server. Specify a single byte range offset like this: Range: bytes=10-.
   * This parameter is compliant with RFC 7233, but note that this parameter
   * only supports a single byte range. The range parameter can also be
   * specified as a query parameter, i.e. range=bytes=10-. For volatile layers
   * use the pagination links returned in the response body.
   * @param callback A callback function to invoke with the data response.
   *
   * @return The cancellation token.
   */

  static DataResponse GetBlob(const client::OlpClient& client,
                              const std::string& layerId,
                              const std::string& dataHandle,
                              boost::optional<std::string> billingTag,
                              boost::optional<std::string> range,
                              client::CancellationContext context);
};

}  // namespace read

}  // namespace dataservice
}  // namespace olp
