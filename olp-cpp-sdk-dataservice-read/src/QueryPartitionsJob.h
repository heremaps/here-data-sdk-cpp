/*
 * Copyright (C) 2020-2022 HERE Europe B.V.
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

#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include <olp/core/client/CancellationContext.h>
#include <olp/core/logging/Log.h>
#include <olp/dataservice/read/ExtendedApiResponse.h>
#include <olp/dataservice/read/Types.h>
#include "Common.h"
#include "QueryMetadataJob.h"
#include "TaskSink.h"

namespace olp {
namespace dataservice {
namespace read {

using PartitionDataHandleResult =
    std::vector<std::pair<std::string, std::string>>;
using PartitionsDataHandleExtendedResponse =
    ExtendedApiResponse<PartitionDataHandleResult, client::ApiError,
                        client::NetworkStatistics>;

class QueryPartitionsJob
    : public QueryMetadataJob<
          std::string, std::vector<std::string>, PrefetchPartitionsResult,
          PartitionsDataHandleExtendedResponse, PrefetchPartitionsStatus> {
 public:
  QueryPartitionsJob(
      QueryItemsFunc<std::string, std::vector<std::string>,
                     PartitionsDataHandleExtendedResponse>
          query,
      FilterItemsFunc<PartitionDataHandleResult> filter,
      std::shared_ptr<DownloadItemsJob<std::string, PrefetchPartitionsResult,
                                       PrefetchPartitionsStatus>>
          download_job,
      TaskSink& task_sink, client::CancellationContext execution_context,
      uint32_t priority)
      : QueryMetadataJob(std::move(query), std::move(filter),
                         std::move(download_job), task_sink, execution_context,
                         priority) {}

  virtual bool CheckIfFail() {
    // Return error only if all fails
    return (query_errors_.size() == query_size_);
  }
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
