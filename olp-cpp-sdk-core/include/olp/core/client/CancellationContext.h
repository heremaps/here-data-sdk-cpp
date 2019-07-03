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

#include <functional>
#include <mutex>
#include "CancellationToken.h"

#include <olp/core/CoreApi.h>

namespace olp {
namespace client {

/**
 * @brief Wrapper which manages cancellation state for multiple network
 * operations in a thread safe way.
 */
class CORE_API CancellationContext {
 public:
  /**
  * @brief Execute a given cancellable code block if the operation has not yet
  been cancelled. Otherwise, execute a custom cancellation function.
  * @param execute_fn A function to execute if this operation is not yet
  cancelled. This function should return a CancellationToken which
  CancellationContext will propogate a cancel request to.
  * @param cancel_fn A function which will be called if this operation is
  already cancelled.
  */
  inline void ExecuteOrCancelled(
      const std::function<olp::client::CancellationToken()>& execute_fn,
      const std::function<void()>& cancel_fn) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (is_cancelled_) {
      cancel_fn();
      return;
    }

    sub_operation_cancel_token_ = execute_fn();
  }

  /**
   * @brief Allows the user to cancel an ongoing operation which may consist of
   * multiple network calls in a threadsafe way.
   */
  inline void CancelOperation() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (is_cancelled_) {
      return;
    }

    sub_operation_cancel_token_.cancel();
    sub_operation_cancel_token_ = CancellationToken();
    is_cancelled_ = true;
  }

  inline bool IsCancelled() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return is_cancelled_;
  }

 private:
  std::recursive_mutex mutex_;
  olp::client::CancellationToken sub_operation_cancel_token_{};
  bool is_cancelled_{false};
};

}  // namespace client
}  // namespace olp
