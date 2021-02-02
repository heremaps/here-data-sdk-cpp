/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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

#include <chrono>
#include <condition_variable>
#include <mutex>

#include <olp/core/client/CancellationContext.h>

namespace olp {
namespace client {

/**
 * @brief A helper class that allows one thread to call and wait for a
 * notification in the other thread.
 */
class CORE_API Condition final {
 public:
  Condition() = default;

  /**
   * @brief Called by the task callback to notify `Wait` to unblock
   * the routine.
   */
  void Notify() {
    std::unique_lock<std::mutex> lock(mutex_);
    signaled_ = true;

    // Condition should be under the lock not to run into the data
    // race that might occur when a spurious wakeup happens in the other
    // thread while waiting for the condition signal.
    condition_.notify_one();
  }

  /**
   * @brief Waits for the `Notify` function.
   *
   * @param timeout The time (in milliseconds) during which the `Wait`
   * function waits for the notification.
   *
   * @return True if the notification is returned before the timeout; false
   * otherwise.
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
