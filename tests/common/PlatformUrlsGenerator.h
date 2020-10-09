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

#include <memory>
#include <string>
#include <vector>

#include <olp/core/client/model/Api.h>
#include <olp/dataservice/read/PartitionsRequest.h>
#include <olp/dataservice/read/model/VersionResponse.h>

class PlatformUrlsGenerator {
 public:
  PlatformUrlsGenerator(olp::client::Apis apis, const std::string& layer);

  PlatformUrlsGenerator(const std::string& endpoint, const std::string& catalog,
                        const std::string& layer);

  std::string PartitionsQuery(
      const olp::dataservice::read::PartitionsRequest::PartitionIds& partitions,
      uint64_t version);

  std::string DataBlob(const std::string& data_handle);

  std::string LatestVersion();

  std::string VersionedQuadTree(const std::string& quadkey, uint64_t version,
                                uint64_t depth);

 private:
  std::string FullPath(const std::string& service, const std::string& path);

  std::shared_ptr<olp::client::Apis> apis_;
  const std::string http_prefix_;
  const std::string catalog_;
  const std::string layer_;
};
