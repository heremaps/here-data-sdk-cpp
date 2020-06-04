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

#include <string>
#include <utility>
#include <vector>

#include <rapidjson/document.h>

#include "olp/authentication/ErrorResponse.h"

#include "olp/authentication/AuthenticationSettings.h"
#include "olp/authentication/AuthorizeResult.h"
#include "olp/authentication/IntrospectAppResult.h"
#include "olp/core/client/OlpClient.h"
#include "olp/core/client/PendingRequests.h"
#include "olp/core/thread/TaskScheduler.h"

namespace olp {
namespace authentication {

void ExecuteOrSchedule(
    const std::shared_ptr<olp::thread::TaskScheduler>& task_scheduler,
    olp::thread::TaskScheduler::CallFuncType&& func);

/*
 * @brief Common function used to wrap a lambda function and a callback that
 * consumes the function result with a TaskContext class and schedule this to a
 * task scheduler.
 * @param task_scheduler Task scheduler instance.
 * @param pending_requests PendingRequests instance that tracks current
 * requests.
 * @param task Function that will be executed.
 * @param callback Function that will consume task output.
 * @param args Additional agrs to pass to TaskContext.
 * @return CancellationToken used to cancel the operation.
 */
template <typename Function, typename Callback, typename... Args>
inline client::CancellationToken AddTask(
    const std::shared_ptr<olp::thread::TaskScheduler>& task_scheduler,
    const std::shared_ptr<client::PendingRequests>& pending_requests,
    Function task, Callback callback, Args&&... args) {
  auto context = client::TaskContext::Create(
      std::move(task), std::move(callback), std::forward<Args>(args)...);
  pending_requests->Insert(context);

  ExecuteOrSchedule(task_scheduler, [=] {
    context.Execute();
    pending_requests->Remove(context);
  });

  return context.CancelToken();
}

std::time_t GetTimestampFromHeaders(const olp::http::Headers& headers);
IntrospectAppResult GetIntrospectAppResult(const rapidjson::Document& doc);
DecisionType GetPermission(const std::string& str);
std::vector<ActionResult> GetDiagnostics(rapidjson::Document& doc);
AuthorizeResult GetAuthorizeResult(rapidjson::Document& doc);
std::time_t ParseTime(const std::string& value);
olp::client::OlpClient CreateOlpClient(
    const AuthenticationSettings& auth_settings,
    boost::optional<olp::client::AuthenticationSettings>
        authentication_settings);

}  // namespace authentication
}  // namespace olp
