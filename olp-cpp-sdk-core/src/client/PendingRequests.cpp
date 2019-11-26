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

#include <olp/core/client/PendingRequests.h>
#include <olp/core/client/TaskContext.h>
#include <olp/core/logging/Log.h>

namespace olp {
namespace client {

namespace {
constexpr auto kLogTag = "PendingRequests";
}

bool PendingRequests::CancelAll() {
  ContextMap contexts;
  {
    std::lock_guard<std::mutex> lock(task_contexts_lock_);
    contexts = task_contexts_;
  }

  for (auto context : contexts) {
    context.CancelToken().Cancel();
  }

  return true;
}

bool PendingRequests::CancelAllAndWait() {
  CancelAll();

  ContextMap contexts;
  {
    std::lock_guard<std::mutex> lock(task_contexts_lock_);
    contexts = std::move(task_contexts_);
  }

  for (auto context : contexts) {
    if (!context.BlockingCancel()) {
      OLP_SDK_LOG_WARNING(kLogTag, "Timeout, when waiting on BlockingCancel");
    }
  }

  return true;
}

void PendingRequests::Insert(TaskContext task_context) {
  std::lock_guard<std::mutex> lock(task_contexts_lock_);
  task_contexts_.insert(task_context);
}

void PendingRequests::Remove(TaskContext task_context) {
  std::lock_guard<std::mutex> lock(task_contexts_lock_);
  task_contexts_.erase(task_context);
}

}  // namespace client
}  // namespace olp
