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

#include "QuadTreeIndex.h"

#include <algorithm>
#include <iostream>

#include <olp/core/logging/Log.h>
#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include "BlobDataReader.h"
#include "BlobDataWriter.h"

namespace olp {
namespace dataservice {
namespace read {
namespace {
constexpr auto kParentQuadsKey = "parentQuads";
constexpr auto kSubQuadsKey = "subQuads";
constexpr auto kDataHandleKey = "dataHandle";
constexpr auto kVersionKey = "version";
constexpr auto kSubQuadKeyKey = "subQuadKey";
constexpr auto kPartitionKey = "partition";
constexpr auto kAdditionalMetadataKey = "additionalMetadata";
constexpr auto kChecksumKey = "checksum";
constexpr auto kDataSizeKey = "dataSize";
constexpr auto kCompressedDataSizeKey = "compressedDataSize";
constexpr auto kCrcKey = "crc";
constexpr auto kLogTag = "QuadTreeIndex";

QuadTreeIndex::IndexData ParseCommonIndexData(rapidjson::Value& value) {
  QuadTreeIndex::IndexData data;
  const auto obj = value.GetObject();
  if (obj.HasMember(kAdditionalMetadataKey) &&
      obj[kAdditionalMetadataKey].IsString()) {
    data.additional_metadata = obj[kAdditionalMetadataKey].GetString();
  }

  if (obj.HasMember(kChecksumKey) && obj[kChecksumKey].IsString()) {
    data.checksum = obj[kChecksumKey].GetString();
  }

  if (obj.HasMember(kCrcKey) && obj[kCrcKey].IsString()) {
    data.crc = obj[kCrcKey].GetString();
  }

  if (obj.HasMember(kDataSizeKey) && obj[kDataSizeKey].IsInt64()) {
    data.data_size = obj[kDataSizeKey].GetInt64();
  }

  if (obj.HasMember(kCompressedDataSizeKey) &&
      obj[kCompressedDataSizeKey].IsInt64()) {
    data.compressed_data_size = obj[kCompressedDataSizeKey].GetInt64();
  }

  if (obj.HasMember(kVersionKey) && obj[kVersionKey].IsUint64()) {
    data.version = obj[kVersionKey].GetUint64();
  }
  return data;
}

bool WriteIndexData(const QuadTreeIndex::IndexData& data,
                    BlobDataWriter& writer) {
  bool success = writer.Write(data.version);
  success &= writer.Write(data.data_size);
  success &= writer.Write(data.compressed_data_size);
  success &= writer.Write(data.data_handle);
  success &= writer.Write(data.checksum);
  success &= writer.Write(data.additional_metadata);
  success &= writer.Write(data.crc);
  return success;
}
}  // namespace

QuadTreeIndex::QuadTreeIndex(const cache::KeyValueCache::ValueTypePtr& data) {
  if (data == nullptr || data->empty()) {
    return;
  }
  data_ = reinterpret_cast<DataHeader*>(data->data());
  raw_data_ = data;
  size_ = data->size();
}

QuadTreeIndex::QuadTreeIndex(const geo::TileKey& root, int depth,
                             std::stringstream& json_stream) {
  thread_local rapidjson::CrtAllocator crt_allocator;
  thread_local rapidjson::MemoryPoolAllocator<> pool_allocator;
  rapidjson::Document doc(&pool_allocator, 4096, &crt_allocator);

  rapidjson::IStreamWrapper stream(json_stream);
  doc.ParseStream(stream);

  if (!doc.IsObject()) {
    return;
  }

  std::vector<IndexData> subs;
  std::vector<IndexData> parents;

  auto parent_quads_value = doc.FindMember(kParentQuadsKey);
  auto sub_quads_value = doc.FindMember(kSubQuadsKey);

  if (parent_quads_value == doc.MemberEnd() &&
      sub_quads_value == doc.MemberEnd()) {
    return;
  }

  if (parent_quads_value != doc.MemberEnd() &&
      parent_quads_value->value.IsArray()) {
    parents.reserve(parent_quads_value->value.Size());
    for (auto& value : parent_quads_value->value.GetArray()) {
      auto obj = value.GetObject();

      if (!obj.HasMember(kDataHandleKey) || !obj[kDataHandleKey].IsString() ||
          !obj.HasMember(kPartitionKey) || !obj[kPartitionKey].IsString()) {
        continue;
      }

      IndexData data = ParseCommonIndexData(value);
      data.data_handle = obj[kDataHandleKey].GetString();
      data.tile_key =
          geo::TileKey::FromHereTile(obj[kPartitionKey].GetString());
      parents.push_back(std::move(data));
    }
  }

  if (sub_quads_value != doc.MemberEnd() && sub_quads_value->value.IsArray()) {
    subs.reserve(sub_quads_value->value.Size());
    for (auto& value : sub_quads_value->value.GetArray()) {
      auto obj = value.GetObject();

      if (!obj.HasMember(kDataHandleKey) || !obj[kDataHandleKey].IsString() ||
          !obj.HasMember(kSubQuadKeyKey) || !obj[kSubQuadKeyKey].IsString()) {
        continue;
      }

      IndexData data = ParseCommonIndexData(value);
      data.data_handle = obj[kDataHandleKey].GetString();
      data.tile_key = root.AddedSubHereTile(obj[kSubQuadKeyKey].GetString());
      subs.push_back(std::move(data));
    }
  }

  CreateBlob(root, depth, std::move(parents), std::move(subs));
}

bool QuadTreeIndex::ReadIndexData(IndexData& data, const uint32_t offset,
                                  const uint32_t limit, int fields) const {
  BlobDataReader reader(*raw_data_);
  reader.SetOffset(offset);

  auto field_required = [fields](const Field field) {
    return (fields & field) > 0;
  };

  bool success = true;
  success &= reader.Read(data.version);
  success &= reader.Read(data.data_size);
  success &= reader.Read(data.compressed_data_size);
  success &= field_required(DataHandle) ? reader.Read(data.data_handle)
                                        : reader.Skip<std::string>();
  success &= field_required(Checksum) ? reader.Read(data.checksum)
                                      : reader.Skip<std::string>();
  success &= field_required(AdditionalMetadata)
                 ? reader.Read(data.additional_metadata)
                 : reader.Skip<std::string>();
  // The CRC field was added after the initial QuadTreeIndex implementation, and
  // to maintain the backwards compatibility, we must check that we do not read
  // the crc from the next index block.
  if (reader.GetOffset() < limit) {
    success &= field_required(Crc) ? reader.Read(data.crc)
                                   : reader.Skip<std::string>();
  }
  return success;
}

void QuadTreeIndex::CreateBlob(geo::TileKey root, int depth,
                               std::vector<IndexData> parents,
                               std::vector<IndexData> subs) {
  // quads must be sorted by their sub quad key, not by Quad::operator<
  std::sort(subs.begin(), subs.end(),
            [](const IndexData& lhs, const IndexData& rhs) {
              return lhs.tile_key.ToQuadKey64() < rhs.tile_key.ToQuadKey64();
            });

  std::sort(parents.begin(), parents.end(),
            [](const IndexData& lhs, const IndexData& rhs) {
              return lhs.tile_key.ToQuadKey64() < rhs.tile_key.ToQuadKey64();
            });

  // count data size (for now it is a header version and data handle)
  size_t additional_data_size = 0;
  for (const IndexData& data : subs) {
    additional_data_size +=
        data.data_handle.size() + 1 + data.checksum.size() + 1 +
        data.crc.size() + 1 + data.additional_metadata.size() + 1 +
        sizeof(IndexData::data_size) + sizeof(IndexData::compressed_data_size) +
        sizeof(IndexData::version);
  }
  for (const IndexData& data : parents) {
    additional_data_size +=
        data.data_handle.size() + 1 + data.checksum.size() + 1 +
        data.crc.size() + 1 + data.additional_metadata.size() + 1 +
        sizeof(IndexData::data_size) + sizeof(IndexData::compressed_data_size) +
        sizeof(IndexData::version);
  }

  // calculate and allocate size
  size_ = sizeof(DataHeader) - sizeof(SubEntry) +
          (subs.size() * sizeof(SubEntry)) +
          (parents.size() * sizeof(ParentEntry)) + additional_data_size;

  raw_data_ = std::make_shared<cache::KeyValueCache::ValueType>(size_);
  data_ = reinterpret_cast<DataHeader*>(&(raw_data_->front()));

  data_->root_tilekey = root.ToQuadKey64();
  data_->blob_version = 0;
  data_->depth = static_cast<int8_t>(depth);
  data_->subkey_count = static_cast<uint16_t>(subs.size());
  data_->parent_count = static_cast<uint8_t>(parents.size());

  // write SubEntry tiles
  SubEntry* entry_ptr = data_->entries;

  auto root_quad_level = root.Level();

  BlobDataWriter serializer(*raw_data_);
  serializer.SetOffset(const_cast<unsigned char*>(DataBegin()) -
                       raw_data_->data());
  for (const IndexData& data : subs) {
    *entry_ptr++ = {static_cast<std::uint16_t>(
                        geo::QuadKey64Helper{data.tile_key.ToQuadKey64()}
                            .GetSubkey(static_cast<int>(data.tile_key.Level() -
                                                        root_quad_level))
                            .key),
                    static_cast<uint32_t>(serializer.GetOffset())};
    if (!WriteIndexData(data, serializer)) {
      OLP_SDK_LOG_ERROR(kLogTag, "Could not write IndexData");
      raw_data_ = nullptr;
      data_ = nullptr;
      size_ = 0;
      return;
    }
  }

  auto* parent_ptr = reinterpret_cast<ParentEntry*>(entry_ptr);
  for (const IndexData& data : parents) {
    *parent_ptr++ = {data.tile_key.ToQuadKey64(),
                     static_cast<uint32_t>(serializer.GetOffset())};
    if (!WriteIndexData(data, serializer)) {
      OLP_SDK_LOG_ERROR(kLogTag, "Could not write IndexData");
      raw_data_ = nullptr;
      data_ = nullptr;
      size_ = 0;
      return;
    }
  }
}

porting::optional<QuadTreeIndex::IndexData> QuadTreeIndex::Find(
    const geo::TileKey& tile_key, bool aggregated_search) const {
  if (IsNull()) {
    return olp::porting::none;
  }
  const geo::TileKey& root_tile_key =
      geo::TileKey::FromQuadKey64(data_->root_tilekey);

  IndexData data;
  if (tile_key.Level() >= root_tile_key.Level()) {
    auto sub = static_cast<std::uint16_t>(tile_key.GetSubkey64(
        static_cast<int>(tile_key.Level() - root_tile_key.Level())));

    const SubEntry* end = SubEntryEnd();
    const SubEntry* entry =
        std::lower_bound(SubEntryBegin(), end, SubEntry{sub, 0});
    if (entry == end || entry->sub_quadkey != sub) {
      return aggregated_search ? FindNearestParent(tile_key)
                               : olp::porting::none;
    }
    const auto offset = entry->tag_offset;

    // The limit is the offset for the next entry or the beginning of the
    // parent's entries. (in case there are no parents use the size of the index
    // block.)
    const auto limit = [&]() {
      const auto next = entry + 1;
      if (next == end) {
        if (data_->parent_count == 0) {
          return static_cast<uint32_t>(raw_data_->size());
        }
        return ParentEntryBegin()->tag_offset;
      }
      return next->tag_offset;
    }();

    if (!ReadIndexData(data, offset, limit, All)) {
      return olp::porting::none;
    }
    data.tile_key = tile_key;
    return data;
  }

  const std::uint64_t key = tile_key.ToQuadKey64();

  const ParentEntry* end = ParentEntryEnd();
  const ParentEntry* entry =
      std::lower_bound(ParentEntryBegin(), end, ParentEntry{key, 0});
  if (entry == end || entry->key != key) {
    return aggregated_search ? FindNearestParent(tile_key) : olp::porting::none;
  }
  const auto offset = entry->tag_offset;

  // The limit is the offset for the next entry, or the end of the index data.
  const auto limit = [&]() {
    const auto next = entry + 1;
    return (next == end) ? raw_data_->size() : next->tag_offset;
  }();

  if (!ReadIndexData(data, offset, limit, All)) {
    return olp::porting::none;
  }
  data.tile_key = tile_key;
  return data;
}
porting::optional<QuadTreeIndex::IndexData> QuadTreeIndex::FindNearestParent(
    geo::TileKey tile_key) const {
  const geo::TileKey& root_tile_key =
      geo::TileKey::FromQuadKey64(data_->root_tilekey);

  if (tile_key.Level() >= root_tile_key.Level()) {
    auto parents_begin = ParentEntryBegin();
    uint32_t limit = parents_begin == ParentEntryEnd()
                         ? static_cast<uint32_t>(raw_data_->size())
                         : parents_begin->tag_offset;

    for (auto it = SubEntryEnd(); it-- != SubEntryBegin();) {
      auto key = root_tile_key.AddedSubkey64(it->sub_quadkey);
      if (tile_key.IsChildOf(key)) {
        IndexData data;
        data.tile_key = key;
        if (!ReadIndexData(data, it->tag_offset, limit, All)) {
          return olp::porting::none;
        }
        return data;
      }
      limit = it->tag_offset;
    }
  }

  auto limit = static_cast<uint32_t>(raw_data_->size());

  for (auto it = ParentEntryEnd(); it-- != ParentEntryBegin();) {
    auto key = geo::TileKey::FromQuadKey64(it->key);
    if (tile_key.IsChildOf(key)) {
      IndexData data;
      data.tile_key = key;
      if (!ReadIndexData(data, it->tag_offset, limit, All)) {
        return olp::porting::none;
      }
      return data;
    }
    limit = it->tag_offset;
  }
  return olp::porting::none;
}

std::vector<QuadTreeIndex::IndexData> QuadTreeIndex::GetIndexData(
    int fields) const {
  std::vector<IndexData> result;
  if (IsNull()) {
    return result;
  }
  result.reserve(data_->parent_count + data_->subkey_count);

  auto limit = static_cast<uint32_t>(raw_data_->size());

  for (auto it = ParentEntryEnd(); it-- != ParentEntryBegin();) {
    IndexData data;
    data.tile_key = geo::TileKey::FromQuadKey64(it->key);
    if (ReadIndexData(data, it->tag_offset, limit, fields)) {
      result.emplace_back(std::move(data));
    }
    limit = it->tag_offset;
  }

  for (auto it = SubEntryEnd(); it-- != SubEntryBegin();) {
    IndexData data;
    const geo::TileKey& root_tile_key =
        geo::TileKey::FromQuadKey64(data_->root_tilekey);
    data.tile_key = root_tile_key.AddedSubkey64(it->sub_quadkey);
    if (ReadIndexData(data, it->tag_offset, limit, fields)) {
      result.emplace_back(std::move(data));
    }
    limit = it->tag_offset;
  }
  return result;
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
