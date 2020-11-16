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

#include <olp/core/client/DefaultLookupEndpointProvider.h>

namespace olp {
namespace client {

std::string DefaultLookupEndpointProvider::operator()(
    const std::string& partition) {
  constexpr struct {
    const char* partition;
    const char* url;
  } kDatastoreServerUrl[4] = {
      {"here", "https://api-lookup.data.api.platform.here.com/lookup/v1"},
      {"here-dev",
       "https://api-lookup.data.api.platform.in.here.com/lookup/v1"},
      {"here-cn", "https://api-lookup.data.api.platform.hereolp.cn/lookup/v1"},
      {"here-cn-dev",
       "https://api-lookup.data.api.platform.in.hereolp.cn/lookup/v1"}};

  for (const auto& it : kDatastoreServerUrl) {
    if (partition == it.partition)
      return it.url;
  }

  return std::string();
}

}  // namespace client
}  // namespace olp
