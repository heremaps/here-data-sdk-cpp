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
#include "ExtendedApiResponse.h"
#include "ExtendedApiResponseHelpers.h"
#include "QueryPartitionsJob.h"
#include "TaskSink.h"
#include "repositories/PartitionsRepository.h"

namespace olp {
namespace dataservice {
namespace read {

class PrefetchPartitionsHelper {
 public:
  using DownloadJob = DownloadItemsJob<std::string, PrefetchPartitionsResult,
                                       PrefetchPartitionsStatus>;
  using QueryFunc = QueryItemsFunc<std::string, std::vector<std::string>,
                                   PartitionsDataHandleExtendedResponse>;

  static client::CancellationToken Prefetch(
      std::shared_ptr<DownloadJob> download_job,
      const std::vector<std::string>& roots, QueryFunc query,
      TaskSink& task_sink, size_t query_max_size, uint32_t priority) {
    client::CancellationContext execution_context;

    auto query_job = std::make_shared<QueryPartitionsJob>(
        std::move(query), nullptr, download_job, task_sink, execution_context,
        priority);

    auto query_size = roots.size() / query_max_size;
    query_size += (roots.size() % query_max_size > 0) ? 1 : 0;

    query_job->Initialize(query_size);

    OLP_SDK_LOG_DEBUG_F("PrefetchJob", "Starting queries, requests=%zu",
                        roots.size());

    execution_context.ExecuteOrCancelled(
        [&]() {
          VectorOfTokens tokens;
          tokens.reserve(query_size);

          // split items to blocks
          auto size_left = roots.size();
          auto start = 0u;
          while (size_left > start) {
            auto size = std::min(query_max_size, size_left - start);
            auto query_element = std::vector<std::string>(
                roots.begin() + start, roots.begin() + start + size);

            tokens.emplace_back(task_sink.AddTask(
                [query_element,
                 query_job](client::CancellationContext context) {
                  return query_job->Query(std::move(query_element), context);
                },
                [query_job](PartitionsDataHandleExtendedResponse response) {
                  query_job->CompleteQuery(std::move(response));
                },
                priority));

            start += size;
          }

          return CreateToken(std::move(tokens));
        },
        [&]() {
          download_job->OnPrefetchCompleted(
              {{client::ErrorCode::Cancelled, "Cancelled"}});
        });

    return client::CancellationToken(
        [execution_context]() mutable { execution_context.CancelOperation(); });
  }
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
