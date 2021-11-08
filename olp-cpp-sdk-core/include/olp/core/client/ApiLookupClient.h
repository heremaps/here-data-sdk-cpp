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

#include <olp/core/CoreApi.h>
#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/FetchOptions.h>
#include <olp/core/client/HRN.h>
#include <olp/core/client/OlpClient.h>
#include <olp/core/client/OlpClientSettings.h>

namespace olp {
namespace client {
class ApiLookupClientImpl;

/// A client for API lookup requests.
class CORE_API ApiLookupClient final {
 public:
  /// Alias for the lookup API response.
  using LookupApiResponse = ApiResponse<OlpClient, ApiError>;
  /// Alias for the lookup API callback.
  using LookupApiCallback = std::function<void(LookupApiResponse)>;

  /**
   * @brief Creates the `ApiLookupClient` instance.
   * 
   * @param catalog The catalog HRN.
   * @param settings The `OlpClientSettings` instance.
   */
  explicit ApiLookupClient(const HRN& catalog,
                           const OlpClientSettings& settings);

  ~ApiLookupClient();

  /**
   * @brief Gets a API for a single service sync, internally uses
   * ApiLookupSettings from OlpClientSettings.
   *
   * @param service The name of the required service.
   * @param service_version The version of the required service.
   * @param options The fetch option that should be used to set the source from
   * which data should be fetched.
   * @param context The `CancellationContext` instance that is used to cancel
   * the request.
   *
   * @return `LookupApiResponse` that contains the `OlpClient` instance
   * or an error.
   */
  LookupApiResponse LookupApi(const std::string& service,
                              const std::string& service_version,
                              FetchOptions options,
                              CancellationContext context);

  /**
   * @brief Gets an API for a single service asynchronously.
   *
   * Internally, it uses `ApiLookupSettings` from `OlpClientSettings`.
   *
   * @param service The name of the required service.
   * @param service_version The version of the required service.
   * @param options The fetch option that should be used to set the source from
   * which data should be fetched.
   * @param callback The function callback used to receive the
   * `LookupApiResponse` instance.
   *
   * @note If the catalog endpoint provider is set and provides a static URL
   * for this catalog, the method does not trigger any asynchronous download and
   * provides the synchronous result instead. This means that
   * the user needs to take special care in case the callback is called within
   * the same context and avoid locking any mutex twice.
   *
   * @return The method used to call or to cancel the request.
   */
  CancellationToken LookupApi(const std::string& service,
                              const std::string& service_version,
                              FetchOptions options, LookupApiCallback callback);

 private:
  std::shared_ptr<ApiLookupClientImpl> impl_;
};

}  // namespace client
}  // namespace olp
