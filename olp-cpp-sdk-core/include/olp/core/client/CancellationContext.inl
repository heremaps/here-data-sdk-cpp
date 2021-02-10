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

namespace olp {
namespace client {

inline CancellationContext::CancellationContext()
    : impl_(std::make_shared<CancellationContextImpl>()) {}

inline bool CancellationContext::ExecuteOrCancelled(
    const std::function<CancellationToken()>& execute_fn,
    const std::function<void()>& cancel_fn) {
  if (!impl_) {
    return true;
  }

  std::lock_guard<std::recursive_mutex> lock(impl_->mutex_);

  if (impl_->is_cancelled_) {
    if (cancel_fn) {
      cancel_fn();
    }
    return false;
  }

  if (execute_fn) {
    impl_->sub_operation_cancel_token_ = execute_fn();
  }

  return true;
}

inline void CancellationContext::CancelOperation() {
  if (!impl_) {
    return;
  }

  std::lock_guard<std::recursive_mutex> lock(impl_->mutex_);
  if (impl_->is_cancelled_) {
    return;
  }

  impl_->sub_operation_cancel_token_.Cancel();
  impl_->sub_operation_cancel_token_ = CancellationToken();
  impl_->is_cancelled_ = true;
}

inline bool CancellationContext::IsCancelled() const {
  if (!impl_) {
    return false;
  }

  std::lock_guard<std::recursive_mutex> lock(impl_->mutex_);
  return impl_->is_cancelled_;
}

inline size_t CancellationContextHash::operator()(
    const CancellationContext& context) const {
  return std::hash<
      std::shared_ptr<CancellationContext::CancellationContextImpl>>()(
      context.impl_);
}

}  // namespace client
}  // namespace olp
