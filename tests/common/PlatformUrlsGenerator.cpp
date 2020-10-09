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

#include "PlatformUrlsGenerator.h"

#include <string>
#include <vector>

// clang-format off
#include "generated/parser/ApiParser.h"
#include <olp/core/generated/parser/JsonParser.h>
// clang-format on
#include <olp/dataservice/read/model/Partitions.h>
#include <olp/dataservice/read/model/VersionResponse.h>
#include "generated/model/Api.h"

PlatformUrlsGenerator::PlatformUrlsGenerator(const std::string& apis,
                                             const std::string& layer)
    : apis_(std::make_shared<olp::dataservice::read::model::Apis>(
          olp::parser::parse<olp::dataservice::read::model::Apis>(apis))),
      layer_(layer) {}

std::string PlatformUrlsGenerator::PartitionsQuery(
    const olp::dataservice::read::PartitionsRequest::PartitionIds& partitions,
    uint64_t version) {
  std::string path = "/layers/" + layer_ + "/partitions?";
  for (const auto& partition : partitions) {
    path.append("partition=" + partition + "&");
  }
  path.append("version=" + std::to_string(version));

  return FullPath("query", path);
}

std::string PlatformUrlsGenerator::DataBlob(const std::string& data_handle) {
  return FullPath("blob", "/layers/" + layer_ + "/data/" + data_handle);
}

std::string PlatformUrlsGenerator::LatestVersion() {
  return FullPath("metadata", "/versions/latest?startVersion=-1");
}

std::string PlatformUrlsGenerator::VersionedQuadTree(const std::string& quadkey,
                                                     uint64_t version,
                                                     uint64_t depth) {
  auto path = "/layers/" + layer_ + "/versions/" + std::to_string(version) +
              "/quadkeys/" + quadkey + "/depths/" + std::to_string(depth);
  return FullPath("query", path);
}

std::string PlatformUrlsGenerator::FullPath(const std::string& api_type,
                                            const std::string& path) {
  auto it = find_if(apis_->begin(), apis_->end(),
                    [&](olp::dataservice::read::model::Api api) {
                      return api.GetApi() == api_type;
                    });
  if (it == apis_->end()) {
    return {};
  }
  auto url = it->GetBaseUrl();
  return url.append(path);
}
