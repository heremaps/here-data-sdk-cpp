/*
 * Copyright (C) 2020-2021 HERE Europe B.V.
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

#include <boost/optional.hpp>

#include <olp/core/client/CancellationToken.h>
#include <olp/core/client/PendingRequests.h>
#include <olp/core/client/TaskContext.h>
#include <olp/core/thread/TaskScheduler.h>

namespace olp {
namespace dataservice {
namespace read {

class TaskSink {
 public:
  explicit TaskSink(std::shared_ptr<thread::TaskScheduler> task_scheduler);

  TaskSink(const TaskSink&) = delete;
  TaskSink& operator=(const TaskSink&) = delete;

  ~TaskSink();

  void CancelTasks();

  client::CancellationToken AddTask(
      std::function<void(client::CancellationContext)> func, uint32_t priority,
      client::CancellationContext context);

  template <typename Function, typename Callback, typename... Args>
  client::CancellationToken AddTask(Function task, Callback callback,
                                    uint32_t priority, Args&&... args) {
    auto context = client::TaskContext::Create(
        std::move(task), std::move(callback), std::forward<Args>(args)...);
    AddTaskImpl(context, priority);
    return context.CancelToken();
  }

  template <typename Function, typename Callback, typename... Args>
  boost::optional<client::CancellationToken> AddTaskChecked(Function task,
                                                            Callback callback,
                                                            uint32_t priority,
                                                            Args&&... args) {
    auto context = client::TaskContext::Create(
        std::move(task), std::move(callback), std::forward<Args>(args)...);
    if (!AddTaskImpl(context, priority)) {
      return boost::none;
    }
    return context.CancelToken();
  }

 protected:
  bool AddTaskImpl(client::TaskContext task, uint32_t priority);

  bool ScheduleTask(client::TaskContext task, uint32_t priority);

  void ExecuteTask(client::TaskContext task);

  const std::shared_ptr<thread::TaskScheduler> task_scheduler_;
  const std::shared_ptr<client::PendingRequests> pending_requests_;
  std::mutex mutex_;
  bool closed_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
