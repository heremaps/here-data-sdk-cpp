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

#include <olp/core/cache/DefaultCache.h>
#include <olp/core/client/OlpClientSettingsFactory.h>
#include <olp/core/client/PendingRequests.h>
#include <olp/core/logging/Log.h>

#include "Common.h"
#include "PrefetchTilesProvider.h"
#include "repositories/ApiRepository.h"
#include "repositories/CatalogRepository.h"
#include "repositories/DataRepository.h"
#include "repositories/ExecuteOrSchedule.inl"
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

CatalogClientImpl::CatalogClientImpl(HRN catalog, OlpClientSettings settings)
    : catalog_(std::move(catalog)), settings_(std::move(settings)) {
  if (!settings_.cache) {
    settings_.cache = client::OlpClientSettingsFactory::CreateDefaultCache({});
  }

  // to avoid capturing task scheduler inside a task, we need a copy of settings
  // without the scheduler
  task_scheduler_ = std::move(settings_.task_scheduler);

  pending_requests_ = std::make_shared<client::PendingRequests>();
}

CatalogClientImpl::~CatalogClientImpl() {
  pending_requests_->CancelAllAndWait();
}

bool CatalogClientImpl::CancelPendingRequests() {
  OLP_SDK_LOG_TRACE(kLogTag, "CancelPendingRequests");
  return pending_requests_->CancelAll();
}

CancellationToken CatalogClientImpl::GetCatalog(
    CatalogRequest request, CatalogResponseCallback callback) {
  auto schedule_get_catalog = [&](CatalogRequest request,
                                  CatalogResponseCallback callback) {
    auto catalog = catalog_;
    auto settings = settings_;

    auto get_catalog_task = [=](client::CancellationContext context) {
      return repository::CatalogRepository::GetCatalog(
          std::move(catalog), std::move(context), std::move(request),
          std::move(settings));
    };

    return AddTask(task_scheduler_, pending_requests_,
                   std::move(get_catalog_task), std::move(callback));
  };

  return ScheduleFetch(std::move(schedule_get_catalog), std::move(request),
                       std::move(callback));
}

CancellableFuture<CatalogResponse> CatalogClientImpl::GetCatalog(
    CatalogRequest request) {
  auto promise = std::make_shared<std::promise<CatalogResponse>>();
  auto cancel_token =
      GetCatalog(std::move(request), [promise](CatalogResponse response) {
        promise->set_value(std::move(response));
      });
  return client::CancellableFuture<CatalogResponse>(std::move(cancel_token),
                                                    std::move(promise));
}

CancellationToken CatalogClientImpl::GetLatestVersion(
    CatalogVersionRequest request, CatalogVersionCallback callback) {
  OLP_SDK_LOG_TRACE_F(kLogTag, "GetCatalog '%s'", request.CreateKey().c_str());
  auto schedule_get_latest_version = [&](CatalogVersionRequest request,
                                         CatalogVersionCallback callback) {
    auto catalog = catalog_;
    auto settings = settings_;

    auto get_latest_version_task = [=](client::CancellationContext context) {
      return repository::CatalogRepository::GetLatestVersion(
          std::move(catalog), std::move(context), std::move(request),
          std::move(settings));
    };

    return AddTask(task_scheduler_, pending_requests_,
                   std::move(get_latest_version_task), std::move(callback));
  };

  return ScheduleFetch(std::move(schedule_get_latest_version),
                       std::move(request), std::move(callback));
}

CancellableFuture<CatalogVersionResponse> CatalogClientImpl::GetLatestVersion(
    CatalogVersionRequest request) {
  auto promise = std::make_shared<std::promise<CatalogVersionResponse>>();
  auto cancel_token = GetLatestVersion(
      std::move(request), [promise](CatalogVersionResponse response) {
        promise->set_value(std::move(response));
      });
  return client::CancellableFuture<CatalogVersionResponse>(
      std::move(cancel_token), std::move(promise));
}
}  // namespace read
}  // namespace dataservice
}  // namespace olp
