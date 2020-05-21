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
#include <sstream>
#include <utility>
#include <vector>

#include <olp/core/geo/tiling/TileKey.h>

namespace olp {
namespace dataservice {
namespace read {

using BlobData = std::vector<unsigned char>;
using BlobDataPtr = std::shared_ptr<BlobData>;

class QuadTreeIndex {
 public:
  struct IndexData {
    /// tile key in the layer tree
    olp::geo::TileKey tileKey_;
    /// tile path can be used in MapEngine::getData
    std::string data_handle_;
    /// catalog version this tile was last changed at
    uint64_t version_ = 0;
    bool operator<(const IndexData& other) const {
      return tileKey_ < other.tileKey_;
    }
  };

  QuadTreeIndex();
  explicit QuadTreeIndex(BlobDataPtr data);
  QuadTreeIndex(const olp::geo::TileKey& root, int depth,
                std::stringstream& json);

  QuadTreeIndex(const QuadTreeIndex& other) = delete;
  QuadTreeIndex& operator=(const QuadTreeIndex& other) = delete;
  ~QuadTreeIndex();

  inline bool IsNull() const { return data_ == nullptr; }

  IndexData Find(const olp::geo::TileKey& tileKey) const;

 private:
  struct SubEntry {
    std::uint16_t sub_quadkey_;
    std::uint16_t tag_offset_;
    bool operator<(const SubEntry& other) const {
      return sub_quadkey_ < other.sub_quadkey_;
    }
  };

  // not aligned tagOffset could be 64
  struct ParentEntry {
    std::uint64_t key_;
    std::uint32_t tag_offset_;
    bool operator<(const ParentEntry& other) const { return key_ < other.key_; }
  };

  struct DataHeader {
    std::uint64_t root_tilekey_;
    std::uint8_t depth_;
    std::uint8_t parent_count_;
    std::uint16_t subkey_count_;
    SubEntry entries_[1];
  };

  // data storage flags
  enum BitSetFlags {
    kVersion = 0x1,
    kCrc = 0x2,
    // values 2-6 reserved
    kDataHandle = 0x8
  };

  // todo
#pragma pack(push) /* push current alignment to stack */
#pragma pack(1)    /* set alignment to 1 byte boundary */

  struct AdditionalDataCompacted {
    uint8_t data_header_;
    uint64_t version_;
    char data_handle_[1];
  };

#pragma pack(pop) /* restore original alignment from stack */

  struct AdditionalData {
    //  uint8_t data_header;
    uint64_t version_;
    std::string data_handle_;
  };

  void CreateBlob(olp::geo::TileKey root, int depth,
                  std::vector<IndexData> parents, std::vector<IndexData> subs);

  const SubEntry* SubEntryBegin() const { return data_->entries_; }
  const SubEntry* SubEntryEnd() const {
    return SubEntryBegin() + data_->subkey_count_;
  }

  const ParentEntry* ParentEntryBegin() const {
    return reinterpret_cast<const ParentEntry*>(SubEntryEnd());
  }
  const ParentEntry* ParentEntryEnd() const {
    return ParentEntryBegin() + data_->parent_count_;
  }

  const char* DataBegin() const {
    return reinterpret_cast<const char*>(ParentEntryEnd());
  }
  const char* DataEnd() const {
    return reinterpret_cast<const char*>(data_) + size_;
  }

  AdditionalData TileData(const SubEntry* entry) const;
  AdditionalData TileData(const ParentEntry* entry) const;
  AdditionalData TileData(const char* tag_begin, const char* tag_end) const;

  DataHeader* data_ = nullptr;
  BlobDataPtr raw_data_ = BlobDataPtr();
  size_t size_ = 0;
};

}  // namespace read
}  // namespace dataservice
}  // namespace olp
