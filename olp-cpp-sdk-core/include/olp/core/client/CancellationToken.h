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

#include <functional>

#include <olp/core/CoreApi.h>

namespace olp {
namespace client {
/**
 * @brief Control object for cancelling service requests
 */
class CORE_API CancellationToken {
 public:
  /**
   * @brief Default constructor
   */
  CancellationToken() = default;

  /**
   * @brief Constructor
   * @param func an operation that should be used to cancel an authentication
   * request.
   */
  CancellationToken(const std::function<void()> func);

  /**
   * @brief Copy Constructor
   * @param other object to copy from
   */
  CancellationToken(const CancellationToken& other);

  /**
   * @brief Cancels the current operation, calls the func_ instance variable.
   */
  void cancel() const;

 private:
  std::function<void()> func_{};
};
}  // namespace client
}  // namespace olp
