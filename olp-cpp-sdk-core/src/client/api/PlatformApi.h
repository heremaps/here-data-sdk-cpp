/*
 * Copyright (C) 2020 HERE Europe B.V.
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

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/model/Api.h>
#include <boost/optional.hpp>

namespace olp {
namespace client {
class OlpClient;
class CancellationContext;

/**
 * @brief Api to lookup platform base urls.
 */
class PlatformApi {
 public:
  using ApisResult = std::pair<Apis, boost::optional<time_t>>;
  using ApisResponse = ApiResponse<ApisResult, ApiError>;
  using ApisCallback = std::function<void(ApisResponse)>;

  /**
   * @brief Call to lookup platform base urls.
   * @param client Instance of OlpClient used to make REST request.
   * @param context A CancellationContext, which can be used to cancel any
   * pending request.
   *
   * @return The Apis response.
   */
  static ApisResponse GetApis(const OlpClient& client,
                              const CancellationContext& context);

  /**
   * @brief Asynchronous version of \c GetApis method.
   */
  static CancellationToken GetApis(const OlpClient& client,
                                   const ApisCallback& callback);
};

}  // namespace client
}  // namespace olp
