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
#include <memory>
#include <mutex>
#include "CancellationToken.h"

#include <olp/core/CoreApi.h>

namespace olp {
namespace client {

/**
 * @brief Wrapper which manages cancellation state for any asynchronous
 * operations in a thread safe way.
 * All public APIs are thread safe.
 * This class is both movable and copyable.
 */
class CORE_API CancellationContext {
 public:
  using ExecuteFuncType = std::function<CancellationToken()>;
  using CancelFuncType = std::function<void()>;

  CancellationContext();

  /**
   * @brief Execute a given cancellable code block if the operation has not yet
   * been cancelled. Otherwise, execute a custom cancellation function.
   * @param execute_fn A function to execute if this operation is not yet
   * cancelled. This function should return a CancellationToken which
   * CancellationContext will propogate a cancel request to.
   * @param cancel_fn A function which will be called if this operation is
   * already cancelled.
   * @return true in case of successfull execution, false in case context was
   * cancelled.
   * @deprecated Will be removed once TaskScheduler will be used.
   */
  bool ExecuteOrCancelled(const ExecuteFuncType& execute_fn,
                          const CancelFuncType& cancel_fn = nullptr);

  /**
   * @brief Allows the user to cancel an ongoing operation in a threadsafe way.
   */
  void CancelOperation();

  /**
   * @brief Check if this context was cancelled.
   * @return `true` on context cancelled, `false` otherwise.
   */
  bool IsCancelled() const;

 private:
  /**
   * @brief Implementation to be able to shared a instance of
   * CancellationContext.
   */
  struct CancellationContextImpl {
    /// Mutex lock to protect from concurrent read/write.
    mutable std::recursive_mutex mutex_;
    /// Sub operation context return from ExecuteOrCancelled() execute_fn.
    /// @deprecated This will be removed once TaskScheduler is used.
    CancellationToken sub_operation_cancel_token_{};
    /// Flag that will be set to `true` on CancelOperation().
    bool is_cancelled_{false};
  };

  /// Shared implementation.
  std::shared_ptr<CancellationContextImpl> impl_;
};

}  // namespace client
}  // namespace olp

#include "CancellationContext.inl"
