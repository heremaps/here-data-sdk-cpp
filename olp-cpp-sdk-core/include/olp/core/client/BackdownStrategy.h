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

#include <olp/core/CoreApi.h>
#include <random>

namespace olp {
namespace client {

/**
 * @brief Functor which computes wait time for the next retry attempt via
 * exponential backoff with added jitter.
 *
 * This backoff strategy is based on the exponential wait time approach, i.e.
 * when wait time exponentially grows with each retry attempt, but with added
 * randomization.
 *
 * The actual formula can be described as:
 * @code{.unparsed}
 * wait_time = random_between(0, initial_backdown_period_msec * (2 ^
 * retry_count))
 * @endcode
 */
struct CORE_API ExponentialBackdownStrategy {
 public:
  /**
   * @brief Function to compute next retry attemp wait time based on number of
   * retries and initial backdown period.
   *
   * @param initial_backdown_period_msec Initial backdown period in
   * milliseconds.
   * @param retry_count Number of retries already made.
   *
   * @return Wait time in milliseconds for the next retry attempt.
   */
  int operator()(int initial_backdown_period_msec, size_t retry_count) {
    const auto exponential_wait_time =
        initial_backdown_period_msec * (1 << retry_count);

    static thread_local std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, exponential_wait_time);
    return dist(generator);
  }
};

}  // namespace client
}  // namespace olp
