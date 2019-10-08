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

#include <string>

#include <boost/optional.hpp>

#include <olp/dataservice/write/DataServiceWriteApi.h>

namespace olp {
namespace dataservice {
namespace write {

struct DATASERVICE_WRITE_API FlushSettings {
  /**
   * How many requests can be cached before an auto flush event is triggered.
   */
  int auto_flush_num_events = 20;

  /**
   * The period (in seconds) between sequential auto flush events when using
   * interval based auto-flush. Setting 0 indicates this feature is disabled.
   */
  int auto_flush_interval = 0;

  /**
   * Queue time of oldest queued partition that will trigger an auto-flush event
   * in seconds. Setting 0 indicates this feature is disabled.
   */
  int auto_flush_old_events_force_flush_interval = 0;

  /**
    @brief The maximum number of partitions to be flushed each time.Set
    boost::none to flush all partitions. Non-positive number will flush nothing.
  */
  boost::optional<int> events_per_single_flush = boost::none;

  /**
    @brief The maximum number of requests that can be stored
    boost::none to store all requests. Must be positive.
  */
  boost::optional<size_t> maximum_requests = boost::none;
};

}  // namespace write
}  // namespace dataservice
}  // namespace olp
