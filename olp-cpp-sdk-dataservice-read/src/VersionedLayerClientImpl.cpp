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

#include "VersionedLayerClientImpl.h"

#include "ApiClientLookup.h"
#include "Condition.h"

#include "olp/core/client/CancellationContext.h"
#include "olp/core/logging/Log.h"
#include "olp/core/thread/TaskScheduler.h"
#include "repositories/ApiCacheRepository.h"
#include "repositories/CatalogRepository.h"
#include "repositories/DataRepository.h"
#include "repositories/ExecuteOrSchedule.inl"
#include "repositories/PartitionsRepository.h"

namespace olp {
namespace dataservice {
namespace read {
namespace {
constexpr auto kLogTag = "VersionedLayerClientImpl";
}

VersionedLayerClientImpl::VersionedLayerClientImpl(
    client::HRN catalog, std::string layer,
    client::OlpClientSettings client_settings)
    : catalog_(std::move(catalog)),
      layer_(std::move(layer)),
      settings_(std::move(client_settings)),
      pending_requests_(std::make_shared<PendingRequests>()) {
  safe_settings_ = settings_;
  safe_settings_.task_scheduler = nullptr;
}

VersionedLayerClientImpl::~VersionedLayerClientImpl() {
  pending_requests_->CancelPendingRequests();
}

client::CancellationToken VersionedLayerClientImpl::GetData(
    DataRequest data_request, Callback<DataResult> callback) {
  auto add_task = [this](DataRequest data_request,
                         Callback<model::Data> callback) {
    using namespace client;
    CancellationContext context;
    CancellationToken token([=]() mutable { context.CancelOperation(); });

    auto pending_requests = pending_requests_;
    int64_t request_key = pending_requests->GenerateRequestPlaceholder();

    pending_requests->Insert(token, request_key);

    repository::ExecuteOrSchedule(&settings_, [=]() {
      auto response = GetData(context, data_request);
      pending_requests->Remove(request_key);
      if (callback) {
        callback(std::move(response));
      }
    });

    return token;
  };

  auto fetch_option = data_request.GetFetchOption();
  if (fetch_option == CacheWithUpdate) {
    auto cache_token =
        add_task(data_request.WithFetchOption(CacheOnly), callback);
    auto online_token =
        add_task(data_request.WithFetchOption(OnlineIfNotFound), nullptr);
    return client::CancellationToken([cache_token, online_token]() {
      cache_token.cancel();
      online_token.cancel();
    });
  } else {
    return add_task(data_request, callback);
  }
}

VersionedLayerClientImpl::CallbackResponse<model::Data>
VersionedLayerClientImpl::GetData(client::CancellationContext context,
                                  DataRequest request) const {
  if (context.IsCancelled()) {
    return client::ApiError(client::ErrorCode::Cancelled,
                            "Operation cancelled.");
  }

  if (!request.GetPartitionId() && !request.GetDataHandle()) {
    auto key = request.CreateKey();
    OLP_SDK_LOG_INFO_F(kLogTag, "getData for '%s' failed", key.c_str());
    return client::ApiError(client::ErrorCode::InvalidArgument,
                            "A data handle or a partition id must be defined.");
  }

  if (!request.GetDataHandle()) {
    if (!request.GetVersion()) {
      auto version = repository::CatalogRepository::GetLatestVersionSync(
          catalog_, context, request, safe_settings_);
      if (!version.IsSuccessful()) {
        return version.GetError();
      }

      request.WithVersion(version.GetResult().GetVersion());
    }

    auto query_response =
        repository::PartitionsRepository::GetPartitionByIdSync(
            catalog_, layer_, context, request, safe_settings_);

    if (!query_response.IsSuccessful()) {
      return query_response.GetError();
    }

    const auto& result = query_response.GetResult();
    const auto& partitions = result.GetPartitions();
    if (partitions.empty()) {
      return client::ApiError(client::ErrorCode::NotFound,
                              "Requested partition not found.");
    }

    request.WithDataHandle(partitions.at(0).GetDataHandle());
  }

  return repository::DataRepository::GetBlobDataSync(catalog_, layer_, request,
                                                     context, safe_settings_);
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
