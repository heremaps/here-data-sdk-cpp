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

#include <unordered_map>
#include <mutex>
#include <vector>

#include <olp/core/client/CancellationToken.h>

namespace olp {
namespace dataservice {
namespace read {

class PendingRequests final {
 public:
  PendingRequests();
  ~PendingRequests();

  /**
   * @brief Cancels all pending requests.
   * @return True on success
   */
  bool CancelPendingRequests();

  /**
   * @brief Generates a placehoder for request cancellation token and returns a
   * key associated with it.
   * @return Returns a key associated with a placeholder.
   */
  int64_t GenerateRequestPlaceholder();

  /**
   * @brief Inserts request cancellation token into the placeholder associated
   * with a key.
   * @param token Cancellation token.
   * @param key Key associated with a placeholder.
   * @return True on success, false when the placeholder is missing.
   */
  bool Insert(const client::CancellationToken& token, int64_t key);

  /**
   * @brief Removes a pending request and placholder.
   * @param key Internal request key to remove
   * @return True on success
   */
  bool Remove(int64_t key);

 private:
  int64_t key_ = 0;
  std::unordered_map<int64_t, client::CancellationToken> requests_map_;
  std::mutex requests_lock_;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp