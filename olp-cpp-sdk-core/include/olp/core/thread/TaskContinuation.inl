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

namespace olp {
namespace thread {

template <typename Callable>
Continuation<internal::AsyncResultType<Callable>> TaskContinuation::Then(
    Callable task) {
  using NewResultType = internal::AsyncResultType<Callable>;
  using Function = internal::TypeToFunctionInput<NewResultType>;
  return {task_scheduler_, execution_context_,
          std::function<void(ExecutionContext, Function)>(std::move(task))};
}

TaskContinuation::TaskContinuation(std::shared_ptr<TaskScheduler> scheduler)
    : task_scheduler_(std::move(scheduler)),
      execution_context_(ExecutionContext()) {}

}  // namespace thread
}  // namespace olp
