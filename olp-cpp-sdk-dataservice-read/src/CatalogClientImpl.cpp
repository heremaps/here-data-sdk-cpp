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

#include "CatalogClientImpl.h"

#include <olp/core/client/OlpClientFactory.h>
#include <olp/core/logging/Log.h>

#include "PendingRequests.h"
#include "PrefetchTilesProvider.h"
#include "repositories/ApiRepository.h"
#include "repositories/CatalogRepository.h"
#include "repositories/DataRepository.h"
#include "repositories/PartitionsRepository.h"
#include "repositories/PrefetchTilesRepository.h"

namespace olp {
namespace dataservice {
namespace read {
using namespace repository;
using namespace olp::client;

namespace {
constexpr auto kLogTag = "CatalogClientImpl";
}

CatalogClientImpl::CatalogClientImpl(
    const HRN& hrn, std::shared_ptr<OlpClientSettings> settings,
    std::shared_ptr<cache::KeyValueCache> cache)
    : hrn_(hrn),
      settings_(settings),
      api_client_(OlpClientFactory::Create(*settings)) {
  // create repositories, satisfying dependencies.
  auto api_repo = std::make_shared<ApiRepository>(hrn_, settings_, cache);

  catalog_repo_ = std::make_shared<CatalogRepository>(hrn_, api_repo, cache);

  partition_repo_ = std::make_shared<PartitionsRepository>(
      hrn_, api_repo, catalog_repo_, cache);

  data_repo_ = std::make_shared<DataRepository>(hrn_, api_repo, catalog_repo_,
                                                partition_repo_, cache);

  prefetch_provider_ = std::make_shared<PrefetchTilesProvider>(
      hrn_, api_repo, catalog_repo_, data_repo_,
      std::make_shared<PrefetchTilesRepository>(
          hrn, api_repo, partition_repo_->GetPartitionsCacheRepository()));

  pending_requests_ = std::make_shared<PendingRequests>();
}

CatalogClientImpl::~CatalogClientImpl() { CancelPendingRequests(); }

bool CatalogClientImpl::CancelPendingRequests() {
  EDGE_SDK_LOG_TRACE(kLogTag, "CancelPendingRequests");
  return pending_requests_->CancelPendingRequests();
}

CancellationToken CatalogClientImpl::GetCatalog(
    const CatalogRequest& request, const CatalogResponseCallback& callback) {
  EDGE_SDK_LOG_TRACE_F(kLogTag, "GetCatalog '%s'", request.CreateKey().c_str());
  CancellationToken token;
  int64_t request_key = pending_requests_->GenerateKey();
  auto pending_requests = pending_requests_;
  auto request_callback = [pending_requests, request_key,
                           callback](CatalogResponse response) {
    EDGE_SDK_LOG_INFO_F(kLogTag, "GetCatalog remove key: %ld", request_key);
    pending_requests->Remove(request_key);
    callback(response);
  };
  if (CacheWithUpdate == request.GetFetchOption()) {
    auto req = request;
    token = catalog_repo_->getCatalog(req.WithFetchOption(CacheOnly),
                                      request_callback);
    auto onlineKey = pending_requests_->GenerateKey();
    EDGE_SDK_LOG_INFO_F(kLogTag, "GetCatalog add key: %ld", onlineKey);
    pending_requests_->Add(catalog_repo_->getCatalog(
                               req.WithFetchOption(OnlineIfNotFound),
                               [pending_requests, onlineKey](CatalogResponse) {
                                 pending_requests->Remove(onlineKey);
                               }),
                           onlineKey);
  } else {
    EDGE_SDK_LOG_INFO_F(kLogTag, "GetCatalog existing: %ld", request_key);
    token = catalog_repo_->getCatalog(request, request_callback);
  }
  pending_requests_->Add(token, request_key);
  return token;
}

CancellableFuture<CatalogResponse> CatalogClientImpl::GetCatalog(
    const CatalogRequest& request) {
  return AsFuture<CatalogRequest, CatalogResponse>(
      request, static_cast<client::CancellationToken (CatalogClientImpl::*)(
                   const CatalogRequest&, const CatalogResponseCallback&)>(
                   &CatalogClientImpl::GetCatalog));
}

CancellationToken CatalogClientImpl::GetCatalogMetadataVersion(
    const CatalogVersionRequest& request,
    const CatalogVersionCallback& callback) {
  EDGE_SDK_LOG_TRACE_F(kLogTag, "GetCatalog '%s'", request.CreateKey().c_str());
  CancellationToken token;
  int64_t request_key = pending_requests_->GenerateKey();
  auto pending_requests = pending_requests_;
  auto request_callback = [pending_requests, request_key,
                           callback](CatalogVersionResponse response) {
    EDGE_SDK_LOG_INFO_F(kLogTag, "GetCatalog remove key: %ld", request_key);
    pending_requests->Remove(request_key);
    callback(response);
  };
  if (CacheWithUpdate == request.GetFetchOption()) {
    auto req = request;
    token = catalog_repo_->getLatestCatalogVersion(
        req.WithFetchOption(CacheOnly), request_callback);
    auto onlineKey = pending_requests_->GenerateKey();
    EDGE_SDK_LOG_INFO_F(kLogTag, "GetCatalog add key: %ld", onlineKey);
    pending_requests_->Add(
        catalog_repo_->getLatestCatalogVersion(
            req.WithFetchOption(OnlineIfNotFound),
            [pending_requests, onlineKey](CatalogVersionResponse) {
              pending_requests->Remove(onlineKey);
            }),
        onlineKey);
  } else {
    EDGE_SDK_LOG_INFO_F(kLogTag, "GetCatalog existing: %ld", request_key);
    token = catalog_repo_->getLatestCatalogVersion(request, request_callback);
  }
  pending_requests_->Add(token, request_key);
  return token;
}

CancellableFuture<CatalogVersionResponse>
CatalogClientImpl::GetCatalogMetadataVersion(
    const CatalogVersionRequest& request) {
  return AsFuture<CatalogVersionRequest, CatalogVersionResponse>(
      request,
      static_cast<client::CancellationToken (CatalogClientImpl::*)(
          const CatalogVersionRequest&, const CatalogVersionCallback&)>(
          &CatalogClientImpl::GetCatalogMetadataVersion));
}

olp::client::CancellationToken CatalogClientImpl::GetPartitions(
    const PartitionsRequest& request,
    const PartitionsResponseCallback& callback) {
  CancellationToken token;
  int64_t request_key = pending_requests_->GenerateKey();
  auto pending_requests = pending_requests_;
  auto request_callback = [pending_requests, request_key,
                           callback](PartitionsResponse response) {
    pending_requests->Remove(request_key);
    callback(response);
  };
  if (CacheWithUpdate == request.GetFetchOption()) {
    auto req = request;
    token = partition_repo_->GetPartitions(req.WithFetchOption(CacheOnly),
                                           request_callback);
    auto onlineKey = pending_requests_->GenerateKey();
    pending_requests_->Add(
        partition_repo_->GetPartitions(
            req.WithFetchOption(OnlineIfNotFound),
            [pending_requests, onlineKey](PartitionsResponse) {
              pending_requests->Remove(onlineKey);
            }),
        onlineKey);
  } else {
    token = partition_repo_->GetPartitions(request, request_callback);
  }
  pending_requests_->Add(token, request_key);
  return token;
}

olp::client::CancellableFuture<PartitionsResponse>
CatalogClientImpl::GetPartitions(const PartitionsRequest& request) {
  return AsFuture<PartitionsRequest, PartitionsResponse>(
      request,
      static_cast<client::CancellationToken (CatalogClientImpl::*)(
          const PartitionsRequest&, const PartitionsResponseCallback&)>(
          &CatalogClientImpl::GetPartitions));
}

client::CancellationToken CatalogClientImpl::GetData(
    const DataRequest& request, const DataResponseCallback& callback) {
  CancellationToken token;
  int64_t request_key = pending_requests_->GenerateKey();
  auto pending_requests = pending_requests_;
  auto request_callback = [pending_requests, request_key,
                           callback](DataResponse response) {
    pending_requests->Remove(request_key);
    callback(response);
  };
  if (CacheWithUpdate == request.GetFetchOption()) {
    auto req = request;
    token =
        data_repo_->GetData(req.WithFetchOption(CacheOnly), request_callback);
    auto onlineKey = pending_requests_->GenerateKey();
    pending_requests_->Add(
        data_repo_->GetData(req.WithFetchOption(OnlineIfNotFound),
                            [pending_requests, onlineKey](DataResponse) {
                              pending_requests->Remove(onlineKey);
                            }),
        onlineKey);
  } else {
    token = data_repo_->GetData(request, request_callback);
  }
  pending_requests_->Add(token, request_key);
  return token;
}

client::CancellableFuture<DataResponse> CatalogClientImpl::GetData(
    const DataRequest& request) {
  return AsFuture<DataRequest, DataResponse>(
      request, static_cast<client::CancellationToken (CatalogClientImpl::*)(
                   const DataRequest&, const DataResponseCallback&)>(
                   &CatalogClientImpl::GetData));
}

client::CancellationToken CatalogClientImpl::PrefetchTiles(
    const PrefetchTilesRequest& request,
    const PrefetchTilesResponseCallback& callback) {
  int64_t request_key = pending_requests_->GenerateKey();
  auto request_callback = [=](const PrefetchTilesResponse& response) {
    pending_requests_->Remove(request_key);
    callback(response);
  };

  auto token = prefetch_provider_->PrefetchTiles(request, callback);

  pending_requests_->Add(token, request_key);
  return token;
}

client::CancellableFuture<PrefetchTilesResponse>
CatalogClientImpl::PrefetchTiles(const PrefetchTilesRequest& request) {
  return AsFuture<PrefetchTilesRequest, PrefetchTilesResponse>(
      request,
      static_cast<client::CancellationToken (CatalogClientImpl::*)(
          const PrefetchTilesRequest&, const PrefetchTilesResponseCallback&)>(
          &CatalogClientImpl::PrefetchTiles));
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
