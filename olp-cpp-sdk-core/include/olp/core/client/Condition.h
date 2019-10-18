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

#include <olp/core/client/CancellationContext.h>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace olp {
namespace client {

/**
 * @brief The Condition helper class allows to wait until the notification is
 * called by a task or cancellation is performed by the user.
 */
class Condition final {
 public:
  Condition() = default;

  /**
   * @brief Should be called by task's callback to notify \c Wait to unblock the
   * routine.
   */
  void Notify() {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      signaled_ = true;
    }
    condition_.notify_one();
  }

  /**
   * @brief Waits a task for a \c Notify or \c CancellationContext to be
   * cancelled by the user.
   * @param timeout milliseconds to wait on condition
   * @return True on notified wake, False on timeout
   */
  bool Wait(std::chrono::milliseconds timeout = std::chrono::seconds(60)) {
    std::unique_lock<std::mutex> lock(mutex_);
    bool triggered =
        condition_.wait_for(lock, timeout, [&] { return signaled_; });

    signaled_ = false;
    return triggered;
  }

 private:
  std::condition_variable condition_;
  std::mutex mutex_;
  bool signaled_{false};
};

}  // namespace client
}  // namespace olp
