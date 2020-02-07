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

#include <chrono>
#include <random>

#include <olp/core/CoreApi.h>

namespace olp {
namespace client {

/**
 * @brief Computes wait time for the next retry attempt via
 * the exponential backoff with the added jitter.
 *
 * This backoff strategy is based on the exponential wait-time approach.
 * For example, when the wait time exponentially grows with each retry attempt,
 * but randomization is added.
 *
 * The actual formula can be described in the following way:
 * @code{.unparsed}
 * wait_time = random_between(0, initial_backdown_period_msec * (2 ^
 * retry_count))
 * @endcode
 */
struct CORE_API ExponentialBackdownStrategy {
 public:
  /**
   * @brief Computes the next retry attempt wait time based on the number of
   * retries and initial backdown period.
   *
   * @param initial_backdown_period The initial backdown period.
   * @param retry_count The number of retries that are already made.
   *
   * @return The timeout for the next retry attempt.
   */
  std::chrono::milliseconds operator()(
      std::chrono::milliseconds initial_backdown_period, size_t retry_count) {
    const auto exponential_wait_time =
        initial_backdown_period.count() * (1 << retry_count);

    static thread_local std::mt19937 kGenerator(std::random_device{}());
    std::uniform_int_distribution<std::chrono::milliseconds::rep> dist(
        0, exponential_wait_time);
    return std::chrono::milliseconds(dist(kGenerator));
  }
};

}  // namespace client
}  // namespace olp
