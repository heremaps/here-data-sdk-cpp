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

#include <olp/core/thread/Continuation.h>

#include <atomic>
#include <memory>
#include <thread>
#include <utility>

#include <olp/core/client/CancellationToken.h>
#include <olp/core/thread/SyncQueue.h>
#include <olp/core/thread/TaskScheduler.h>

namespace olp {
namespace thread {

using internal::ContinuationImpl;

namespace internal {

class Processor {
  using Task = std::function<void()>;
  struct ProcessorInternal;

 public:
  Processor(ExecutionContext execution_context,
            std::deque<ContinuationImpl::ContinuationTask> tasks,
            ContinuationImpl::FinalCallbackType final_callback)
      : processor_(std::make_shared<ProcessorInternal>(
            std::move(tasks), std::move(final_callback),
            std::move(execution_context))) {}

  // returns false when the current task is over and the queue should proceed
  // further, true means do nothing
  bool ProcessAsyncTask(std::shared_ptr<ProcessorInternal> process,
                        std::pair<ContinuationImpl::AsyncTaskType,
                                  ContinuationImpl::TaskType>& task) {
    auto& current_task = task;
    if (!current_task.first) {
      // go to the next task in the queue to finilize the task
      process->tasks_.pop_front();
      return false;
    }

    auto callback = [=](void* arg) mutable {
      // limit lifetime of the context shared_ptr to the function scope, for
      // extra safety in case user's async function stores callback somewhere
      // for forever.
      auto context = process;
      // save the arg (the identity function)
      context->last_output_ = current_task.second(arg);

      // re-enter ProcessAsyncTask() in order to finilize the task.
      if (context->last_output_) {
        ProcessNextTask(context);
      }
    };

    auto func = std::move(current_task.first);
    current_task.first = nullptr;
    func(process->LastOutput(), std::move(callback));
    if (process->public_execution_context_.Cancelled()) {
      return false;
    }

    return true;
  }

  void FinishQueueExecution(const std::shared_ptr<ProcessorInternal>& context) {
    if (context->final_callback_) {
      // the final execution context, all tasks are done or the execution
      // is cancelled
      context->final_callback_(context->LastOutput(), context->IsCancelled());
      context->final_callback_ = nullptr;
      context->tasks_.clear();
    }
  }

  // the function is called iteratively for the same ExecutionContext until all
  // tasks are processed
  void ProcessNextTask(const std::shared_ptr<ProcessorInternal>& context) {
    if (!context->tasks_.empty() && !context->IsCancelled()) {
      // run the task
      auto& task = context->tasks_.front();
      if (ProcessAsyncTask(context, task)) {
        return;
      }
    }

    if (context->tasks_.empty() || context->IsCancelled()) {
      FinishQueueExecution(context);
    } else {
      auto task_to_execute = CreateFollowingTask(context);
      task_to_execute();
    }
  }

  // creates a task for execution by TaskContinuation
  Task CreateFollowingTask(const std::shared_ptr<ProcessorInternal>& context) {
    return [=] { ProcessNextTask(context); };
  }

  // starts the first task in a chain
  void Start() {
    auto current_task_queue_element = CreateFollowingTask(processor_);
    current_task_queue_element();
  }

 private:
  struct ProcessorInternal {
    // a chain of tasks to execute
    std::deque<ContinuationImpl::ContinuationTask> tasks_;

    // a callback is being be executed at the end of the queue
    ContinuationImpl::FinalCallbackType final_callback_;

    // the result of the previous task, used as an input for the current task
    ContinuationImpl::OutResultType last_output_;

    // the public execution context provides functionality to cancel or finish
    // an execution
    ExecutionContext public_execution_context_;

    ProcessorInternal(std::deque<ContinuationImpl::ContinuationTask> tasks,
                      ContinuationImpl::FinalCallbackType&& final_callback,
                      ExecutionContext execution_context)
        : tasks_(std::move(tasks)),
          final_callback_(std::move(final_callback)),
          public_execution_context_(execution_context) {}

    // checks whether the Continuation is cancelled
    bool IsCancelled() const { return public_execution_context_.Cancelled(); }

    // An input argument used by the current task in the queue, it's the result
    // of the previous task
    void* LastOutput() const {
      return last_output_ ? last_output_->Get() : nullptr;
    }
  };

  std::shared_ptr<ProcessorInternal> processor_;
};

ContinuationImpl::ContinuationImpl(
    std::shared_ptr<TaskScheduler> task_scheduler, ExecutionContext context,
    ContinuationTask task)
    : task_scheduler_(std::move(task_scheduler)), execution_context_(context) {
  if (change_allowed) {
    tasks_.push_back(std::move(task));
  }
}

ContinuationImpl ContinuationImpl::Then(ContinuationTask task) {
  if (change_allowed) {
    tasks_.push_back(std::move(task));
  }
  return *this;
}

void ContinuationImpl::Run(FinalCallbackType callback) {
  if (change_allowed) {
    change_allowed = false;
    task_scheduler_->ScheduleTask([this, callback]() {
      Processor processor(execution_context_, std::move(tasks_),
                          std::move(callback));
      processor.Start();
    });
  }
}

const ExecutionContext& ContinuationImpl::GetExecutionContext() const {
  return execution_context_;
}

bool ContinuationImpl::Cancelled() const {
  return execution_context_.Cancelled();
}

void ContinuationImpl::SetFailedCallback(FailedCallback callback) {
  if (change_allowed) {
    execution_context_.SetFailedCallback(std::move(callback));
  }
}

void ContinuationImpl::Clear() {
  if (change_allowed) {
    tasks_.clear();
    change_allowed = false;
  }
}

}  // namespace internal
}  // namespace thread
}  // namespace olp
