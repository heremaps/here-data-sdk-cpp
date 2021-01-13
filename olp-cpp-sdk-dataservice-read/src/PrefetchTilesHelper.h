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
#include <utility>
#include <vector>

#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/PendingRequests.h>
#include <olp/core/logging/Log.h>
#include <olp/dataservice/read/Types.h>
#include "Common.h"
#include "DownloadItemsJob.h"
#include "ExtendedApiResponse.h"
#include "ExtendedApiResponseHelpers.h"
#include "QueryMetadataJob.h"
#include "TaskSink.h"
#include "repositories/PrefetchTilesRepository.h"

namespace olp {
namespace dataservice {
namespace read {

class PrefetchTilesHelper {
 public:
  using DownloadJob =
      DownloadItemsJob<geo::TileKey, PrefetchTilesResult, PrefetchStatus>;
  using QueryFunc =
      QueryItemsFunc<geo::TileKey, geo::TileKey, repository::SubQuadsResponse>;

  static client::ApiError Canceled() {
    return client::ApiError(client::ErrorCode::Cancelled, "Cancelled");
  }

  static void Prefetch(std::shared_ptr<DownloadJob> download_job,
                       const std::vector<geo::TileKey>& roots, QueryFunc query,
                       FilterItemsFunc<repository::SubQuadsResult> filter,
                       TaskSink& task_sink, uint32_t priority,
                       client::CancellationContext execution_context) {
    auto query_job = std::make_shared<
        QueryMetadataJob<geo::TileKey, geo::TileKey, PrefetchTilesResult,
                         repository::SubQuadsResponse, PrefetchStatus>>(
        std::move(query), std::move(filter), download_job, task_sink,
        execution_context, priority);

    query_job->Initialize(roots.size());

    OLP_SDK_LOG_DEBUG_F("PrefetchJob", "Starting queries, requests=%zu",
                        roots.size());

    execution_context.ExecuteOrCancelled(
        [&]() {
          VectorOfTokens tokens;

          auto transform_func = [&](geo::TileKey root) {
            auto token = task_sink.AddTaskChecked(
                [=](client::CancellationContext context) {
                  return query_job->Query(root, context);
                },
                [=](repository::SubQuadsResponse response) {
                  query_job->CompleteQuery(std::move(response));
                },
                priority);
            if (!token) {
              query_job->CompleteQuery(Canceled());
              return client::CancellationToken();
            }
            return *token;
          };

          std::transform(std::begin(roots), std::end(roots),
                         std::back_inserter(tokens), std::move(transform_func));
          return CreateToken(std::move(tokens));
        },
        [&]() { download_job->OnPrefetchCompleted(Canceled()); });
  }
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
