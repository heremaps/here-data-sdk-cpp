/*
 * Copyright (C) 2022 HERE Europe B.V.
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

#include <utility>

#include <olp/core/client/ApiError.h>
#include <olp/core/client/CancellationContext.h>

namespace olp {
namespace thread {

/// Handles the cancellation and final mechanisms.
class CORE_API ExecutionContext final {
  using CancelFuncType = std::function<void()>;
  using ExecuteFuncType = std::function<client::CancellationToken()>;
  using FailedCallback = std::function<void(client::ApiError)>;

 public:
  /// A default contructor, initializes the `ExecutionContextImpl` instance.
  ExecutionContext();

  /**
   * @brief Checks whether `CancellationContext` is cancelled.
   *
   * @return True if `CancellationContext` is cancelled; false otherwise.
   */
  bool Cancelled() const;

  /// @copydoc CancellationContext::CancelOperation()
  void CancelOperation();

  /// @copydoc CancellationContext::ExecuteOrCancelled()
  void ExecuteOrCancelled(const ExecuteFuncType& execute_fn,
                          const CancelFuncType& cancel_fn = nullptr);

  /**
   * @brief Sets the error that is returned in the `Finally`
   * method of the execution.
   *
   * It immediately finishes the task execution and provides an error
   * via the `SetFailedCallback` callback of `ExecutionContext`.
   *
   * @param error The `ApiError` instance containing the error information.
   */
  void SetError(client::ApiError error);

  /**
   * @brief Sets a callback for `SetError`.
   *
   * @param callback Handles the finalization of the execution
   * in case of an error.
   */
  void SetFailedCallback(FailedCallback callback);

  /**
   * @brief Gets the `CancellationContext` object associated
   * with this `ExecutionContext` instance.
   *
   * The caller can use it to cancel the ongoing operation.
   *
   * @return The `CancellationContext` instance.
   */
  client::CancellationContext GetContext() const;

 private:
  class ExecutionContextImpl;
  std::shared_ptr<ExecutionContextImpl> impl_;
};

}  // namespace thread
}  // namespace olp
