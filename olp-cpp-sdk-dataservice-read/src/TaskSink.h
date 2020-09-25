/*
 * Copyright (C) 2020 HERE Europe B.V.
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
#include <olp/core/client/TaskContext.h>
#include <olp/core/logging/Log.h>
#include <olp/core/thread/TaskScheduler.h>

#include "repositories/ExecuteOrSchedule.inl"

namespace olp {
namespace dataservice {
namespace read {

class TaskSink {
 public:
  explicit TaskSink(std::shared_ptr<thread::TaskScheduler> task_scheduler)
      : task_scheduler_(std::move(task_scheduler)),
        pending_requests_(std::make_shared<client::PendingRequests>()),
        closed_(false) {}

  TaskSink(const TaskSink&) = delete;
  TaskSink& operator=(const TaskSink&) = delete;

  ~TaskSink() {
    closed_.store(true);
    pending_requests_->CancelAllAndWait();
  }

  void CancelTasks() { pending_requests_->CancelAll(); }

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

 private:
  bool AddTaskImpl(client::TaskContext task, uint32_t priority) {
    if (closed_.load()) {
      OLP_SDK_LOG_WARNING(
          "TaskSink", "Attempt to add a task when the sink is already closed");
      return false;
    }

    pending_requests_->Insert(task);

    auto pending_requests = pending_requests_;

    repository::ExecuteOrSchedule(task_scheduler_,
                                  [=] {
                                    task.Execute();
                                    pending_requests->Remove(task);
                                  },
                                  priority);

    return true;
  }

  std::shared_ptr<thread::TaskScheduler> task_scheduler_;
  std::shared_ptr<client::PendingRequests> pending_requests_;
  std::atomic_bool closed_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
