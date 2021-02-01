/*
 * Copyright (C) Copyright (C) 2019 HERE Europe B.V.
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

#include <condition_variable>
#include <mutex>
#include <vector>

namespace olp {
namespace dataservice {
namespace write {

template <typename T>
class BackgroundTaskCollection {
 public:
  explicit BackgroundTaskCollection();

  T AddTask();

  void ReleaseTask(T id);

  size_t Size();

  void WaitForBackgroundTaskCompletion();

  template <typename _Rep, typename _Period>
  void WaitForBackgroundTaskCompletion(
      const std::chrono::duration<_Rep, _Period>& timeout);

 private:
  mutable std::mutex mutex_;
  std::vector<T> background_tasks_ids_;
  std::condition_variable cond_var_;
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
