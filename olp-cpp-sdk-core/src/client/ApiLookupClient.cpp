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

#include "olp/core/client/ApiLookupClient.h"

#include "client/ApiLookupClientImpl.h"

namespace olp {
namespace client {

ApiLookupClient::ApiLookupClient(const HRN& catalog,
                                 const OlpClientSettings& settings)
    : impl_(std::make_shared<ApiLookupClientImpl>(catalog, settings)) {}

ApiLookupClient::~ApiLookupClient() = default;

ApiLookupClient::LookupApiResponse ApiLookupClient::LookupApi(
    const std::string& service, const std::string& service_version,
    FetchOptions options, CancellationContext context) {
  return impl_->LookupApi(service, service_version, options,
                          std::move(context));
}

CancellationToken ApiLookupClient::LookupApi(const std::string& service,
                                             const std::string& service_version,
                                             FetchOptions options,
                                             LookupApiCallback callback) {
  return impl_->LookupApi(service, service_version, options,
                          std::move(callback));
}

}  // namespace client
}  // namespace olp
