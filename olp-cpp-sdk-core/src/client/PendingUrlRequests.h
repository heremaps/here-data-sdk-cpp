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

#include <map>
#include <mutex>
#include <string>
#include <unordered_map>

#include "olp/core/client/CancellationContext.h"
#include "olp/core/client/Condition.h"
#include "olp/core/client/ErrorCode.h"
#include "olp/core/client/HttpResponse.h"
#include "olp/core/client/OlpClient.h"
#include "olp/core/client/OlpClientSettings.h"
#include "olp/core/http/HttpStatusCode.h"
#include "olp/core/http/NetworkConstants.h"

namespace olp {
namespace client {

/// This class represents one URL request holding one or more callbacks.
class PendingUrlRequest {
 public:
  /// Alias for the CancellationContext execute function.
  using ExecuteFuncType = std::function<CancellationToken(http::RequestId& id)>;
  /// Alias for the CancellationContext cancel function.
  using CancelFuncType = client::CancellationContext::CancelFuncType;

  /// Identifies a invalid request Id.
  static constexpr http::RequestId kInvalidRequestId =
      static_cast<http::RequestId>(http::RequestIdConstants::RequestIdInvalid);
  /// Cancelled Network request error code.
  static constexpr int kCancelledStatus =
      static_cast<int>(http::ErrorCode::CANCELLED_ERROR);

  /// Append one callback to the request.
  size_t Append(NetworkAsyncCallback callback);

  /// Calls the managed CancellationContext ExecuteOrCancelled so that it can
  /// remember the func returned CancellationToken to be able to cancel any
  /// async operation taking place in our name. Should be called with the
  /// Network trigger lambda.
  bool ExecuteOrCancelled(const ExecuteFuncType& func,
                          const CancelFuncType& cancel_func = nullptr);

  /// Get the Network request Id associated with this request.
  http::RequestId GetRequestId() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return http_request_id_;
  }

  /// Cancels the ongoing Network request. Do not block for long!
  /// NOTE: This should only be called if you know what you are doing.
  void CancelOperation() { context_.CancelOperation(); }

  /// Cancel one individual request or the entire request if no callback left
  /// to serve.
  bool Cancel(size_t callback_id);

  /// If cancelled this will return true.
  bool IsCancelled() const { return context_.IsCancelled(); }

  /// Cancels current network request and waits for response.
  bool CancelAndWait(
      std::chrono::milliseconds timeout = std::chrono::seconds(60)) {
    // Cancel Network call, wait for the Network callback
    context_.CancelOperation();
    return condition_.Wait(timeout);
  }

  /// Will be called when the Network response arrives.
  void OnRequestCompleted(HttpResponse response);

 private:
  /// Keeping the class thread safe.
  mutable std::mutex mutex_;
  /// The id of the Network request to identify the correct response to the
  /// correct request.
  http::RequestId http_request_id_{kInvalidRequestId};
  /// Notifies once this request has been completed. Will be used by
  /// CancelAndWait().
  Condition condition_;
  /// Keeps track of the Network call and cancels it in case user wants to
  /// cancel the entire request or the last call
  CancellationContext context_;
  /// The list of pending requests
  std::map<size_t, NetworkAsyncCallback> callbacks_;
  /// The list of cancelled requests
  std::vector<NetworkAsyncCallback> cancelled_callbacks_;
};

/// This class holds all URL based requests.
class PendingUrlRequests {
 public:
  /// Alias for a shareable pending request
  using PendingUrlRequestPtr = std::shared_ptr<PendingUrlRequest>;

  virtual ~PendingUrlRequests() { CancelAllAndWait(); }

  /// Get the total size of requests pending and cancelled requests.
  size_t Size() const;

  /// Cancel one callback from a request based on callback ID.
  bool Cancel(const std::string& url, size_t callback_id);

  /// Cancel all pending requests, non-blocking.
  bool CancelAll();

  /// Cancel pending requests and wait for all requests to finish, blocking.
  bool CancelAllAndWait() const;

  /// Get the existing pending request associated with the url or create a new
  /// one if not present yet.
  PendingUrlRequestPtr operator[](const std::string& url);

  /// Atomically append to an pending request or create a new one and append.
  /// This will make sure that there is no gap between operator[] and appending
  /// the callback to the request.
  size_t Append(const std::string& url, NetworkAsyncCallback callback,
                PendingUrlRequestPtr& out);

  /// Should be called inside the Network callback when request is finished.
  void OnRequestCompleted(http::RequestId request_id, const std::string& url,
                          HttpResponse response);

 private:
  /// The type of the contained used to store pending or cancelled requests.
  using PendingRequestsType =
      std::unordered_map<std::string, PendingUrlRequestPtr>;

  mutable std::mutex mutex_;
  PendingRequestsType pending_requests_;
  PendingRequestsType cancelled_requests_;
};

}  // namespace client
}  // namespace olp
