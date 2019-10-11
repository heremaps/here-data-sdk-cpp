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

#pragma once

#include <algorithm>
#include <memory>
#include <mutex>
#include <vector>

#include <olp/dataservice/write/FlushMetrics.h>
#include <olp/dataservice/write/FlushEventListener.h>

namespace olp {
namespace dataservice {
namespace write {

/**
 @brief Default implementation of the FlushEventListener.
 @deprecated Currently deperecated and not used in the public API.
 Should be refactored and used directly in \c StreamLayerClientImpl and \c
 AutoFlushController.
 */
template <typename FlushResponse>
class DefaultFlushEventListener : public FlushEventListener<FlushResponse> {
 public:
  void NotifyFlushEventStarted() override {
    FlushMetrics metrics;
    {
      std::lock_guard<std::mutex> locker(mutex_);
      ++metrics_.num_attempted_flush_events;
      metrics = metrics_;
    }
    NotifyFlushMetricsHasChanged(std::move(metrics));
  }

  void NotifyFlushEventResults(FlushResponse results) override;

  void NotifyFlushMetricsHasChanged(FlushMetrics metrics) override{};

 protected:
  template <typename T>
  bool CollateFlushEventResults(const std::vector<T>& results) {
    metrics_.num_total_flushed_requests += results.size();

    thread_local size_t flush_requests_failed =
        std::count_if(std::begin(results), std::end(results),
                      [](T result) -> bool { return !result.IsSuccessful(); });
    metrics_.num_failed_flushed_requests += flush_requests_failed;
    return flush_requests_failed > 0ull;
  }

  mutable std::mutex mutex_;
  FlushMetrics metrics_;
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
