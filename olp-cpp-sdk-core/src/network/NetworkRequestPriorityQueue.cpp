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

#include "NetworkRequestPriorityQueue.h"

#include <numeric>

namespace olp {
namespace network {
void NetworkRequestPriorityQueue::Push(const RequestContextPtr& context) {
  std::lock_guard<std::mutex> lock(lock_);
  requests_[context->request_.Priority()].push_back(context);
}

RequestContextPtr NetworkRequestPriorityQueue::Pop() {
  std::lock_guard<std::mutex> lock(lock_);
  for (int i = NetworkRequest::PriorityMax; i >= NetworkRequest::PriorityMin;
       i--) {
    if (requests_[i].empty()) continue;

    auto ctx = requests_[i].front();
    if (ctx) {
      requests_[i].pop_front();
      return ctx;
    }
  }
  return RequestContextPtr();
}

NetworkRequestPriorityQueue::SizeType NetworkRequestPriorityQueue::Size()
    const {
  std::lock_guard<std::mutex> lock(lock_);
  return std::accumulate(
      begin(requests_), end(requests_), SizeType(0),
      [](SizeType count, const mem::deque<RequestContextPtr>& queue) {
        return count += queue.size();
      });
}

}  // namespace network
}  // namespace olp
