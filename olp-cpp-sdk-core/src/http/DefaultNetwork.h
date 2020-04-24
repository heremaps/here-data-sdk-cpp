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

#include <atomic>
#include <mutex>
#include <unordered_map>

#include <olp/core/CoreApi.h>
#include <olp/core/http/Network.h>
#include <olp/core/thread/Atomic.h>

namespace olp {
namespace http {

/**
 * @brief The default network class.
 *
 * Designed to provide default headers functionality on top of other `Network`
 * instances.
 */
class DefaultNetwork final : public Network {
 public:
  /**
   * @brief Creates the `DefaultNetwork` instance.
   *
   * @param network The `Network` instance that is used to handle `Send` and
   * `Cancel` methods.
   */
  explicit DefaultNetwork(std::shared_ptr<Network> network);
  ~DefaultNetwork() override;

  /// Implements the `Send` method of the `Network` class.
  SendOutcome Send(NetworkRequest request, Payload payload, Callback callback,
                   HeaderCallback header_callback = nullptr,
                   DataCallback data_callback = nullptr) override;

  /// Implements the `Cancel` method of the `Network` class.
  void Cancel(RequestId id) override;

  /// Implements the `SetDefaultHeaders` method of the `Network` class.
  void SetDefaultHeaders(Headers headers) override;

  /// Implements the `SetCurrentBucket` method of the `Network` class.
  void SetCurrentBucket(uint8_t bucket_id) override;

  /// Implements the `GetStatistics` method of the `Network` class.
  Statistics GetStatistics(uint8_t bucket_id) override;

 private:
  void AppendUserAgent(Headers& request_headers) const;
  void AppendDefaultHeaders(Headers& request_headers) const;

  void LockStatistics(uint8_t bucket_id,
                      std::function<void(Statistics&)> callback);

  std::atomic<uint8_t> current_statistics_bucket_;

  using BucketsContainer = std::unordered_map<uint8_t, Statistics>;
  thread::Atomic<BucketsContainer> buckets_;

  std::mutex default_headers_mutex_;
  Headers default_headers_;
  std::string user_agent_;

  std::shared_ptr<Network> network_;
};

}  // namespace http
}  // namespace olp
