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

#include <olp/dataservice/read/model/Partitions.h>
#include <olp/dataservice/read/model/VersionResponse.h>

PlatformUrlsGenerator::PlatformUrlsGenerator(olp::client::Apis apis,
                                             const std::string& layer)
    : apis_(std::make_shared<olp::client::Apis>(apis)), layer_(layer) {}

PlatformUrlsGenerator::PlatformUrlsGenerator(const std::string& endpoint,
                                             const std::string& catalog,
                                             const std::string& layer)
    : apis_(nullptr),
      http_prefix_(endpoint),
      catalog_(catalog),
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

std::string PlatformUrlsGenerator::FullPath(const std::string& service,
                                            const std::string& path) {
  std::string url;
  if (!apis_) {
    url = http_prefix_ + "/catalogs/" + catalog_;
  } else {
    auto it = find_if(apis_->begin(), apis_->end(), [&](olp::client::Api api) {
      return api.GetApi() == service;
    });
    if (it == apis_->end()) {
      assert(false);
      return {};
    }
    url = it->GetBaseUrl();
  }
  return url.append(path);
}
