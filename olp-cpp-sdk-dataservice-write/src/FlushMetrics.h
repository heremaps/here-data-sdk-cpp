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

#include <cstddef>

namespace olp {
namespace dataservice {
namespace write {

/**
 * @brief Struct which gather the metrics of Flush events and requests queued
 * by \c StreamLayerClient.
 */
struct FlushMetrics {
  /**
  * @brief Number of attempted flush events.
  */
  size_t num_attempted_flush_events;

  /**
   * @brief Number of failed flush events
   */
  size_t num_failed_flush_events;

  /**
   * @brief Total number of flush events.
   */
  size_t num_total_flush_events;

  /**
   * @brief Total number of requests queued to \c StreamLayerClient.
   */
  size_t num_total_flushed_requests;

  /**
   * @brief Number of failed requests, which were queued to \c
   * StreamLayerClient.
   */
  size_t num_failed_flushed_requests;
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
