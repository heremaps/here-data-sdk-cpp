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

#include "PendingUrlRequests.h"

#include "olp/core/logging/Log.h"

namespace {
constexpr auto kLogTag = "PendingUrlRequest";

olp::client::HttpResponse GetCancelledResponse() {
  return {olp::client::PendingUrlRequest::kCancelledStatus,
          "Operation cancelled"};
}
}  // namespace

namespace olp {
namespace client {

size_t PendingUrlRequest::Append(NetworkAsyncCallback callback) {
  // NOTE: You should not append anything if this request was cancelled
  std::lock_guard<std::mutex> lock(mutex_);
  const auto callback_id = callbacks_.size();
  callbacks_.emplace(callback_id, std::move(callback));
  return callback_id;
}

bool PendingUrlRequest::ExecuteOrCancelled(const ExecuteFuncType& func,
                                           const CancelFuncType& cancel_func) {
  // We should be able to call this multiple times in case of an retry event
  return context_.ExecuteOrCancelled(
      [&]() -> client::CancellationToken { return func(http_request_id_); },
      cancel_func);
}

bool PendingUrlRequest::Cancel(size_t callback_id) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = callbacks_.find(callback_id);
  if (it == callbacks_.end()) {
    // Unknown callback_id
    OLP_SDK_LOG_WARNING_F(
        kLogTag,
        "Cancel, unknown callback, callback_id=%zu, request_id=%" PRIu64,
        callback_id, http_request_id_);
    return false;
  }

  // Once cancelled, moved the callback out of the callbacks_ map and place it
  // in the cancelled callbacks vector.
  cancelled_callbacks_.emplace_back(std::move(it->second));
  callbacks_.erase(it);

  // If there is no more callback left in the map cancel the Network request
  if (callbacks_.empty()) {
    OLP_SDK_LOG_DEBUG_F(
        kLogTag, "Cancel, cancelling Network request, request_id=%" PRIu64,
        http_request_id_);
    CancelOperation();
  }

  return true;
}

void PendingUrlRequest::OnRequestCompleted(HttpResponse response) {
  std::map<size_t, NetworkAsyncCallback> callbacks;
  std::vector<NetworkAsyncCallback> cancelled_callbacks;
  http::RequestId request_id;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    callbacks = std::move(callbacks_);
    cancelled_callbacks = std::move(cancelled_callbacks_);
    request_id = http_request_id_;

    // In case the response happens to be one of the retryable HTTP errors
    // then we should reset the Network RequestId here so that in case there is
    // a new request to be triggered but it was cancelled by the user in the
    // meantime, so that ExecuteOrCancelled() will work properly.
    http_request_id_ = kInvalidRequestId;
  }

  OLP_SDK_LOG_DEBUG_F(kLogTag,
                      "OnRequestCompleted, request_id=%" PRIu64
                      ", callbacks=%zu, cancelled_callbacks=%zu",
                      request_id, callbacks.size(), cancelled_callbacks.size());

  client::HttpResponse response_out;
  if (!context_.IsCancelled()) {
    response_out = std::move(response);
  } else {
    response_out = GetCancelledResponse();
  }

  // If only one callback, move response instead of copy.
  if (callbacks.size() == 1u && cancelled_callbacks.empty()) {
    callbacks.begin()->second(std::move(response_out));
  } else if (cancelled_callbacks.size() == 1 && callbacks.empty()) {
    cancelled_callbacks.front()(std::move(response_out));
  } else {
    for (auto& callback : callbacks) {
      // TODO: We need to switch HttpResponse to shared pimpl once the
      // deprecation period is completed to avoid unnecessary copy of data.
      callback.second(response_out);

      // We need to reset the stringstream position else the next copy
      // constructor will not be able to read anything because the read
      // position is at the end of the stream.
      response_out.response.seekg(0, std::ios::beg);
    }

    if (!cancelled_callbacks.empty() &&
        response_out.GetStatus() != kCancelledStatus) {
      response_out = GetCancelledResponse();
    }

    for (auto& callback : cancelled_callbacks) {
      callback(response_out);

      // We need to reset the stringstream position else the next copy
      // constructor will not be able to read anything because the read
      // position is at the end of the stream.
      response_out.response.seekg(0, std::ios::beg);
    }
  }

