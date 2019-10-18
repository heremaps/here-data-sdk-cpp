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

#include <olp/core/cache/DefaultCache.h>
#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/dataservice/read/PartitionsRequest.h>
#include "repositories/DataRepository.h"
#include "repositories/ExecuteOrSchedule.inl"
#include "repositories/PartitionsRepository.h"

#include "TaskContext.h"

namespace olp {
namespace dataservice {
namespace read {

namespace {
constexpr auto kLogTag = "VolatileLayerClientImpl";
}  // namespace

VolatileLayerClientImpl::VolatileLayerClientImpl(
    client::HRN catalog, std::string layer_id,
    client::OlpClientSettings settings)
    : catalog_(std::move(catalog)),
      layer_id_(std::move(layer_id)),
      settings_(std::move(settings)),
      pending_requests_(std::make_shared<PendingRequests>()) {
  if (!settings_.cache) {
    settings_.cache = client::OlpClientSettingsFactory::CreateDefaultCache({});
  }
  // to avoid capturing task scheduler inside a task, we need a copy of settings
  // without the scheduler
  task_scheduler_ = std::move(settings_.task_scheduler);
}

VolatileLayerClientImpl::~VolatileLayerClientImpl() {
  pending_requests_->CancelPendingRequests();
}

client::CancellationToken VolatileLayerClientImpl::GetData(
    DataRequest request, Callback<DataResponse> callback) {
  auto add_task = [&](DataRequest& request, Callback<DataResponse> callback) {
    auto catalog = catalog_;
    auto layer_id = layer_id_;
    auto settings = settings_;
    auto pending_requests = pending_requests_;

    auto data_task = [=](client::CancellationContext context) {
      return repository::DataRepository::GetVolatileData(
          catalog, layer_id, request, context, settings);
    };

    auto context =
        TaskContext::Create(std::move(data_task), std::move(callback));

    pending_requests->Insert(context);

    repository::ExecuteOrSchedule(task_scheduler_, [=]() {
      context.Execute();
      pending_requests->Remove(context);
    });

    return context.CancelToken();
  };

  if (request.GetFetchOption() == FetchOptions::CacheWithUpdate) {
    auto cache_token = add_task(
        request.WithFetchOption(FetchOptions::CacheOnly), std::move(callback));
    auto online_token =
        add_task(request.WithFetchOption(FetchOptions::OnlineOnly), nullptr);

    return client::CancellationToken([=]() {
      cache_token.cancel();
      online_token.cancel();
    });
  }

  return add_task(request, std::move(callback));
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
