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

#include <functional>
#include <memory>
#include <mutex>

#include <olp/core/CoreApi.h>
#include <olp/core/client/CancellationToken.h>

namespace olp {
namespace client {

/**
 * @brief A wrapper that manages the cancellation state of an asynchronous
 * operation in a thread-safe way.
 *
 * All public APIs are thread-safe.
 *
 * This class is both movable and copyable.
 */
class CORE_API CancellationContext {
 public:
  /**
   * @brief The type alias of the operation function.
   */
  using ExecuteFuncType = std::function<CancellationToken()>;
  /**
   * @brief The type alias of the cancel function.
   */
  using CancelFuncType = std::function<void()>;

  CancellationContext();

  /**
   * @brief Executes the given cancellable code block if the operation is not
   * cancelled.
   *
   * Otherwise, executes the custom cancellation function.
   *
   * @param execute_fn The function that should be executed if this operation is
   * not cancelled. This function should return `CancellationToken` for which
   * `CancellationContext` propagates a cancel request.
   * @param cancel_fn The function that is called if this operation is
   * cancelled.
   *
   * @return True if the execution is successful; false if the context was
   * cancelled.
   */
  bool ExecuteOrCancelled(const ExecuteFuncType& execute_fn,
                          const CancelFuncType& cancel_fn = nullptr);

  /**
   * @brief Cancels the ongoing operation in a thread-safe way.
   */
  void CancelOperation();

  /**
   * @brief Checks whether this context is cancelled.
   *
   * @return True if the context is cancelled; false otherwise.
   */
  bool IsCancelled() const;

  /**
   * @brief Overload operator < for CancellationContext.
   *
   * @param other CancellationContext object.
   *
   * @return True if the context less than other value.
   */
  bool operator<(const CancellationContext& other);

 private:
  /**
   * @brief An implementation used to shared the `CancellationContext` instance.
   */
  struct CancellationContextImpl {
    /**
     * @brief The mutex lock used to protect the `CancellationContext` object
     * from the concurrent read and write operations.
     */
    mutable std::recursive_mutex mutex_;
    /**
     * @brief The suboperation context returned from `execute_fn` of
     * `ExecuteOrCancelled()`.
     *
     * @deprecated Will be removed. Use `TaskScheduler` instead.
     */
    CancellationToken sub_operation_cancel_token_{};
    /**
     * @brief The flag that is set to `true` for `CancelOperation()`.
     */
    bool is_cancelled_{false};
  };

  /**
   * @brief The shared implementation.
   */
  std::shared_ptr<CancellationContextImpl> impl_;
};

}  // namespace client
}  // namespace olp

#include "CancellationContext.inl"
