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

#include "olp/core/thread/ThreadPoolTaskScheduler.h"

#if defined(PORTING_PLATFORM_QNX)
#include <process.h>
#elif defined(PORTING_PLATFORM_MAC)
#include <pthread.h>
#elif defined(PORTING_PLATFORM_LINUX) || defined(PORTING_PLATFORM_ANDROID)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <pthread.h>
#endif
#include <string>

#include "olp/core/logging/Log.h"
#include "olp/core/porting/make_unique.h"
#include "olp/core/porting/platform.h"
#include "olp/core/thread/SyncQueue.h"
#include "olp/core/utils/WarningWorkarounds.h"
#include "thread/PriorityQueueExtended.h"

namespace olp {
namespace thread {

namespace {
constexpr auto kLogTag = "ThreadPoolTaskScheduler";

void SetCurrentThreadName(const std::string& thread_name) {
  // Currently only supported for pthread users
  OLP_SDK_CORE_UNUSED(thread_name);

#if defined(PORTING_PLATFORM_MAC)
  // Note that in Mac based systems the pthread_setname_np takes 1 argument
  // only.
  pthread_setname_np(thread_name.c_str());
#elif defined(OLP_SDK_HAVE_PTHREAD_SETNAME_NP)  // Linux, Android, QNX
  // QNX allows 100 but Linux only 16 so select min value and apply for both.
  // If maximum length is exceeded on some systems, e.g. Linux, the name is not
  // set at all. So better truncate it to have at least the minimum set.
  constexpr size_t kMaxThreadNameLength = 16u;
  std::string truncated_name = thread_name.substr(0, kMaxThreadNameLength - 1);
  pthread_setname_np(pthread_self(), truncated_name.c_str());
#endif  // OLP_SDK_HAVE_PTHREAD_SETNAME_NP
}

struct PrioritizedTask {
  TaskScheduler::CallFuncType function;
  uint32_t priority;
};

struct ComparePrioritizedTask {
  bool operator()(const PrioritizedTask& lhs,
                  const PrioritizedTask& rhs) const {
    return lhs.priority < rhs.priority;
  }
};

}  // namespace

class ThreadPoolTaskScheduler::QueueImpl {
 public:
  using ElementType = PrioritizedTask;

  bool Pull(ElementType& element) { return sync_queue_.Pull(element); }

  void Push(ElementType&& element) {
    sync_queue_.Push(std::forward<ElementType>(element));
  }

  void Close() { sync_queue_.Close(); }

 private:
  using PriorityQueue =
      PriorityQueueExtended<ElementType, ComparePrioritizedTask>;
  SyncQueue<ElementType, PriorityQueue> sync_queue_;
};

ThreadPoolTaskScheduler::ThreadPoolTaskScheduler(size_t thread_count) {
  queue_ = std::make_unique<QueueImpl>();

  thread_pool_.reserve(thread_count);

  for (size_t idx = 0; idx < thread_count; ++idx) {
    std::thread executor([this, idx]() {
      // Set thread name for easy profiling and debugging
      std::string thread_name = "OLPSDKPOOL_" + std::to_string(idx);
      SetCurrentThreadName(thread_name);
      OLP_SDK_LOG_INFO_F(kLogTag, "Starting thread '%s'", thread_name.c_str());

      for (;;) {
        PrioritizedTask task;
        if (!queue_->Pull(task)) {
          return;
        }
        task.function();
      }
    });

    thread_pool_.push_back(std::move(executor));
  }
}

ThreadPoolTaskScheduler::~ThreadPoolTaskScheduler() {
  queue_->Close();
  for (auto& thread : thread_pool_) {
    thread.join();
  }
  thread_pool_.clear();
}

void ThreadPoolTaskScheduler::EnqueueTask(TaskScheduler::CallFuncType&& func) {
  queue_->Push({std::move(func), thread::NORMAL});
}

void ThreadPoolTaskScheduler::EnqueueTask(TaskScheduler::CallFuncType&& func,
                                          uint32_t priority) {
  queue_->Push({std::move(func), priority});
}

}  // namespace thread
}  // namespace olp
