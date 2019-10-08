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

#include "VolatileLayerClientImpl.h"

namespace olp {

namespace dataservice {

namespace read {
VolatileLayerClientImpl::VolatileLayerClientImpl(
    olp::client::HRN catalog, std::string layer_id,
    olp::client::OlpClientSettings client_settings) {}

VolatileLayerClientImpl::~VolatileLayerClientImpl() = default;

olp::client::CancellationToken VolatileLayerClientImpl::GetData(
    DataRequest data_request, Callback<DataResult> callback) {
  return olp::client::CancellationToken{[]() {}};
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
