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

#include "PrefetchJob.h"

#include <olp/core/logging/Log.h>
#include <olp/dataservice/read/PrefetchTileResult.h>

namespace {
constexpr auto kLogTag = "PrefetchJob";
}

namespace olp {
namespace dataservice {
namespace read {

PrefetchJob::PrefetchJob(PrefetchTilesResponseCallback user_callback,
                         uint32_t task_count)
    : user_callback_(std::move(user_callback)),
      task_count_{task_count},
      canceled_{false} {
  tasks_contexts_.reserve(task_count_);
  prefetch_result_.reserve(task_count_);
}

PrefetchJob::~PrefetchJob() = default;

client::CancellationContext PrefetchJob::AddTask() {
  std::lock_guard<std::mutex> lock(mutex_);
  tasks_contexts_.emplace_back();
  return tasks_contexts_.back();
}

void PrefetchJob::CompleteTask(geo::TileKey tile) {
  CompleteTask(
      std::make_shared<PrefetchTileResult>(tile, PrefetchTileNoError()));
}

void PrefetchJob::CompleteTask(geo::TileKey tile,
                               const client::ApiError& error) {
  CompleteTask(std::make_shared<PrefetchTileResult>(tile, error));
}

void PrefetchJob::CancelOperation() {
  std::lock_guard<std::mutex> lock(mutex_);
  canceled_ = true;

  for (auto& context : tasks_contexts_) {
    context.CancelOperation();
  }
}

void PrefetchJob::CompleteTask(std::shared_ptr<PrefetchTileResult> result) {
  std::lock_guard<std::mutex> lock(mutex_);
  prefetch_result_.push_back(std::move(result));

  if (!--task_count_) {
    OLP_SDK_LOG_INFO_F(kLogTag, "Prefetch done, tiles=%zu",
                       prefetch_result_.size());

    PrefetchTilesResponseCallback user_callback = std::move(user_callback_);
    if (canceled_) {
      user_callback({{client::ErrorCode::Cancelled, "Cancelled"}});
    } else {
      user_callback(std::move(prefetch_result_));
    }
  }
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
