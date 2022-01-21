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

#include <olp/core/thread/ExecutionContext.h>

namespace olp {
namespace thread {

class ExecutionContext::ExecutionContextImpl {
 public:
  bool IsCancelled() const { return context_.IsCancelled(); }

  void CancelOperation() { context_.CancelOperation(); }

  void ExecuteOrCancelled(const ExecuteFuncType& execute_fn,
                          const CancelFuncType& cancel_fn) {
    context_.ExecuteOrCancelled(execute_fn, cancel_fn);
  }

  void SetError(client::ApiError error) {
    if (failed_callback_) {
      failed_callback_(std::move(error));
      failed_callback_ = nullptr;
    }
  }

  void SetFailedCallback(FailedCallback callback) {
    failed_callback_ = std::move(callback);
  }

  const client::CancellationContext& GetContext() const { return context_; }

 private:
  client::CancellationContext context_;
  FailedCallback failed_callback_;
};

ExecutionContext::ExecutionContext()
    : impl_(std::make_shared<ExecutionContextImpl>()) {}

void ExecutionContext::SetError(client::ApiError error) {
  impl_->SetError(std::move(error));
}

bool ExecutionContext::Cancelled() const { return impl_->IsCancelled(); }

void ExecutionContext::CancelOperation() { return impl_->CancelOperation(); }

void ExecutionContext::ExecuteOrCancelled(const ExecuteFuncType& execute_fn,
                                          const CancelFuncType& cancel_fn) {
  impl_->ExecuteOrCancelled(execute_fn, cancel_fn);
}

void ExecutionContext::SetFailedCallback(FailedCallback callback) {
  impl_->SetFailedCallback(std::move(callback));
}

const client::CancellationContext& ExecutionContext::GetContext() const {
  return impl_->GetContext();
}

}  // namespace thread
}  // namespace olp
