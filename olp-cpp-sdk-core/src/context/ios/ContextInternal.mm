/*
 * Copyright (C) 2023 HERE Europe B.V.
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

#include "context/ContextInternal.h"

#include <utility>
#include <dispatch/dispatch.h>

namespace olp {
namespace context {

void ContextInternal::SubscribeEnterBackground(
    std::weak_ptr<EnterBackgroundSubscriber> subscriber) {
  auto cd = Instance();
  std::lock_guard<std::mutex> lock(cd->context_mutex);

  auto& clients = cd->enter_background_subscibers;
  clients.push_back(std::move(subscriber));
}

void ContextInternal::UnsubscribeEnterBackground(
    std::weak_ptr<EnterBackgroundSubscriber> subscriber) {
  auto cd = Instance();
  std::lock_guard<std::mutex> lock(cd->context_mutex);

  auto& clients = cd->enter_background_subscibers;
  auto to_remove = subscriber.lock();
  if (!to_remove) {
    return;
  }
  clients.remove_if(
      [&](std::weak_ptr<EnterBackgroundSubscriber>& client) -> bool {
        auto locked_client = client.lock();
        return !locked_client || to_remove == locked_client;
      });
}

void ContextInternal::CallBackgroundSessionCompletionHandler(
    const std::string& session_name) {
  auto cd = Instance();
  std::lock_guard<std::mutex> lock(cd->context_mutex);

  auto& completion_handlers = cd->completion_handlers;
  auto found = completion_handlers.find(session_name);
  if (found != completion_handlers.end()) {
    auto completion_handler{std::move(found->second)};
    dispatch_async(dispatch_get_main_queue(), ^{
        completion_handler();
    });
    completion_handlers.erase(session_name);
  }
}

}  // namespace context
}  // namespace olp
