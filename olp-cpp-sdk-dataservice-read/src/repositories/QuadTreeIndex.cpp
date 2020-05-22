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

#include "QuadTreeIndex.h"

#include <algorithm>
#include <bitset>
#include <iostream>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

namespace olp {
namespace dataservice {
namespace read {

constexpr auto kParentQuadsKey = "parentQuads";
constexpr auto kSubQuadsKey = "subQuads";
constexpr auto kDataHandleKey = "dataHandle";
constexpr auto kVersionKey = "version";
constexpr auto kSubQuadKeyKey = "subQuadKey";
constexpr auto kPartitionKey = "partition";

QuadTreeIndex::QuadTreeIndex() {}

QuadTreeIndex::QuadTreeIndex(BlobDataPtr data) {
  if (data == nullptr || data->empty()) {
    return;
  }
  data_ = reinterpret_cast<DataHeader*>(data->data());
  size_ = data->size();
}

QuadTreeIndex::QuadTreeIndex(const olp::geo::TileKey& root, int depth,
                             std::stringstream& json_stream) {
  std::vector<IndexData> subs;
  std::vector<IndexData> parents;

  rapidjson::Document doc;
  rapidjson::IStreamWrapper stream(json_stream);
  doc.ParseStream(stream);

  auto parentQuadsValue = doc.FindMember(kParentQuadsKey);
  auto subQuadsValue = doc.FindMember(kSubQuadsKey);

  if (parentQuadsValue == doc.MemberEnd() && subQuadsValue == doc.MemberEnd()) {
    return;
  }

  if (parentQuadsValue->value.IsArray()) {
    for (auto& value : parentQuadsValue->value.GetArray()) {
      auto obj = value.GetObject();

      if (!obj.HasMember(kDataHandleKey) || !obj[kDataHandleKey].IsString() ||
          !obj.HasMember(kPartitionKey) || !obj[kPartitionKey].IsString()) {
        continue;
      }

      IndexData data;
      data.data_handle = obj[kDataHandleKey].GetString();
      data.tileKey = root.FromHereTile(obj[kPartitionKey].GetString());

      if (obj.HasMember(kVersionKey) && obj[kVersionKey].IsUint64()) {
        data.version = obj[kVersionKey].GetUint64();
      } else {
        data.version = 0;
      }

      parents.push_back(data);
    }
  }

  if (subQuadsValue->value.IsArray()) {
    for (auto& value : subQuadsValue->value.GetArray()) {
      auto obj = value.GetObject();

      if (!obj.HasMember(kDataHandleKey) || !obj[kDataHandleKey].IsString() ||
          !obj.HasMember(kSubQuadKeyKey) || !obj[kSubQuadKeyKey].IsString()) {
        continue;
      }

      IndexData data;
      data.data_handle = obj[kDataHandleKey].GetString();
      data.tileKey = root.AddedSubHereTile(obj[kSubQuadKeyKey].GetString());

      if (obj.HasMember(kVersionKey) && obj[kVersionKey].IsUint64()) {
        data.version = obj[kVersionKey].GetUint64();
      } else {
        data.version = 0;
      }
      subs.push_back(data);
    }
  }

  CreateBlob(root, depth, std::move(parents), std::move(subs));
}

void QuadTreeIndex::CreateBlob(olp::geo::TileKey root, int depth,
                               std::vector<IndexData> parents,
                               std::vector<IndexData> subs) {
  // quads must be sorted by their sub quad key, not by Quad::operator<
  std::sort(subs.begin(), subs.end(),
            [](const IndexData& lhs, const IndexData& rhs) {
              return lhs.tileKey.ToQuadKey64() < rhs.tileKey.ToQuadKey64();
            });

  std::sort(parents.begin(), parents.end(),
            [](const IndexData& lhs, const IndexData& rhs) {
              return lhs.tileKey.ToQuadKey64() < rhs.tileKey.ToQuadKey64();
            });

  // count data size(for now it is header version and data handle)
  size_t additional_data_size = 0;
  for (const IndexData& data : subs) {
    additional_data_size += data.data_handle.size() +
                            sizeof(AdditionalDataCompacted::data_header) +
                            sizeof(AdditionalDataCompacted::version);
  }
  for (const IndexData& data : parents) {
    additional_data_size += data.data_handle.size() +
                            sizeof(AdditionalDataCompacted::data_header) +
                            sizeof(AdditionalDataCompacted::version);
  }

  // calculate and allocate size
  size_ = sizeof(DataHeader) - sizeof(SubEntry) +
          (subs.size() * sizeof(SubEntry)) +
          (parents.size() * sizeof(ParentEntry)) + additional_data_size;

  raw_data_ = std::make_shared<std::vector<unsigned char>>(
      std::vector<unsigned char>(size_));
  data_ = reinterpret_cast<DataHeader*>(&(raw_data_->front()));

  data_->root_tilekey = root.ToQuadKey64();
  data_->depth = uint8_t(depth);
  data_->subkey_count = uint16_t(subs.size());
  data_->parent_count = uint8_t(parents.size());

  // write SubEntry tiles
  SubEntry* entry_ptr = data_->entries;
  char* data_ptr = const_cast<char*>(DataBegin());
  std::uint16_t data_offset = 0u;

  auto rootQuadLevel = root.Level();

  uint8_t header = BitSetFlags::kVersion | BitSetFlags::kDataHandle;

  for (const IndexData& data : subs) {
    *entry_ptr++ = {
        std::uint16_t(olp::geo::QuadKey64Helper{data.tileKey.ToQuadKey64()}
                          .GetSubkey(data.tileKey.Level() - rootQuadLevel)
                          .key),
        data_offset};
    // write additional data

    AdditionalDataCompacted* additional_data =
        reinterpret_cast<AdditionalDataCompacted*>(data_ptr + data_offset);
    additional_data->data_header = header;    // write header
    additional_data->version = data.version;  // write version
    memcpy(additional_data->data_handle, data.data_handle.data(),
           data.data_handle.size());  // data_handle
    data_offset += uint16_t(sizeof(*additional_data) -
                            sizeof(additional_data->data_handle) +
                            data.data_handle.size());
  }

  ParentEntry* parentPtr = reinterpret_cast<ParentEntry*>(entry_ptr);
  for (const IndexData& data : parents) {
    *parentPtr++ = {data.tileKey.ToQuadKey64(), data_offset};

    // write additional data
    AdditionalDataCompacted* additional_data =
        reinterpret_cast<AdditionalDataCompacted*>(data_ptr + data_offset);
    additional_data->data_header = header;    // write header
    additional_data->version = data.version;  // write version
    memcpy(additional_data->data_handle, data.data_handle.data(),
           data.data_handle.size());  // data_handle
    data_offset += uint16_t(sizeof(*additional_data) -
                            sizeof(additional_data->data_handle) +
                            data.data_handle.size());
  }
}

QuadTreeIndex::IndexData QuadTreeIndex::Find(
    const olp::geo::TileKey& tileKey) const {
  if (IsNull())
    return {};

  const olp::geo::TileKey& rootTileKey =
      olp::geo::TileKey::FromQuadKey64(data_->root_tilekey);

  if (tileKey.Level() >= rootTileKey.Level()) {
    std::uint16_t sub = std::uint16_t(
        tileKey.GetSubkey64(tileKey.Level() - rootTileKey.Level()));

    const SubEntry* end = SubEntryEnd();
    const SubEntry* entry =
        std::lower_bound(SubEntryBegin(), end, SubEntry{sub, 0});
    if (entry == end || entry->sub_quadkey != sub)
      return IndexData{tileKey, std::string(), 0};
    QuadTreeIndex::AdditionalData tile_data = TileData(entry);
    return IndexData{tileKey, std::move(tile_data.data_handle),
                     tile_data.version};
  } else {
    std::uint64_t key = tileKey.ToQuadKey64();

    const ParentEntry* end = ParentEntryEnd();
    const ParentEntry* entry =
        std::lower_bound(ParentEntryBegin(), end, ParentEntry{key, 0});
    if (entry == end || entry->key != key)
      return IndexData{tileKey, std::string(), 0};
    QuadTreeIndex::AdditionalData tile_data = TileData(entry);
    return IndexData{tileKey, std::move(tile_data.data_handle),
                     tile_data.version};
  }
}

QuadTreeIndex::AdditionalData QuadTreeIndex::TileData(
    const SubEntry* entry) const {
  const SubEntry* end = SubEntryEnd();
  const char* tagBegin = DataBegin() + entry->tag_offset;
  const char* tagEnd =
      entry + 1 == end
          ? (data_->parent_count > 0
                 ? tagBegin + ((reinterpret_cast<const ParentEntry*>(entry + 1))
                                   ->tag_offset -
                               entry->tag_offset)
                 : DataEnd())
          : tagBegin + ((entry + 1)->tag_offset - entry->tag_offset);
  return TileData(tagBegin, tagEnd);
}

QuadTreeIndex::AdditionalData QuadTreeIndex::TileData(
    const ParentEntry* entry) const {
  const ParentEntry* end = ParentEntryEnd();
  const char* tag_begin = DataBegin() + entry->tag_offset;
  const char* tag_end =
      entry + 1 == end
          ? DataEnd()
          : tag_begin + ((entry + 1)->tag_offset - entry->tag_offset);
  return TileData(tag_begin, tag_end);
}

QuadTreeIndex::AdditionalData QuadTreeIndex::TileData(
    const char* tag_begin, const char* tag_end) const {
  const AdditionalDataCompacted* additional_data =
      reinterpret_cast<const AdditionalDataCompacted*>(tag_begin);
  // here we could check flags if in future will be multiple versions of
  // packaging additional data
  auto handle =
      std::string((const char*)(additional_data->data_handle),
                  tag_end - (const char*)(additional_data->data_handle));
  return {additional_data->version, std::move(handle)};
}

QuadTreeIndex::~QuadTreeIndex() {}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
