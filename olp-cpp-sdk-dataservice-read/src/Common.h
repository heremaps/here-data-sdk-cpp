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

}  // namespace read
}  // namespace dataservice
}  // namespace olp
