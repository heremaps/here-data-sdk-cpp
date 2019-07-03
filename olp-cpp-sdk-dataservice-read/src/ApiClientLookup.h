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
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClient.h>
#include "generated/model/Api.h"

namespace olp {
namespace dataservice {
namespace read {
/**
 * @brief Api to lookup service base urls.
 */
class ApiClientLookup {
 public:
  using ApisResponse = client::ApiResponse<model::Apis, client::ApiError>;
  using ApisCallback = std::function<void(ApisResponse)>;

  static client::CancellationToken LookupApi(
      std::shared_ptr<client::OlpClient> client, const std::string& service,
      const std::string& service_version, const client::HRN& hrn,
      const ApisCallback& callback);

  using ApiClientResponse =
      client::ApiResponse<client::OlpClient, client::ApiError>;
  using ApiClientCallback = std::function<void(ApiClientResponse)>;

  static client::CancellationToken LookupApiClient(
      std::shared_ptr<client::OlpClient> client, const std::string& service,
      const std::string& service_version, const client::HRN& hrn,
      const ApiClientCallback& callback);
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
