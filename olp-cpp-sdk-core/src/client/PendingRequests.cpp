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

PendingRequests::PendingRequests(){};

PendingRequests::~PendingRequests(){};

bool PendingRequests::CancelAll() {
  RequestMap requests_map;
  ContextMap contexts;
  {
    std::lock_guard<std::mutex> lk(requests_lock_);
    requests_map = requests_map_;
    contexts = task_contexts_;
  }

  for (auto& pair : requests_map) {
    pair.second.cancel();
  }

  for (auto context : contexts) {
    context.CancelToken().cancel();
  }

  return true;
}

bool PendingRequests::CancelAllAndWait() {
  CancelAll();

  ContextMap contexts;
  {
    std::lock_guard<std::mutex> lk(requests_lock_);
    contexts = std::move(task_contexts_);
  }

  for (auto context : contexts) {
    if (!context.BlockingCancel()) {
      OLP_SDK_LOG_WARNING(kLogTag, "Timeout, when waiting on BlockingCancel");
    }
  }

  return true;
}

int64_t PendingRequests::GenerateRequestPlaceholder() {
  std::lock_guard<std::mutex> lk(requests_lock_);
  key_++;
  requests_map_[key_] = client::CancellationToken();
  return key_;
}

bool PendingRequests::Insert(const client::CancellationToken& request,
                             int64_t key) {
  std::lock_guard<std::mutex> lk(requests_lock_);
  if (requests_map_.find(key) == requests_map_.end()) {
    return false;
  }
  requests_map_[key] = request;
  return true;
}

void PendingRequests::Insert(client::TaskContext task_context) {
  std::lock_guard<std::mutex> lk(requests_lock_);
  task_contexts_.insert(task_context);
}

bool PendingRequests::Remove(int64_t key) {
  std::lock_guard<std::mutex> lk(requests_lock_);
  auto request = requests_map_.find(key);
  if (request != requests_map_.end()) {
    requests_map_.erase(request);
    return true;
  }
  return false;
}

void PendingRequests::Remove(client::TaskContext task_context) {
  std::lock_guard<std::mutex> lk(requests_lock_);
  task_contexts_.erase(task_context);
}

}  // namespace client
}  // namespace olp
