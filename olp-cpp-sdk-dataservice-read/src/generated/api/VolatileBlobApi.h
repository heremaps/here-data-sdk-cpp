/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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
#include <boost/optional.hpp>
#include "olp/dataservice/read/model/Data.h"

#include "ExtendedApiResponse.h"

namespace olp {
namespace client {
class OlpClient;
class CancellationContext;
}  // namespace client

namespace dataservice {
namespace read {
/**
 * @brief Api to upload and retrieve large volumes of data.
 */
class VolatileBlobApi {
 public:
  using DataResponse = ExtendedApiResponse<model::Data, client::ApiError,
                                           client::NetworkStatistics>;

  /**
   * @brief Retrieves a volatile data blob for specified handle.
   * @param client Instance of OlpClient used to make REST request.
   * @param layer_id Layer id.
   * @param data_handle Identifies a specific blob.
   * @param billing_tag An optional free-form tag which is used for grouping
   * billing records together. If supplied, it must be between 4 - 16
   * characters, contain only alpha/numeric ASCII characters  [A-Za-z0-9].
   * @param context A CancellationContext, which can be used to cancel request.
   *
   * @return Data response.
   */
  static DataResponse GetVolatileBlob(
      const client::OlpClient& client, const std::string& layer_id,
      const std::string& data_handle, boost::optional<std::string> billing_tag,
      const client::CancellationContext& context);

  static client::CancellationToken GetVolatileBlob(
      const client::OlpClient& client, const std::string& layer_id,
      const std::string& data_handle, boost::optional<std::string> billing_tag,
      std::function<void(DataResponse)> callback);
};

}  // namespace read

}  // namespace dataservice
}  // namespace olp
