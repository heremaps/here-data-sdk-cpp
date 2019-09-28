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

#include "olp/core/client/CancellationContext.h"

namespace olp {
namespace dataservice {
namespace read {

class Condition {
 public:
  Condition(client::CancellationContext context, int timeout = 60)
      : context_{context}, timeout_{timeout}, signaled_{false} {}

  void Notify() {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      signaled_ = true;
    }
    condition_.notify_one();
  }

  bool Wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    bool triggered = condition_.wait_for(
        lock, std::chrono::seconds(timeout_),
        [&] { return signaled_ || context_.IsCancelled(); });

    signaled_ = false;
    return triggered;
  }

 private:
  client::CancellationContext context_;
  int timeout_;
  std::condition_variable condition_;
  std::mutex mutex_;
  bool signaled_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp