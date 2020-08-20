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

#include <olp/core/client/ApiResponse.h>

namespace olp {
namespace client {

template <typename Result, typename Error, typename Payload>
class ExtendedApiResponse : public client::ApiResponse<Result, Error> {
 public:
  ExtendedApiResponse() = default;

  // Implicit constructor by desing, same as in ApiResponse
  ExtendedApiResponse(Result result)
      : client::ApiResponse<Result, Error>(std::move(result)), payload_{} {}

  // Implicit constructor by desing, same as in ApiResponse
  ExtendedApiResponse(const Error& error)
      : client::ApiResponse<Result, Error>(error), payload_{} {}

  ExtendedApiResponse(Result result, Payload payload)
      : client::ApiResponse<Result, Error>(std::move(result)),
        payload_(std::move(payload)) {}

  ExtendedApiResponse(const Error& error, Payload payload)
      : client::ApiResponse<Result, Error>(error),
        payload_(std::move(payload)) {}

  const Payload& GetPayload() const { return payload_; }

 private:
  Payload payload_;
};

}  // namespace client
}  // namespace olp
