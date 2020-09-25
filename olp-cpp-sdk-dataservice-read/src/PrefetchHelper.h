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

namespace olp {
namespace dataservice {
namespace read {

template <typename PrefetchItemsResult>
using PrefetchItemsResponseCallback = Callback<PrefetchItemsResult>;

class PrefetchHelper {
 public:
  template <typename ItemType, typename QueryType, typename PrefetchResult>
  static client::CancellationToken Prefetch(
      const std::vector<QueryType>& roots,
      QueryItemsFunc<ItemType, QueryType> query,
      FilterItemsFunc<ItemType> filter, DownloadFunc download,
      AppendResultFunc<ItemType, PrefetchResult> append_result,
      Callback<PrefetchResult> user_callback,
      PrefetchStatusCallback status_callback, TaskSink& task_sink,
      uint32_t priority) {
    client::CancellationContext execution_context;

    auto download_job =
        std::make_shared<DownloadItemsJob<ItemType, PrefetchResult>>(
            std::move(download), std::move(append_result),
            std::move(user_callback), std::move(status_callback));

    auto query_job =
        std::make_shared<QueryMetadataJob<ItemType, QueryType, PrefetchResult>>(
            std::move(query), std::move(filter), download_job, task_sink,
            execution_context, priority);

    query_job->Initialize(roots.size());

    OLP_SDK_LOG_DEBUG_F("PrefetchJob", "Starting queries, requests=%zu",
                        roots.size());

    execution_context.ExecuteOrCancelled([&]() {
      VectorOfTokens tokens;
      std::transform(std::begin(roots), std::end(roots),
                     std::back_inserter(tokens), [&](QueryType root) {
                       return task_sink.AddTask(
                           [=](client::CancellationContext context) {
                             return query_job->Query(root, context);
                           },
                           [=](QueryItemsResponse<ItemType> response) {
                             query_job->CompleteQuery(std::move(response));
                           },
                           priority);
                     });
      return CreateToken(std::move(tokens));
    });

    return client::CancellationToken(
        [execution_context]() mutable { execution_context.CancelOperation(); });
  }
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
