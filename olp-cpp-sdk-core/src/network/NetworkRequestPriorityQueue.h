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

#include "Memory.h"
#include "RequestContext.h"

#include <algorithm>
#include <climits>
#include <cstddef>
#include <deque>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <ostream>
#include <vector>

namespace olp {
namespace network {
/**
 * @brief Priority queue containing requests per priority level per client
 */
class CORE_API NetworkRequestPriorityQueue {
 public:
  /// Unsigned integer type
  using SizeType = std::deque<RequestContextPtr>::size_type;

  /**
   * @brief Push to the queue
   * @param [in] context request context to be pushed
   */
  void Push(const RequestContextPtr& context);

  /**
   * @brief Pop from the queue
   */
  RequestContextPtr Pop();

  /**
   * @brief Remove all elements satisying specific criteria.
   * @param f unary predicate which returns true if the element should be
   * removed
   */
  template <class Functor>
  mem::vector<RequestContextPtr> RemoveIf(Functor f) {
    using std::begin;
    using std::end;

    mem::vector<RequestContextPtr> removed;
    std::lock_guard<std::mutex> lock(lock_);

    std::for_each(
        begin(requests_), end(requests_),
        [&f, &removed](mem::deque<RequestContextPtr>& queue) {
          std::copy_if(begin(queue), end(queue), back_inserter(removed), f);
          queue.erase(std::remove_if(begin(queue), end(queue), f), end(queue));
        });

    return removed;
  }

  /**
   * @brief Get number of queued requests.
   * @return number of queued requests
   */
  SizeType Size() const;

 private:
  static const size_t kArraySize =
      NetworkRequest::PriorityMax - NetworkRequest::PriorityMin + 1;
  mem::deque<RequestContextPtr> requests_[kArraySize];
  mutable std::mutex lock_;
};

}  // namespace network
}  // namespace olp
