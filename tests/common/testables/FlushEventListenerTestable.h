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

#include <mutex>

#include <olp/dataservice/write/StreamLayerClient.h>

namespace olp {
namespace tests {
namespace common {

class FlushEventListenerTestable
    : public olp::dataservice::write::DefaultFlushEventListener<
          const olp::dataservice::write::StreamLayerClient::FlushResponse&> {
 public:
  size_t GetNumFlushEventsAttempted() const {
    std::lock_guard<std::mutex> locker(mutex_);
    return metrics_.num_attempted_flush_events;
  }

  size_t GetNumFlushEventsFailed() const {
    std::lock_guard<std::mutex> locker(mutex_);
    return metrics_.num_failed_flush_events;
  }

  size_t GetNumFlushEvents() const {
    std::lock_guard<std::mutex> locker(mutex_);
    return metrics_.num_total_flush_events;
  }

  size_t GetNumFlushedRequests() const {
    std::lock_guard<std::mutex> locker(mutex_);
    return metrics_.num_total_flushed_requests;
  }

  size_t GetNumFlushedRequestsFailed() const {
    std::lock_guard<std::mutex> locker(mutex_);
    return metrics_.num_failed_flushed_requests;
  }

  void NotifyFlushMetricsHasChanged(
      olp::dataservice::write::FlushMetrics metrics) override {
    std::lock_guard<std::mutex> locker(mutex_);
    metrics_ = std::move(metrics);
  }

 private:
  mutable std::mutex mutex_;
  olp::dataservice::write::FlushMetrics metrics_;
};

}  // namespace common
}  // namespace tests
}  // namespace olp
