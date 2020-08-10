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

size_t GetAccumulatedBytes(const olp::client::NetworkStatistics& statistics) {
  // This narrow cast is necessary to avoid narrowing compiler errors like
  // -Wc++11-narrowing when building for 32bit targets.
  const auto bytes_transferred =
      statistics.GetBytesDownloaded() + statistics.GetBytesUploaded();
  return static_cast<size_t>(bytes_transferred &
                             std::numeric_limits<size_t>::max());
}
}  // namespace

namespace olp {
namespace dataservice {
namespace read {

PrefetchJob::PrefetchJob(PrefetchTilesResponseCallback user_callback,
                         PrefetchStatusCallback status_callback,
                         uint32_t task_count,
                         client::NetworkStatistics initial_network_statistics)
    : user_callback_(std::move(user_callback)),
      status_callback_(std::move(status_callback)),
      task_count_{task_count},
      total_task_count_{task_count},
      canceled_{false},
      accumulated_statistics_{initial_network_statistics} {
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
  CompleteTask(tile, client::NetworkStatistics{});
}

void PrefetchJob::CompleteTask(geo::TileKey tile,
                               const client::ApiError& error) {
  CompleteTask(tile, error, {});
}

void PrefetchJob::CompleteTask(geo::TileKey tile,
                               client::NetworkStatistics statistics) {
  CompleteTask(
      std::make_shared<PrefetchTileResult>(tile, PrefetchTileNoError()),
      statistics);
}

void PrefetchJob::CompleteTask(geo::TileKey tile, const client::ApiError& error,
                               client::NetworkStatistics statistics) {
  CompleteTask(std::make_shared<PrefetchTileResult>(tile, error), statistics);
}

void PrefetchJob::CancelOperation() {
  std::lock_guard<std::mutex> lock(mutex_);
  canceled_ = true;

  for (auto& context : tasks_contexts_) {
    context.CancelOperation();
  }
}

void PrefetchJob::CompleteTask(std::shared_ptr<PrefetchTileResult> result,
                               client::NetworkStatistics statistics) {
  std::lock_guard<std::mutex> lock(mutex_);
  prefetch_result_.push_back(std::move(result));

  accumulated_statistics_ += statistics;

  if (status_callback_) {
    status_callback_(
        PrefetchStatus{prefetch_result_.size(), total_task_count_,
                       GetAccumulatedBytes(accumulated_statistics_)});
  }

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
