/*
 * Copyright (C) 2021-2022 HERE Europe B.V.
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

#include "PrefetchPartitionsHelper.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <olp/core/client/CancellationContext.h>
#include <olp/core/client/PendingRequests.h>
#include <olp/core/logging/Log.h>
#include <olp/dataservice/read/Types.h>
#include "Common.h"
#include "DownloadItemsJob.h"
#include "ExtendedApiResponseHelpers.h"
#include "QueryPartitionsJob.h"
#include "TaskSink.h"
#include "repositories/PartitionsRepository.h"

namespace olp {
namespace dataservice {
namespace read {
namespace {
const size_t kQueryPartitionsMaxSize = 100u;
}

void PrefetchPartitionsHelper::Prefetch(
    std::shared_ptr<DownloadJob> download_job,
    std::vector<std::string> partitions, QueryFunc query, TaskSink& task_sink,
    uint32_t priority, client::CancellationContext execution_context) {
  auto query_job = std::make_shared<QueryPartitionsJob>(
      std::move(query), nullptr, download_job, task_sink, execution_context,
      priority);

  size_t query_size = partitions.size() / kQueryPartitionsMaxSize;
  query_size += (partitions.size() % kQueryPartitionsMaxSize > 0) ? 1 : 0;

  query_job->Initialize(query_size);

  OLP_SDK_LOG_DEBUG_F("PrefetchJob", "Starting queries, requests=%zu",
                      query_size);

  execution_context.ExecuteOrCancelled(
      [&]() {
        VectorOfTokens tokens;
        tokens.reserve(query_size);

        for (auto p_it = partitions.begin(); p_it != partitions.end();) {
          const size_t batch_size = std::min(
              kQueryPartitionsMaxSize,
              static_cast<size_t>(std::distance(p_it, partitions.end())));
          std::vector<std::string> elements(
              std::make_move_iterator(p_it),
              std::make_move_iterator(p_it + batch_size));

          std::advance(p_it, batch_size);

          auto query_partition_func =
              [query_job](client::CancellationContext context,
                          std::vector<std::string>& partitions) {
                return query_job->Query(std::move(partitions), context);
              };

          auto token = task_sink.AddTaskChecked(
              std::bind(std::move(query_partition_func), std::placeholders::_1,
                        std::move(elements)),
              [query_job](PartitionsDataHandleExtendedResponse response) {
                query_job->CompleteQuery(std::move(response));
              },
              priority);

          if (!token) {
            query_job->CompleteQuery(
                client::ApiError(client::ErrorCode::Cancelled, "Cancelled"));
          } else {
            tokens.emplace_back(*token);
          }
        }

        return CreateToken(std::move(tokens));
      },
      [&]() {
        download_job->OnPrefetchCompleted(
            {{client::ErrorCode::Cancelled, "Cancelled"}});
      });
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
