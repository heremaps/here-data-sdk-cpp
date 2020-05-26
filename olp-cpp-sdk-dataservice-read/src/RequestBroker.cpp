/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
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

#include "RequestBroker.h"

namespace olp {
namespace dataservice {
namespace read {

namespace {
std::string UniqueId() {
  // TODO: generate a better unique id
  static unsigned int id = 0;
  return std::to_string(id++);
}
}  // namespace

void RequestBroker::RequestContext::AddCallback(CallerId id,
                                                Callback callback) {
  callbacks[id] = std::move(callback);
}

void RequestBroker::RequestContext::PropagateResponse(DataResponse response) {
  for (auto& callback : callbacks) {
    callback.second(response);
  }
  callbacks.clear();
}

// Return true if the operation was canceled
bool RequestBroker::RequestContext::CancelRequest(CallerId id) {
  // Cancel individual request
  {
    auto callback_it = callbacks.find(id);
    if (callback_it != callbacks.end()) {
      Callback callback = std::move(callback_it->second);
      callback(client::ApiError(client::ErrorCode::Cancelled, "Canceled"));
      callbacks.erase(callback_it);
    } else {
      assert(false);
    }
  }

  const bool cancel_operation = callbacks.empty();

  if (cancel_operation) {
    cancelation_context.CancelOperation();
  }

  return cancel_operation;
}

client::CancellationContext
RequestBroker::RequestContext::CancelationContext() {
  return cancelation_context;
}

RequestBroker::CreateOrAssociateResult RequestBroker::CreateOrAssociateRequest(
    RequestId req_id, Callback callback) {
  const CallerId caller_id = UniqueId();
  GetOrCreateResult result = GetOrCreateContext(req_id);
  result.ctx.AddCallback(caller_id, std::move(callback));
  return {result.ctx.CancelationContext(), CancelToken(req_id, caller_id),
          result.just_created};
}

DataResponseCallback RequestBroker::ResponseHandler(RequestId req_id) {
  return [=](DataResponse response) {
    PropagateResponse(req_id, std::move(response));
  };
}

RequestBroker::GetOrCreateResult RequestBroker::GetOrCreateContext(
    RequestId req_id) {
  std::unique_lock<std::mutex> lock(mutex_);

  auto request_ctx_it = request_map_.find(req_id);
  if (request_ctx_it != request_map_.end()) {
    return {request_ctx_it->second, false};
  } else {
    request_ctx_it =
        request_map_.insert(std::make_pair(req_id, RequestContext{})).first;
    return {request_ctx_it->second, true};
  }
}

void RequestBroker::PropagateResponse(RequestId req_id, DataResponse response) {
  std::unique_lock<std::mutex> lock(mutex_);

  auto request_ctx_it = request_map_.find(req_id);
  if (request_ctx_it == request_map_.end()) {
    assert(!response.IsSuccessful()); // Expect cancel here
    return;
  }

  auto ctx = std::move(request_ctx_it->second);
  request_map_.erase(request_ctx_it);
  ctx.PropagateResponse(std::move(response));
}

void RequestBroker::CancelRequest(RequestId req_id, CallerId id) {
  std::unique_lock<std::mutex> lock(mutex_);

  auto request_ctx_it = request_map_.find(req_id);
  if (request_ctx_it == request_map_.end()) {
    assert(false);
    return;
  }

  RequestContext& ctx = request_ctx_it->second;
  if (ctx.CancelRequest(id)) {
    request_map_.erase(request_ctx_it);
  }
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
