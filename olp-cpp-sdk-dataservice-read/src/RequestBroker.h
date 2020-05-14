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

#pragma once

#include <unordered_map>

#include <olp/core/client/CancellationContext.h>
#include <olp/dataservice/read/Types.h>

namespace olp {
namespace dataservice {
namespace read {

class RequestBroker {
 public:
  using Callback = DataResponseCallback;
  using RequestId = std::string;

  struct CreateOrAssociateResult {
    client::CancellationContext context;
    client::CancellationToken caller_cancelation_token;
    bool just_created;
  };

  CreateOrAssociateResult CreateOrAssociateRequest(RequestId req_id,
                                                   Callback callback);

  DataResponseCallback ResponseHandler(RequestId req_id);

 private:
  using CallerId = std::string;

  class RequestContext {
   public:
    void AddCallback(CallerId id, Callback callback);
    void PropagateResponse(DataResponse response);
    // Return true if the operation was canceled
    bool CancelRequest(CallerId id);

    client::CancellationContext CancelationContext();

   private:
    client::CancellationContext cancelation_context;
    std::unordered_map<CallerId, DataResponseCallback> callbacks;
  };

  using RequestMap = std::unordered_map<RequestId, RequestContext>;

  inline client::CancellationToken CancelToken(RequestId req_id, CallerId id) {
    return client::CancellationToken([=]() { CancelRequest(req_id, id); });
  }

  struct GetOrCreateResult {
    RequestContext& ctx;
    bool just_created;
  };

  GetOrCreateResult GetOrCreateContext(RequestId req_id);

  void PropagateResponse(RequestId req_id, DataResponse response);

  void CancelRequest(RequestId req_id, CallerId id);

  std::mutex mutex_;
  RequestMap request_map_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
