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

#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/PendingRequests.h>
#include <olp/core/thread/TaskScheduler.h>

namespace olp {
namespace dataservice {
namespace write {

/*
 * @brief Common function used to wrap a lambda function and a callback that
 * consumes the function result with a TaskContext class and schedule this to a
 * task scheduler.
 * @param task_scheduler Task scheduler instance.
 * @param pending_requests PendingRequests instance that tracks current
 * requests.
 * @param task Function that will be executed.
 * @param callback Function that will consume task output.
 * @param args Additional agrs to pass to TaskContext.
 * @return CancellationToken used to cancel the operation.
 */
template <typename Function, typename Callback, typename... Args>
inline client::CancellationToken AddTask(
    const std::shared_ptr<thread::TaskScheduler>& task_scheduler,
    const std::shared_ptr<client::PendingRequests>& pending_requests,
    Function task, Callback callback, Args&&... args) {
  auto context = client::TaskContext::Create(
      std::move(task), std::move(callback), std::forward<Args>(args)...);
  pending_requests->Insert(context);

  auto scheduler_func = [=] {
    context.Execute();
    pending_requests->Remove(context);
  };

  if (!task_scheduler) {
    // User didn't specify a TaskScheduler, execute sync
    scheduler_func();
  } else {
    task_scheduler->ScheduleTask(std::move(scheduler_func));
  }

  return context.CancelToken();
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
