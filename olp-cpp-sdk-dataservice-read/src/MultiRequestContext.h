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

#include <iostream>
#include <map>
#include <mutex>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <olp/core/client/CancellationToken.h>

#include <olp/core/client/ErrorCode.h>

#include <olp/core/logging/Log.h>

namespace olp {
namespace dataservice {
namespace read {

namespace {
constexpr auto kMultiRequestContextLogTag = "MultiRequestContext";
}

template <typename Response>
class MultiRequestContext final {
 public:
  using Callback = std::function<void(Response)>;
  using ExecuteFn = std::function<client::CancellationToken(Callback)>;

  MultiRequestContext(const Response& cancelled)
      : cancelled_(cancelled),
        mutex_(std::make_shared<std::recursive_mutex>()),
        active_reqs_(
            std::make_shared<
                std::map<std::string, std::shared_ptr<RequestContext>>>()) {}

  ~MultiRequestContext() {
    // cancel all requests, replying on the underlying provider to invoke
    // onRequestCompleted which in turn invokes all the callbacks.
    std::lock_guard<std::recursive_mutex> lock(*mutex_);

    while (!active_reqs_->empty()) {
      auto itr = active_reqs_->begin();
      OLP_SDK_LOG_INFO_F(kMultiRequestContextLogTag,
                         "~MultiRequestContext() -> cancelling id %s",
                         itr->first.c_str());
      itr->second->cancellation_token_.cancel();
    }
  }

  inline olp::client::CancellationToken ExecuteOrAssociate(
      const std::string& key, const ExecuteFn& execute_fn,
      Callback callback_fn) {
    boost::uuids::uuid request_id = uuid_gen_();

    OLP_SDK_LOG_TRACE_F(kMultiRequestContextLogTag,
                        "ExecuteOrAssociate(%s) -> request uuid = %s",
                        key.c_str(),
                        boost::uuids::to_string(request_id).c_str());

    std::shared_ptr<RequestContext> newContext;

    {
      std::lock_guard<std::recursive_mutex> lock(*mutex_);

      auto itr = active_reqs_->find(key);
      if (itr != active_reqs_->end()) {
        OLP_SDK_LOG_INFO_F(kMultiRequestContextLogTag,
                           "ExecuteOrAssociate(%s) -> existing request key",
                           key.c_str());

        itr->second->callbacks_[request_id] = callback_fn;
      } else {
        OLP_SDK_LOG_INFO_F(kMultiRequestContextLogTag,
                           "ExecuteOrAssociate(%s) -> new request key",
                           key.c_str());

        newContext = std::make_shared<RequestContext>();
        newContext->callbacks_[request_id] = callback_fn;

        (*active_reqs_)[key] = newContext;
      }
    }

    if (newContext) {
      auto mutex = mutex_;
      auto reqs = active_reqs_;
      Callback callback = [mutex, reqs, key](Response response) {
        onRequestCompleted(mutex, reqs, response, key);
      };

      OLP_SDK_LOG_INFO_F(kMultiRequestContextLogTag,
                         "ExecuteOrAssociate(%s) -> execute request()",
                         key.c_str());

      newContext->cancellation_token_ = execute_fn(callback);
    }

    // return a CancellationToken
    auto mutex = mutex_;
    auto reqs = active_reqs_;
    auto cancelled = cancelled_;
    return olp::client::CancellationToken(
        [cancelled, mutex, reqs, request_id, key]() {
          onRequestCancelled(cancelled, mutex, reqs, key, request_id);
        });
  }

 private:
  struct RequestContext {
    olp::client::CancellationToken cancellation_token_;
    std::map<boost::uuids::uuid, Callback> callbacks_;
  };

  using RequestContextPtr = std::shared_ptr<RequestContext>;

  using ReqsPtr = std::shared_ptr<std::map<std::string, RequestContextPtr>>;

 private:
  static void onRequestCompleted(std::shared_ptr<std::recursive_mutex> mutex,
                                 ReqsPtr reqs, Response response,
                                 std::string key) {
    OLP_SDK_LOG_TRACE_F(kMultiRequestContextLogTag, "onRequestCompleted(%s)",
                        key.c_str());
    RequestContextPtr context;

    {
      std::lock_guard<std::recursive_mutex> lock(*mutex);
      auto itr = reqs->find(key);

      if (itr != reqs->end()) {
        context = itr->second;  // copy the ptr
        reqs->erase(itr);
      }
    }  // release the lock, then invoke the callbacks.

    if (context) {
      OLP_SDK_LOG_INFO_F(kMultiRequestContextLogTag,
                         "onRequestCompleted(%s) -> callback count = %lu",
                         key.c_str(), context->callbacks_.size());

      for (auto& callback : context->callbacks_) {
        callback.second(response);
      }
    }
  }

  static void onRequestCancelled(Response cancelled,
                                 std::shared_ptr<std::recursive_mutex> mutex,
                                 ReqsPtr reqs, const std::string& key,
                                 const boost::uuids::uuid& request_id) {
    OLP_SDK_LOG_TRACE_F(kMultiRequestContextLogTag, "onRequestCancelled(%s)",
                        key.c_str());
    RequestContextPtr context;

    {
      std::lock_guard<std::recursive_mutex> lock(*mutex);
      auto itr = reqs->find(key);

      if (itr != reqs->end()) {
        context = itr->second;  // copy the ptr
        reqs->erase(itr);
      }
    }  // release the lock, then invoke the callbacks.

    if (context) {
      OLP_SDK_LOG_INFO_F(kMultiRequestContextLogTag,
                         "onRequestCancelled(key=%s, id=%s)", key.c_str(),
                         boost::uuids::to_string(request_id).c_str());

      // find the callback by request_id
      auto requestItr = context->callbacks_.find(request_id);

      if (requestItr != context->callbacks_.end()) {
        // if the callback map has other callback entries

        if (context->callbacks_.size() <= 1) {
          // this is the last callback.
          // cancel the underlying request, allowing the provider to invoke the
          // callback(s)
          context->cancellation_token_.cancel();
        }

        auto req = *requestItr;
        // remove the callback and invoke it
        context->callbacks_.erase(requestItr);
        req.second(cancelled);
      }
    }
  }

 private:
  Response cancelled_;
  std::shared_ptr<std::recursive_mutex> mutex_;
  std::shared_ptr<std::map<std::string, std::shared_ptr<RequestContext>>>
      active_reqs_;
  boost::uuids::random_generator uuid_gen_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