  condition_.Notify();
}

size_t PendingUrlRequests::Size() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return pending_requests_.size() + cancelled_requests_.size();
}

bool PendingUrlRequests::Cancel(const std::string& url, size_t callback_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = pending_requests_.find(url);
  if (it == pending_requests_.end()) {
    // Nothing to cancel
    OLP_SDK_LOG_DEBUG_F(kLogTag, "Cancel, unknown, url='%s', callback_id=%zu",
                        url.c_str(), callback_id);
    return true;
  }

  auto& request_ptr = it->second;
  auto ret = request_ptr->Cancel(callback_id);

  // If the last callback was cancelled then move this to the cancelled list
  if (request_ptr->IsCancelled()) {
    cancelled_requests_.emplace(it->first, it->second);
    pending_requests_.erase(it);
  }

  return ret;
}

bool PendingUrlRequests::CancelAll() {
  // This only cancells the ongoing Network request the callback trigger
  // is taking care of the Network callback.
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto& request : pending_requests_) {
    request.second->CancelOperation();
  }

  return true;
}

bool PendingUrlRequests::CancelAllAndWait() const {
  PendingRequestsType pending_requests;
  PendingRequestsType cancelled_requests;

  {
    // Copy, do not move else the OnRequestCompleted callback
    // will not work
    std::lock_guard<std::mutex> lock(mutex_);
    if (pending_requests_.empty() && cancelled_requests_.empty()) {
      return true;
    }

    pending_requests = pending_requests_;
    cancelled_requests = cancelled_requests_;
  }

  bool ret = true;

  for (auto& request : pending_requests) {
    if (request.second && !request.second->CancelAndWait()) {
      OLP_SDK_LOG_WARNING_F(
          kLogTag, "CancelAllAndWait, timeout on pending, request_id=%" PRIu64,
          request.second->GetRequestId());
      ret = false;
    }
  }

  for (auto& request : cancelled_requests) {
    if (request.second && !request.second->CancelAndWait()) {
      OLP_SDK_LOG_WARNING_F(
          kLogTag,
          "CancelAllAndWait, timeout on cancelled, request_id=%" PRIu64,
          request.second->GetRequestId());
      ret = false;
    }
  }

  return ret;
}

PendingUrlRequests::PendingUrlRequestPtr PendingUrlRequests::operator[](
    const std::string& url) {
  PendingUrlRequestPtr result_ptr = nullptr;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = pending_requests_.find(url);
    if (it != pending_requests_.end()) {
      result_ptr = it->second;
    } else {
      result_ptr = std::make_shared<PendingUrlRequest>();
      pending_requests_.emplace(url, result_ptr);
    }
  }

  return result_ptr;
}

void PendingUrlRequests::OnRequestCompleted(http::RequestId request_id,
                                            const std::string& url,
                                            HttpResponse response) {
  PendingUrlRequestPtr request_ptr;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto pending_it = pending_requests_.find(url);
    auto cancelled_it = cancelled_requests_.find(url);

    // Either the request is in the pending or in the cancelled requests list.
    // Check that it is the same request. It might happen that we have a
    // cancelled request and then a followup valid request with the same URL.
    if (pending_it != pending_requests_.end() &&
        request_id == pending_it->second->GetRequestId()) {
      request_ptr = std::move(pending_it->second);
      pending_requests_.erase(pending_it);
    } else if (cancelled_it != cancelled_requests_.end() &&
               request_id == cancelled_it->second->GetRequestId()) {
      request_ptr = std::move(cancelled_it->second);
      cancelled_requests_.erase(cancelled_it);
    }
  }

  if (!request_ptr) {
    // Result but no request
    return;
  }

  request_ptr->OnRequestCompleted(std::move(response));
}

}  // namespace client
}  // namespace olp
