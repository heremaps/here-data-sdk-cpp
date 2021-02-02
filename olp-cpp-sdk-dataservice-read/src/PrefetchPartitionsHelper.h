/*
 * Copyright (C) 2020-2021 HERE Europe B.V.
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
#include <olp/dataservice/read/Types.h>
#include "Common.h"
#include "DownloadItemsJob.h"
#include "QueryPartitionsJob.h"

namespace olp {
namespace dataservice {
namespace read {

class TaskSink;

class PrefetchPartitionsHelper {
 public:
  using DownloadJob = DownloadItemsJob<std::string, PrefetchPartitionsResult,
                                       PrefetchPartitionsStatus>;
  using QueryFunc = QueryItemsFunc<std::string, std::vector<std::string>,
                                   PartitionsDataHandleExtendedResponse>;

  static void Prefetch(std::shared_ptr<DownloadJob> download_job,
                       std::vector<std::string> partitions, QueryFunc query,
                       TaskSink& task_sink, uint32_t priority,
                       client::CancellationContext execution_context);
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
