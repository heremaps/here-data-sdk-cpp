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
#include <vector>

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
   Get the number of flush events attempted.

   @return number of attempts.
   */
  virtual size_t GetNumFlushEventsAttempted() const = 0;
  /**
   Get the number of flush events failed.

   @return number of failues.
   */
  virtual size_t GetNumFlushEventsFailed() const = 0;
  /**
   Get the total number of flush events.

   @return total number of flush events.
   */
  virtual size_t GetNumFlushEvents() const = 0;
  /**
   Get the number of requests flushed.

   @return number of flushed requests.
   */
  virtual size_t GetNumFlushedRequests() const = 0;
  /**
   Get the number of requests which are failed to be flushed.

   @return number of failed requests.
   */
  virtual size_t GetNumFlushedRequestsFailed() const = 0;

  /**
   Notify the start of the flush event
   */
  virtual void NotifyFlushEventStarted() = 0;
  /**
   Notify the flush event results. The IngestPublishResponse are listed
   in the same order as the requests being flushed.

   @param results vector of flush event results.
   */
  virtual void NotifyFlushEventResults(FlushResponse results) = 0;
};

/**
 @brief Default implementation of the FlushEventListener
 */
template <typename FlushResponse>
class DefaultFlushEventListener : public FlushEventListener<FlushResponse> {
 public:
  /**
   Number of flush events attempted.

   @return Number of the flush events attempted
   */
  size_t GetNumFlushEventsAttempted() const override {
    return num_flush_events_attempted_;
  }
  /**
   Get the number of flush events failed.

   @return number of failues.
   */
  size_t GetNumFlushEventsFailed() const override {
    return num_flush_events_failed_;
  }
  /**
   Total number of flush events.

   @return Total number of flush events.
   */
  size_t GetNumFlushEvents() const override { return num_flush_events_; }
  /**
   Number of flushed requests.

   @return Number of flushed requests
   */
  size_t GetNumFlushedRequests() const override {
    return total_flush_requests_;
  }
  /**
   Number of requests that were failed to be flushed.

   @return Number of failed requests
   */
  size_t GetNumFlushedRequestsFailed() const override {
    return num_flush_requests_failed_;
  }

  /**
   Notify the start of the flush event.

   */
  void NotifyFlushEventStarted() override { ++num_flush_events_attempted_; }

  /**
   Notify the flush event results. The IngestPublishResponse are listed
   in the same order as the requests being flushed.

   @param results vector of flush event results.
   */
  void NotifyFlushEventResults(FlushResponse results) override;

 protected:
  template <typename T>
  bool CollateFlushEventResults(const std::vector<T>& results) {
    total_flush_requests_ += results.size();

    thread_local size_t flushRequestsFailed;
    flushRequestsFailed = 0ull;
    auto self = this;
    std::for_each(std::begin(results), std::end(results), [self](T t) {
      if (!t.IsSuccessful()) {
        ++flushRequestsFailed;
        ++self->num_flush_requests_failed_;
      }
    });
    return flushRequestsFailed > 0ull;
  }

  std::atomic_size_t num_flush_events_attempted_{};
  std::atomic_size_t num_flush_events_failed_{};
  std::atomic_size_t num_flush_events_{};
  std::atomic_size_t total_flush_requests_{};
  std::atomic_size_t num_flush_requests_failed_{};
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
