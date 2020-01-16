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

#include <memory>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/ApiResponse.h>

#include <olp/dataservice/read/model/Catalog.h>
#include <olp/dataservice/read/model/Data.h>
#include <olp/dataservice/read/model/Partitions.h>
#include <olp/dataservice/read/model/VersionResponse.h>

namespace olp {
namespace dataservice {
namespace read {

class PrefetchTileResult;

/// Response template type
template <typename ResultType>
using Response = client::ApiResponse<ResultType, client::ApiError>;

/// Callback template type
template <typename ResultType>
using Callback = std::function<void(Response<ResultType>)>;

/// Catalog configuration alias type
using CatalogResult = model::Catalog;
/// Catalog configuration response type
using CatalogResponse = Response<CatalogResult>;
/// Catalog configuration response callback type
using CatalogResponseCallback = Callback<CatalogResult>;

/// Catalog version result alias type
using CatalogVersionResult = model::VersionResponse;
/// Catalog version response type
using CatalogVersionResponse = Response<CatalogVersionResult>;
/// Catalog version response callback type
using CatalogVersionCallback = Callback<CatalogVersionResult>;

/// Partition metadata result alias type
using PartitionsResult = model::Partitions;
/// Partition metadata response type
using PartitionsResponse = Response<PartitionsResult>;
/// Partition metadata response callback type
using PartitionsResponseCallback = Callback<PartitionsResult>;

/// Data alias type
using DataResult = model::Data;
/// Data response alias
using DataResponse = Response<DataResult>;
/// Data response callback type
using DataResponseCallback = Callback<DataResult>;

/// Prefetch tiles result alias
using PrefetchTilesResult = std::vector<std::shared_ptr<PrefetchTileResult>>;
/// Prefetch tiles response type
using PrefetchTilesResponse = Response<PrefetchTilesResult>;
/// Prefetch completion callback type
using PrefetchTilesResponseCallback = Callback<PrefetchTilesResult>;

/// The subcribe ID type of the stream layer client.
using SubscriptionId = std::string;
/// The subscribe tiles response type of the stream layer client.
using SubscribeResponse = Response<SubscriptionId>;
/// The subscribe completion callback type of the stream layer client.
using SubscribeResponseCallback = Callback<SubscriptionId>;

}  // namespace read
}  // namespace dataservice
}  // namespace olp
