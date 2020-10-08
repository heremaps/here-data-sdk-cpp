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

#include <olp/dataservice/read/PartitionsRequest.h>
#include <olp/dataservice/read/model/VersionResponse.h>

class PathGenerator {
 public:
  static std::string GetPartitions(
      const std::string& layer,
      const olp::dataservice::read::PartitionsRequest::PartitionIds& partitions,
      uint64_t version);

  static std::string GetData(const std::string& layer,
                             const std::string& data_handle);

  static std::string GetLatestVersion();

  static std::string GetQuadKey(const std::string& quadkey,
                                const std::string& layer, uint64_t version,
                                uint64_t depth);

  static std::string FullPath(const std::string& apis,
                              const std::string& api_type,
                              const std::string& path);
};
