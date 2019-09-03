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

#include <olp/core/cache/DefaultCache.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/context/Context.h>
#include <olp/core/thread/TaskScheduler.h>
#include "repositories/CatalogRepository.h"
#include "repositories/DataRepository.h"
#include "repositories/ExecuteOrSchedule.inl"
#include "repositories/PartitionsRepository.h"

namespace olp {
namespace dataservice {
namespace read {

VersionedLayerClientImpl::VersionedLayerClientImpl(
    olp::client::HRN catalog, std::string layer_id,
    olp::client::OlpClientSettings client_settings)
    : catalog_(std::move(catalog)),
      layer_id_(std::move(layer_id)),
      settings_(std::move(client_settings)),
      pending_requests_(std::make_shared<PendingRequests>()) {
  if (!settings_.cache) {
    settings_.cache =
        olp::client::OlpClientSettingsFactory::CreateDefaultCache({});
  }
  // to avoid capturing task scheduler inside a task, we need a copy of settings
  // without the scheduler
  task_scheduler_ = std::move(settings_.task_scheduler);
}

VersionedLayerClientImpl::~VersionedLayerClientImpl() {
  pending_requests_->CancelPendingRequests();
}

olp::client::CancellationToken VersionedLayerClientImpl::GetData(
    DataRequest data_request, Callback callback) const {
  auto fetch_option = data_request.GetFetchOption();
  if (fetch_option == CacheWithUpdate) {
    auto cache_token = AddGetDataTask(data_request.WithFetchOption(CacheOnly),
                                      std::move(callback));
    auto online_token =
        AddGetDataTask(data_request.WithFetchOption(OnlineIfNotFound), nullptr);
    return client::CancellationToken([cache_token, online_token]() {
      cache_token.cancel();
      online_token.cancel();
    });
  } else {
    return AddGetDataTask(data_request, std::move(callback));
  }
}

client::CancellationToken VersionedLayerClientImpl::AddGetDataTask(
    DataRequest data_request, Callback callback) const {
  auto catalog = catalog_;
  auto layer_id = layer_id_;
  auto settings = settings_;
  auto pending_requests = pending_requests_;
  auto request_key = pending_requests->GenerateRequestPlaceholder();
  olp::client::CancellationContext context;
  olp::client::CancellationToken token(
      [context]() mutable { context.CancelOperation(); });

  pending_requests->Insert(token, request_key);

  repository::ExecuteOrSchedule(
      task_scheduler_, [catalog, layer_id, settings, context, data_request,
                        pending_requests, request_key, callback]() {
        auto response = repository::DataRepository::GetVersionedData(
            std::move(catalog), std::move(layer_id), std::move(settings),
            std::move(data_request), std::move(context));
        pending_requests->Remove(request_key);
        if (callback) {
          callback(std::move(response));
        }
      });

  return token;
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
