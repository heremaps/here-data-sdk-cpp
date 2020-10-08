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

#include <string>
#include <vector>

#include "generated/model/Api.h"
#include "olp/dataservice/read/model/Partitions.h"
#include "olp/dataservice/read/model/VersionResponse.h"

namespace mock {
std::string GenerateGetPartitionsPath(
    const std::string& layer,
    const olp::dataservice::read::PartitionsRequest::PartitionIds& partitions,
    uint64_t version) {
  std::string path = "/layers/" + layer + "/partitions?";
  for (const auto& partition : partitions) {
    path.append("partition=" + partition + "&");
  }
  path.append("version=" + std::to_string(version));
  return path;
}

std::string GenerateGetDataPath(const std::string& layer,
                                const std::string& data_handle) {
  return "/layers/" + layer + "/data/" + data_handle;
}

std::string GenerateGetLatestVersionPath() {
  return "/versions/latest?startVersion=-1";
}

std::string GenerateGetQuadKeyPath(const std::string& quadkey,
                                   const std::string& layer, uint64_t version,
                                   uint64_t depth) {
  return "/layers/" + layer + "/versions/" + std::to_string(version) +
         "/quadkeys/" + quadkey + "/depths/" + std::to_string(depth);
}

std::string GeneratePath(const olp::dataservice::read::model::Apis& apis,
                         const std::string& api_type, const std::string& path) {
  auto it = find_if(apis.begin(), apis.end(),
                    [&](olp::dataservice::read::model::Api api) {
                      return api.GetApi() == api_type;
                    });
  if (it == apis.end()) {
    return {};
  }
  auto url = it->GetBaseUrl();
  return url.append(path);
}

}  // namespace mock
