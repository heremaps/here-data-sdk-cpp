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

#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/PendingRequests.h>
#include <olp/core/thread/TaskScheduler.h>
#include <olp/dataservice/read/FetchOptions.h>
#include "repositories/ExecuteOrSchedule.inl"

namespace olp {
namespace dataservice {
namespace read {

/*
 * @brief Common function to perform task separation based on fetch option.
 * When user specifies CacheWithUpdate request, we start 2 tasks:
 * - perform cache lookup and return user the result
 * - perform online request and update cache
 * @param schedule_task Function used to setup a task, consumes the request
 * and user callback.
 * @param request Request provided by the user.
 * @param callback Operation callback specified by the user.
 * @return CancellationToken used to cancel the operation.
 */
template <class TaskScheduler, class Request, class Callback>
inline client::CancellationToken ScheduleFetch(TaskScheduler&& schedule_task,
                                               Request&& request,
                                               Callback&& callback) {
  if (request.GetFetchOption() == FetchOptions::CacheWithUpdate) {
    auto cache_token =
        schedule_task(request.WithFetchOption(FetchOptions::CacheOnly),
                      std::forward<Callback>(callback));
    auto online_token = schedule_task(
        std::move(request.WithFetchOption(FetchOptions::CacheWithUpdate)),
        nullptr);

    return client::CancellationToken([=]() {
      cache_token.Cancel();
      online_token.Cancel();
    });
  }

  return schedule_task(std::forward<Request>(request),
                       std::forward<Callback>(callback));
}

/*
 * @brief Common function used to wrap a lambda function and a callback that
 * consumes the function result with a TaskContext class and schedule this to a
 * task scheduler.
 * @param task_scheduler Task scheduler instance.
 * @param pending_requests PendingRequests instance that tracks current
 * requests.
 * @param task Function that will be executed.
 * @param callback Function that will consume task output.
 * @param priority Priority of the task.
 * @param args Additional agrs to pass to TaskContext.
 * @return CancellationToken used to cancel the operation.
 */
template <typename Function, typename Callback, typename... Args>
inline client::CancellationToken AddTaskWithPriority(
    const std::shared_ptr<thread::TaskScheduler>& task_scheduler,
    const std::shared_ptr<client::PendingRequests>& pending_requests,
    Function task, Callback callback, uint32_t priority, Args&&... args) {
  auto context = client::TaskContext::Create(
      std::move(task), std::move(callback), std::forward<Args>(args)...);
  pending_requests->Insert(context);

  repository::ExecuteOrSchedule(task_scheduler,
                                [=] {
                                  context.Execute();
                                  pending_requests->Remove(context);
                                },
                                priority);

  return context.CancelToken();
}

/*
 * @brief Common function used to wrap a lambda function and a callback that
 * consumes the function result with a TaskContext class and schedule this to a
 * task scheduler with NORMAL priority.
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
  return AddTaskWithPriority(task_scheduler, pending_requests, task, callback,
                             thread::NORMAL, std::forward<Args>(args)...);
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
