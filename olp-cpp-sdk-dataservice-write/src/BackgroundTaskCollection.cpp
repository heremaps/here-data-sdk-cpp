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

#include "BackgroundTaskCollection.h"
#include <algorithm>
#include <chrono>

namespace olp {
namespace dataservice {
namespace write {
template <typename T>
BackgroundTaskCollection<T>::BackgroundTaskCollection() = default;

template <typename T>
T BackgroundTaskCollection<T>::AddTask() {
  static T id = T{};

  std::lock_guard<std::mutex> lock(mutex_);
  auto myId = id++;
  background_tasks_ids_.push_back(myId);
  return myId;
}

template <typename T>
void BackgroundTaskCollection<T>::ReleaseTask(T id) {
  std::lock_guard<std::mutex> lock(mutex_);
  background_tasks_ids_.erase(
      std::remove_if(std::begin(background_tasks_ids_),
                     std::end(background_tasks_ids_),
                     [id](const size_t& taskId) { return taskId == id; }),
      std::end(background_tasks_ids_));
  cond_var_.notify_all();
}

template <typename T>
size_t BackgroundTaskCollection<T>::Size() {
  std::unique_lock<std::mutex> lock{mutex_};
  return background_tasks_ids_.size();
}

template <typename T>
void BackgroundTaskCollection<T>::WaitForBackgroundTaskCompletion() {
  std::unique_lock<std::mutex> lock{mutex_};
  cond_var_.wait(lock, [&] { return background_tasks_ids_.empty(); });
}

template <typename T>
template <typename _Rep, typename _Period>
void BackgroundTaskCollection<T>::WaitForBackgroundTaskCompletion(
    const std::chrono::duration<_Rep, _Period>& timeout) {
  std::unique_lock<std::mutex> lock{mutex_};
  cond_var_.wait_for(lock, timeout,
                     [&] { return background_tasks_ids_.empty(); });
}

template class BackgroundTaskCollection<size_t>;
template void BackgroundTaskCollection<size_t>::WaitForBackgroundTaskCompletion(
    const std::chrono::milliseconds& timeout);

}  // namespace write
}  // namespace dataservice
}  // namespace olp
