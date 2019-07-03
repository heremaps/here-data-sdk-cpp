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

#include <olp/dataservice/write/FlushEventListener.h>

#include <olp/dataservice/write/StreamLayerClient.h>

namespace olp {
namespace dataservice {
namespace write {

template <>
void DefaultFlushEventListener<const StreamLayerClient::FlushResponse&>::
    NotifyFlushEventResults(const StreamLayerClient::FlushResponse& results) {
  ++num_flush_events_;
  if (results.empty() || CollateFlushEventResults(results)) {
    ++num_flush_events_failed_;
  }
}

}  // namespace write
}  // namespace dataservice
}  // namespace olp
