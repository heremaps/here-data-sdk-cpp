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

#include <olp/core/client/OlpClientSettings.h>
#include <olp/core/thread/TaskScheduler.h>

namespace olp {
namespace dataservice {
namespace read {
namespace repository {

using CallFuncType = thread::TaskScheduler::CallFuncType;
using Priority = thread::Priority;

inline void ExecuteOrSchedule(
    const std::shared_ptr<thread::TaskScheduler>& task_scheduler,
    CallFuncType&& func, Priority priority) {
  if (!task_scheduler) {
    // User didn't specify a TaskScheduler, execute sync
    func();
  } else {
    task_scheduler->ScheduleTask(std::move(func), priority);
  }
}

inline void ExecuteOrSchedule(const client::OlpClientSettings* settings,
                              CallFuncType&& func, Priority priority) {
  ExecuteOrSchedule(settings ? settings->task_scheduler : nullptr,
                    std::move(func), priority);
}

}  // namespace repository
}  // namespace read
}  // namespace dataservice
}  // namespace olp
