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

#include <memory>

namespace olp {
namespace thread {

class ExecutionContext;
class TaskScheduler;

template <typename ResultType>
void Continuation<ResultType>::Run() {
  if (!finally_callback_) {
    impl_.Clear();
    return;
  }

  if (impl_.Cancelled()) {
    finally_callback_(client::ApiError::Cancelled());
    impl_.Clear();
    finally_callback_ = nullptr;
    return;
  }

  impl_.Run([this](void* input, bool cancelled) {
    if (cancelled) {
      finally_callback_(client::ApiError::Cancelled());
    } else {
      auto value = std::move(*static_cast<ResultType*>(input));
      finally_callback_(std::move(value));
    }
    finally_callback_ = nullptr;
  });
}

template <typename ResultType>
Continuation<ResultType>& Continuation<ResultType>::Finally(
    FinallyCallbackType finally_callback) {
  finally_callback_ = std::move(finally_callback);
  impl_.SetFailedCallback(
      [this](client::ApiError error) { finally_callback_(std::move(error)); });
  return *this;
}

template <typename NewType>
internal::ContinuationImpl::ContinuationTask Continuation<void>::ToAsyncTask(
    ExecutionContext context,
    std::function<void(ExecutionContext, std::function<void(NewType)>)> func) {
  using NewResultType = internal::RemoveRefAndConst<NewType>;
  return {[=](void*, CallbackType callback) {
            func(context, [callback](NewResultType input) {
              callback(static_cast<NewResultType*>(&input));
            });
          },
          [](void* input) {
            auto in = *static_cast<NewResultType*>(input);
            auto result = std::make_unique<NewResultType>(std::move(in));
            return internal::MakeUntypedUnique(std::move(result));
          }};
}

template <typename Callable>
Continuation<internal::AsyncResultType<Callable>> Continuation<void>::Then(
    Callable task) {
  using NewResultType = internal::AsyncResultType<Callable>;
  using Function = internal::TypeToFunctionInput<NewResultType>;
  return Then(std::function<void(Function)>(std::forward<Callable>(task)));
}

template <typename ResultType>
template <typename Callable>
Continuation<internal::DeducedType<Callable>> Continuation<ResultType>::Then(
    Callable task) {
  using NewResultType = internal::DeducedType<Callable>;
  using Function = internal::TypeToFunctionInput<NewResultType>;
  return Then(std::function<void(ExecutionContext, ResultType, Function)>(
      std::move(task)));
}

template <typename ResultType>
template <typename NewType>
Continuation<internal::RemoveRefAndConst<NewType>>
Continuation<ResultType>::Then(std::function<void(ExecutionContext, ResultType,
                                                  std::function<void(NewType)>)>
                                   task) {
  using NewResultType = internal::RemoveRefAndConst<NewType>;
  auto context = impl_.GetExecutionContext();

  return impl_.Then({[=](void* input, CallbackType callback) {
                       auto in = *static_cast<ResultType*>(input);
                       task(std::move(context), std::move(in),
                            [callback, context](NewResultType arg) {
                              callback(static_cast<void*>(&arg));
                            });
                     },
                     [](void* input) {
                       auto in = *static_cast<NewResultType*>(input);
                       auto result =
                           std::make_unique<NewResultType>(std::move(in));
                       return internal::MakeUntypedUnique(std::move(result));
                     }});
}

template <typename ResultType>
client::CancellationToken Continuation<ResultType>::CancelToken() {
  if (!finally_callback_) {
    return client::CancellationToken();
  }

  auto context = impl_.GetContext();
  return client::CancellationToken(
      [context]() mutable { context.CancelOperation(); });
}

template <typename ResultType>
Continuation<ResultType>::Continuation(
    std::shared_ptr<TaskScheduler> scheduler, ExecutionContext context,
    std::function<void(ExecutionContext, std::function<void(ResultType)>)> task)
    : impl_(scheduler, context,
            Continuation<void>::ToAsyncTask(context, std::move(task))) {}

template <typename ResultType>
Continuation<ResultType>::Continuation(ContinuationImplType continuation)
    : impl_(std::move(continuation)) {}

}  // namespace thread
}  // namespace olp
