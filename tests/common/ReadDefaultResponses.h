/*
 * Copyright (C) 2020-2024 HERE Europe B.V.
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

#include <map>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <olp/core/geo/tiling/TileKey.h>

#include "olp/dataservice/read/model/Partitions.h"
#include "olp/dataservice/read/model/VersionResponse.h"

namespace mockserver {

struct TileMetadata {
  std::string data_handle;
  boost::optional<int32_t> version;
  std::string crc;
  std::string checksum;
  int32_t data_size{0};
  int32_t compressed_data_size{0};
};

class ReadDefaultResponses {
 public:
  static olp::dataservice::read::model::VersionResponse GenerateVersionResponse(
      int64_t version) {
    olp::dataservice::read::model::VersionResponse version_responce;
    version_responce.SetVersion(version);
    return version_responce;
  }

  static std::string GenerateDataHandle(const std::string& partition) {
    return partition + "-data-handle";
  }

  static olp::dataservice::read::model::Partition GeneratePartitionResponse(
      const std::string& id) {
    olp::dataservice::read::model::Partition partition;
    partition.SetPartition(id);
    partition.SetDataHandle(GenerateDataHandle(id));
    return partition;
  }

  static olp::dataservice::read::model::Partitions GeneratePartitionsResponse(
      size_t size = 10, uint32_t start_index = 0) {
    std::vector<olp::dataservice::read::model::Partition> partitions_vect;
    partitions_vect.reserve(size);
    for (size_t i = 0; i < size; i++) {
      partitions_vect.emplace_back(
          GeneratePartitionResponse(std::to_string(start_index + i)));
    }

    olp::dataservice::read::model::Partitions partitions;
    partitions.SetPartitions(partitions_vect);
    return partitions;
  }

  static std::string GenerateData(size_t length = 64u);

  static std::string GenerateQuadTreeResponse(
      olp::geo::TileKey root_tile, std::uint32_t depth,
      const std::vector<std::uint32_t>& available_levels);
};

class QuadTreeBuilder {
 public:
  // if version is set, the quad tree is considered to be a versioned type.
  explicit QuadTreeBuilder(olp::geo::TileKey root_tile,
                           boost::optional<int32_t> base_version);

  QuadTreeBuilder& WithParent(olp::geo::TileKey parent, std::string data_handle,
                              boost::optional<int32_t> version = boost::none);
  QuadTreeBuilder& FillParents();

  // tile is represented as a normal tilekey
  QuadTreeBuilder& WithSubQuad(olp::geo::TileKey tile, std::string datahandle,
                               boost::optional<int32_t> version = boost::none);

  std::string BuildJson() const;

  olp::geo::TileKey Root() const;

 protected:
  olp::geo::TileKey root_tile_;
  boost::optional<int32_t> base_version_;

  std::map<std::uint64_t, TileMetadata> sub_quads_;
  std::map<std::uint64_t, TileMetadata> parent_quads_;
};

}  // namespace mockserver
