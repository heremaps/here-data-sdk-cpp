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

#include <memory>
#include <string>

#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationContext.h>
#include "generated/model/Api.h"
#include "olp/core/client/ApiError.h"

namespace olp {
namespace client {
class OlpClient;
}
namespace dataservice {
namespace write {

/**
 * @brief Api to lookup platform base urls.
 */
class PlatformApi {
 public:
  using ApisResponse = client::ApiResponse<model::Apis, client::ApiError>;
  using ApisCallback = std::function<void(ApisResponse)>;

  /**
   * @brief Call to lookup platform base urls.
   * @param client Instance of OlpClient used to make REST request.
   * @param service Name of the service.
   * @param serviceVersion Version of the service.
   * @param A callback function to invoke with the collection of Api services
   * that match the parameters.
   *
   * @return The cancellation token.
   */
  static client::CancellationToken GetApis(
      std::shared_ptr<client::OlpClient> client, const std::string& service,
      const std::string& service_version, const ApisCallback& callback);

  /**
   * @brief Synchronous version of \c GetApis method.
   */
  static ApisResponse GetApis(const client::OlpClient& client,
                              const std::string& service,
                              const std::string& service_version,
                              client::CancellationContext cancel_context);
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
