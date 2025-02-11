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
#include <boost/json/parse.hpp>
#include <boost/json/value.hpp>
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

olp::dataservice::read::QuadTreeIndex::IndexData ParseCommonIndexData(
    boost::json::object& obj) {
  olp::dataservice::read::QuadTreeIndex::IndexData data;
  if (obj.contains(kAdditionalMetadataKey) &&
      obj[kAdditionalMetadataKey].is_string()) {
    data.additional_metadata = obj[kAdditionalMetadataKey].get_string().c_str();
  }

  if (obj.contains(kChecksumKey) && obj[kChecksumKey].is_string()) {
    data.checksum = obj[kChecksumKey].get_string().c_str();
  }

  if (obj.contains(kCrcKey) && obj[kCrcKey].is_string()) {
    data.crc = obj[kCrcKey].get_string().c_str();
  }

  if (auto* data_size = obj.if_contains(kDataSizeKey)) {
    if (data_size->is_uint64() || data_size->is_int64()) {
      data.data_size = data_size->to_number<int64_t>();
    }
  }

  if (auto* data_size = obj.if_contains(kCompressedDataSizeKey)) {
    if (data_size->is_uint64() || data_size->is_int64()) {
      data.compressed_data_size = data_size->to_number<int64_t>();
    }
  }

  if (auto* version = obj.if_contains(kVersionKey)) {
    if (version->is_uint64() || version->is_int64()) {
      data.version = version->to_number<uint64_t>();
    }
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

QuadTreeIndex::QuadTreeIndex(const olp::geo::TileKey& root, int depth,
                             std::stringstream& json_stream) {
  boost::json::error_code ec;
  auto parsed_value = boost::json::parse(json_stream, ec);

  if (!parsed_value.is_object()) {
    return;
  }

  auto& top_object = parsed_value.as_object();

  auto parent_quads_value = top_object.find(kParentQuadsKey);
  auto sub_quads_value = top_object.find(kSubQuadsKey);

  if (parent_quads_value == top_object.end() &&
      sub_quads_value == top_object.end()) {
    return;
  }

  std::vector<IndexData> subs;
  std::vector<IndexData> parents;

  if (parent_quads_value != top_object.end() &&
      parent_quads_value->value().is_array()) {
    auto& parent_quads = parent_quads_value->value().as_array();
    parents.reserve(parent_quads.size());
    for (auto& value : parent_quads) {
      auto& obj = value.as_object();

      if (!obj.contains(kDataHandleKey) || !obj[kDataHandleKey].is_string() ||
          !obj.contains(kPartitionKey) || !obj[kPartitionKey].is_string()) {
        continue;
      }

      IndexData data = ParseCommonIndexData(obj);
      data.data_handle = obj[kDataHandleKey].as_string().c_str();
      data.tile_key = olp::geo::TileKey::FromHereTile(
          obj[kPartitionKey].as_string().c_str());
      parents.push_back(std::move(data));
    }
  }

  if (sub_quads_value != top_object.end() &&
      sub_quads_value->value().is_array()) {
    auto& sub_quads = sub_quads_value->value().as_array();
    subs.reserve(sub_quads.size());
    for (auto& value : sub_quads) {
      auto obj = value.as_object();

      if (!obj.contains(kDataHandleKey) || !obj[kDataHandleKey].is_string() ||
          !obj.contains(kSubQuadKeyKey) || !obj[kSubQuadKeyKey].is_string()) {
        continue;
      }

      IndexData data = ParseCommonIndexData(obj);
      data.data_handle = obj[kDataHandleKey].as_string().c_str();
      data.tile_key =
          root.AddedSubHereTile(obj[kSubQuadKeyKey].as_string().c_str());
      subs.push_back(std::move(data));
    }
  }

  CreateBlob(root, depth, std::move(parents), std::move(subs));
}

bool QuadTreeIndex::ReadIndexData(QuadTreeIndex::IndexData& data,
                                  uint32_t offset, uint32_t limit) const {
  BlobDataReader reader(*raw_data_);
  reader.SetOffset(offset);

  bool success = reader.Read(data.version);
  success &= reader.Read(data.data_size);
  success &= reader.Read(data.compressed_data_size);
  success &= reader.Read(data.data_handle);
  success &= reader.Read(data.checksum);
  success &= reader.Read(data.additional_metadata);
  // The CRC field was added after the initial QuadTreeIndex implementation, and
  // to maintain the backwards compatibility we must check that we do not read
  // the crc from the next index block.
  if (reader.GetOffset() < limit) {
    success &= reader.Read(data.crc);
  }
  return success;
}

void QuadTreeIndex::CreateBlob(olp::geo::TileKey root, int depth,
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

  // count data size(for now it is header version and data handle)
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
    *entry_ptr++ = {
        std::uint16_t(olp::geo::QuadKey64Helper{data.tile_key.ToQuadKey64()}
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

boost::optional<QuadTreeIndex::IndexData> QuadTreeIndex::Find(
    const olp::geo::TileKey& tile_key, bool aggregated) const {
  if (IsNull()) {
    return boost::none;
  }
  const olp::geo::TileKey& root_tile_key =
      olp::geo::TileKey::FromQuadKey64(data_->root_tilekey);

  IndexData data;
  if (tile_key.Level() >= root_tile_key.Level()) {
    auto sub = std::uint16_t(tile_key.GetSubkey64(
        static_cast<int>(tile_key.Level() - root_tile_key.Level())));

    const SubEntry* end = SubEntryEnd();
    const SubEntry* entry =
        std::lower_bound(SubEntryBegin(), end, SubEntry{sub, 0});
    if (entry == end || entry->sub_quadkey != sub) {
      return aggregated ? FindNearestParent(tile_key) : boost::none;
    }
    const auto offset = entry->tag_offset;

    // The limit is the offset for the next entry, or the beginning of the
    // parents entries. (in case there are no parents use the size of the index
    // block.)
    const auto limit = [&]() {
      auto next = entry + 1;
      if (next == end) {
        if (data_->parent_count == 0) {
          return static_cast<uint32_t>(raw_data_->size());
        } else {
          return ParentEntryBegin()->tag_offset;
        }
      }
      return next->tag_offset;
    }();

    if (!ReadIndexData(data, offset, limit)) {
      return boost::none;
    }
    data.tile_key = tile_key;
    return data;
  } else {
    std::uint64_t key = tile_key.ToQuadKey64();

    const ParentEntry* end = ParentEntryEnd();
    const ParentEntry* entry =
        std::lower_bound(ParentEntryBegin(), end, ParentEntry{key, 0});
    if (entry == end || entry->key != key) {
      return aggregated ? FindNearestParent(tile_key) : boost::none;
    }
    const auto offset = entry->tag_offset;

    // The limit is the offset for the next entry, or the end of the index data.
    const auto limit = [&]() {
      auto next = entry + 1;
      return (next == end) ? raw_data_->size() : next->tag_offset;
    }();

    if (!ReadIndexData(data, offset, limit)) {
      return boost::none;
    }
    data.tile_key = tile_key;
    return data;
  }
}
boost::optional<QuadTreeIndex::IndexData> QuadTreeIndex::FindNearestParent(
    geo::TileKey tile_key) const {
  const olp::geo::TileKey& root_tile_key =
      olp::geo::TileKey::FromQuadKey64(data_->root_tilekey);

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
        if (!ReadIndexData(data, it->tag_offset, limit)) {
          return boost::none;
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
      if (!ReadIndexData(data, it->tag_offset, limit)) {
        return boost::none;
      }
      return data;
    }
    limit = it->tag_offset;
  }
  return boost::none;
}

std::vector<QuadTreeIndex::IndexData> QuadTreeIndex::GetIndexData() const {
  std::vector<QuadTreeIndex::IndexData> result;
  if (IsNull()) {
    return result;
  }
  result.reserve(data_->parent_count + data_->subkey_count);

  auto limit = static_cast<uint32_t>(raw_data_->size());

  for (auto it = ParentEntryEnd(); it-- != ParentEntryBegin();) {
    QuadTreeIndex::IndexData data;
    data.tile_key = geo::TileKey::FromQuadKey64(it->key);
    if (ReadIndexData(data, it->tag_offset, limit)) {
      result.emplace_back(std::move(data));
    }
    limit = it->tag_offset;
  }

  for (auto it = SubEntryEnd(); it-- != SubEntryBegin();) {
    QuadTreeIndex::IndexData data;
    const olp::geo::TileKey& root_tile_key =
        olp::geo::TileKey::FromQuadKey64(data_->root_tilekey);
    auto subtile = root_tile_key.AddedSubkey64(std::uint64_t(it->sub_quadkey));
    data.tile_key = subtile;
    if (ReadIndexData(data, it->tag_offset, limit)) {
      result.emplace_back(std::move(data));
    }
    limit = it->tag_offset;
  }
  return result;
}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
