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
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#include <olp/dataservice/write/FlushMetrics.h>

namespace olp {
namespace dataservice {
namespace write {

/**
 @brief FlushEventListener is the listener that can be used to monitor the
 request flush events. Clients can provide concrete implementation by
 extending this class.
 */
template <typename FlushResponse>
class FlushEventListener {
 public:
  virtual ~FlushEventListener() {}

  /**
   * Notify the start of the flush event
   */
  virtual void NotifyFlushEventStarted() = 0;
  /**
   * Notify the flush event results. The IngestPublishResponse are listed
   * in the same order as the requests being flushed.
   *
   *  @param results vector of flush event results.
   */
  virtual void NotifyFlushEventResults(FlushResponse results) = 0;

  /**
   * Notifies the listener that flush metrics has changed.
   *
   * @param metrics Collected \c FlushMetrics.
   */
  virtual void NotifyFlushMetricsHasChanged(FlushMetrics metrics) = 0;
};

/**
 @brief Default implementation of the FlushEventListener
 */
template <typename FlushResponse>
class DefaultFlushEventListener : public FlushEventListener<FlushResponse> {
 public:
  /**
   Notify the start of the flush event.

   */
  void NotifyFlushEventStarted() override {
    FlushMetrics metrics;
    {
      std::lock_guard<std::mutex> locker(mutex_);
      ++metrics_.num_attempted_flush_events;
      metrics = metrics_;
    }
    NotifyFlushMetricsHasChanged(std::move(metrics));
  }

  /**
   Notify the flush event results. The IngestPublishResponse are listed
   in the same order as the requests being flushed.

   @param results vector of flush event results.
   */
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
