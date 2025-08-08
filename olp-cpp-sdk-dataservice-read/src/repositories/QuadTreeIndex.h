/*
 * Copyright (C) 2020-2025 HERE Europe B.V.
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
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <olp/core/cache/KeyValueCache.h>
#include <olp/core/geo/tiling/TileKey.h>

namespace olp {
namespace dataservice {
namespace read {

class BlobDataWriter;

class QuadTreeIndex {
 public:
  struct IndexData {
    geo::TileKey tile_key;
    std::string data_handle;
    std::string additional_metadata;
    std::string crc;
    std::string checksum;
    uint64_t version = -1;
    int64_t data_size = -1;
    int64_t compressed_data_size = -1;

    bool operator<(const IndexData& other) const {
      return tile_key < other.tile_key;
    }
  };

  enum Field {
    DataHandle = 1 << 1,
    AdditionalMetadata = 1 << 2,
    Crc = 1 << 3,
    Checksum = 1 << 4,
    All = DataHandle | AdditionalMetadata | Crc | Checksum
  };

  QuadTreeIndex() = default;
  explicit QuadTreeIndex(const cache::KeyValueCache::ValueTypePtr& data);
  QuadTreeIndex(const geo::TileKey& root, int depth,
                std::stringstream& json_stream);

  QuadTreeIndex(const QuadTreeIndex& other) = delete;
  QuadTreeIndex(QuadTreeIndex&& other) noexcept = default;
  QuadTreeIndex& operator=(QuadTreeIndex&& other) noexcept = default;
  QuadTreeIndex& operator=(const QuadTreeIndex& other) = delete;
  ~QuadTreeIndex() = default;

  bool IsNull() const { return data_ == nullptr; }

  porting::optional<IndexData> Find(const geo::TileKey& tile_key,
                                    bool aggregated_search) const;

  cache::KeyValueCache::ValueTypePtr GetRawData() const { return raw_data_; }

  // Get the entire index data, including all parents and sub quads. With a
  // `fields` parameter, the caller can specify which fields should be included
  // in the result.
  std::vector<IndexData> GetIndexData(int fields = All) const;

  geo::TileKey GetRootTile() const {
    return data_ ? geo::TileKey::FromQuadKey64(data_->root_tilekey)
                 : geo::TileKey();
  }

 private:
  struct SubEntry {
    std::uint16_t sub_quadkey;
    std::uint32_t tag_offset;
    bool operator<(const SubEntry& other) const {
      return sub_quadkey < other.sub_quadkey;
    }
  };

  // not aligned tagOffset could be 64
  struct ParentEntry {
    std::uint64_t key;
    std::uint32_t tag_offset;
    bool operator<(const ParentEntry& other) const { return key < other.key; }
  };

  struct DataHeader {
    std::uint64_t root_tilekey;
    std::uint16_t blob_version;
    std::int8_t depth;
    std::uint8_t parent_count;
    std::uint16_t subkey_count;
    SubEntry entries[1];
  };

  void CreateBlob(geo::TileKey root, int depth, std::vector<IndexData> parents,
                  std::vector<IndexData> subs);

  porting::optional<IndexData> FindNearestParent(geo::TileKey tile_key) const;

  const SubEntry* SubEntryBegin() const { return data_->entries; }
  const SubEntry* SubEntryEnd() const {
    return SubEntryBegin() + data_->subkey_count;
  }

  const ParentEntry* ParentEntryBegin() const {
    return reinterpret_cast<const ParentEntry*>(SubEntryEnd());
  }
  const ParentEntry* ParentEntryEnd() const {
    return ParentEntryBegin() + data_->parent_count;
  }

  const uint8_t* DataBegin() const {
    return reinterpret_cast<const uint8_t*>(ParentEntryEnd());
  }
  const uint8_t* DataEnd() const {
    return reinterpret_cast<const uint8_t*>(data_) + size_;
  }

  bool ReadIndexData(IndexData& data, uint32_t offset, uint32_t limit,
                     int fields) const;

  DataHeader* data_ = nullptr;
  cache::KeyValueCache::ValueTypePtr raw_data_ = nullptr;
  size_t size_ = 0;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
