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

#include <rapidjson/document.h>
#include <algorithm>
#include <bitset>
#include <iostream>
#include "generated/model/Index.h"
// clang-format off
#include "generated/parser/IndexParser.h"
#include "generated/parser/PartitionsParser.h"
#include <olp/core/generated/parser/JsonParser.h>
// clang-format on

namespace olp {
namespace dataservice {
namespace read {

QuadTreeIndex::LayerIndex QuadTreeIndex::FromJson(const olp::geo::TileKey& root,
                                                  int depth,
                                                  const std::string& json) {
  std::vector<LayerIndex::IndexData> subs;
  std::vector<LayerIndex::IndexData> parents;

  model::Index idx = olp::parser::parse<model::Index>(json);
  // TODO reduce copying from index
  for (const auto& subQuad : idx.GetSubQuads()) {
    LayerIndex::IndexData record;
    record.tileKey_ = root.AddedSubHereTile(subQuad->GetSubQuadKey());
    record.data_handle_ = subQuad->GetDataHandle();
    record.version_ = subQuad->GetVersion();
    subs.push_back(std::move(record));
  }

  std::sort(subs.begin(), subs.end());

  for (auto& parentQuad : idx.GetParentQuads()) {
    LayerIndex::IndexData record;
    record.tileKey_ = root.FromQuadKey(parentQuad->GetPartition());
    record.data_handle_ = parentQuad->GetDataHandle();
    record.version_ = parentQuad->GetVersion();
    parents.push_back(std::move(record));
  }

  std::sort(parents.begin(), parents.end());

  return LayerIndex(root, depth, std::move(parents), std::move(subs));
}

void QuadTreeIndex::FromLayerIndex(const QuadTreeIndex::LayerIndex& index) {
  std::vector<LayerIndex::IndexData> subs = index.subs_;
  std::vector<LayerIndex::IndexData> parents = index.parents_;

  // quads must be sorted by their sub quad key, not by Quad::operator<
  std::sort(
      subs.begin(), subs.end(),
      [](const LayerIndex::IndexData& q1, const LayerIndex::IndexData& q2) {
        return q1.tileKey_.ToQuadKey64() < q2.tileKey_.ToQuadKey64();
      });

  std::sort(
      parents.begin(), parents.end(),
      [](const LayerIndex::IndexData& q1, const LayerIndex::IndexData& q2) {
        return q1.tileKey_.ToQuadKey64() < q2.tileKey_.ToQuadKey64();
      });

  // count data size(for now it is header version and data handle)
  size_t additionalDataSize = 0;
  for (const LayerIndex::IndexData& data : subs) {
    additionalDataSize += data.data_handle_.size() +
                          sizeof(AdditionalDataCompacted::data_header_) +
                          sizeof(AdditionalDataCompacted::version_);
  }
  for (const LayerIndex::IndexData& data : parents) {
    additionalDataSize += data.data_handle_.size() +
                          sizeof(AdditionalDataCompacted::data_header_) +
                          sizeof(AdditionalDataCompacted::version_);
  }

  // calculate and allocate size
  size_ = sizeof(DataHeader) - sizeof(SubEntry) +
          (subs.size() * sizeof(SubEntry)) +
          (parents.size() * sizeof(ParentEntry)) + additionalDataSize;

  raw_data_ = std::make_shared<std::vector<unsigned char>>(
      std::vector<unsigned char>(size_));
  data_ = reinterpret_cast<DataHeader*>(&(raw_data_->front()));

  data_->root_tilekey_ = index.root_.ToQuadKey64();
  data_->depth_ = uint8_t(index.depth_);
  data_->subkey_count_ = uint16_t(subs.size());
  data_->parent_count_ = uint8_t(parents.size());

  // write SubEntry tiles
  SubEntry* entryPtr = data_->entries_;
  char* dataPtr = const_cast<char*>(DataBegin());
  std::uint16_t dataOffset = 0u;

  auto rootQuadLevel = index.root_.Level();

  uint8_t header = BitSetFlags::kVersion | BitSetFlags::kDataHandle;

  for (const LayerIndex::IndexData& data : subs) {
    *entryPtr++ = {
        std::uint16_t(olp::geo::QuadKey64Helper{data.tileKey_.ToQuadKey64()}
                          .GetSubkey(data.tileKey_.Level() - rootQuadLevel)
                          .key),
        dataOffset};
    // write additional data

    AdditionalDataCompacted* additional_data =
        reinterpret_cast<AdditionalDataCompacted*>(dataPtr + dataOffset);
    additional_data->data_header_ = header;     // write header
    additional_data->version_ = data.version_;  // write version
    memcpy(additional_data->data_handle_, data.data_handle_.data(),
           data.data_handle_.size());  // data_handle
    dataOffset += uint16_t(sizeof(*additional_data) -
                           sizeof(additional_data->data_handle_) +
                           data.data_handle_.size());
  }

  ParentEntry* parentPtr = reinterpret_cast<ParentEntry*>(entryPtr);
  for (const LayerIndex::IndexData& data : parents) {
    *parentPtr++ = {data.tileKey_.ToQuadKey64(), dataOffset};

    // write additional data
    AdditionalDataCompacted* additional_data =
        reinterpret_cast<AdditionalDataCompacted*>(dataPtr + dataOffset);
    additional_data->data_header_ = header;     // write header
    additional_data->version_ = data.version_;  // write version
    memcpy(additional_data->data_handle_, data.data_handle_.data(),
           data.data_handle_.size());  // data_handle
    dataOffset += uint16_t(sizeof(*additional_data) -
                           sizeof(additional_data->data_handle_) +
                           data.data_handle_.size());
  }
}

QuadTreeIndex::LayerIndex::IndexData QuadTreeIndex::find(
    const olp::geo::TileKey& tileKey) const {
  if (IsNull())
    return {};

  const olp::geo::TileKey& rootTileKey =
      olp::geo::TileKey::FromQuadKey64(data_->root_tilekey_);

  if (tileKey.Level() >= rootTileKey.Level()) {
    std::uint16_t sub = std::uint16_t(
        tileKey.GetSubkey64(tileKey.Level() - rootTileKey.Level()));

    const SubEntry* end = SubEntryEnd();
    const SubEntry* entry =
        std::lower_bound(SubEntryBegin(), end, SubEntry{sub, 0});
    if (entry == end || entry->sub_quadkey_ != sub)
      return LayerIndex::IndexData{tileKey, std::string(), 0};
    QuadTreeIndex::AdditionalData tile_data = TileData(entry);
    return LayerIndex::IndexData{tileKey, std::move(tile_data.data_handle_),
                                 tile_data.version_};
  } else {
    std::uint64_t key = tileKey.ToQuadKey64();

    const ParentEntry* end = ParentEntryEnd();
    const ParentEntry* entry =
        std::lower_bound(ParentEntryBegin(), end, ParentEntry{key, 0});
    if (entry == end || entry->key_ != key)
      return LayerIndex::IndexData{tileKey, std::string(), 0};
    QuadTreeIndex::AdditionalData tile_data = TileData(entry);
    return LayerIndex::IndexData{tileKey, std::move(tile_data.data_handle_),
                                 tile_data.version_};
  }
}

QuadTreeIndex::AdditionalData QuadTreeIndex::TileData(
    const SubEntry* entry) const {
  const SubEntry* end = SubEntryEnd();
  const char* tagBegin = DataBegin() + entry->tag_offset_;
  const char* tagEnd =
      entry + 1 == end
          ? (data_->parent_count_ > 0
                 ? tagBegin + ((reinterpret_cast<const ParentEntry*>(entry + 1))
                                   ->tag_offset_ -
                               entry->tag_offset_)
                 : DataEnd())
          : tagBegin + ((entry + 1)->tag_offset_ - entry->tag_offset_);
  return TileData(tagBegin, tagEnd);
}

QuadTreeIndex::AdditionalData QuadTreeIndex::TileData(
    const ParentEntry* entry) const {
  const ParentEntry* end = ParentEntryEnd();
  const char* tagBegin = DataBegin() + entry->tag_offset_;
  const char* tagEnd =
      entry + 1 == end
          ? DataEnd()
          : tagBegin + ((entry + 1)->tag_offset_ - entry->tag_offset_);
  return TileData(tagBegin, tagEnd);
}

QuadTreeIndex::AdditionalData QuadTreeIndex::TileData(
    const char* tagBegin, const char* tagEnd) const {
  const AdditionalDataCompacted* additional_data =
      reinterpret_cast<const AdditionalDataCompacted*>(tagBegin);
  // here we could check flags if in future will be multiple versions of
  // packaging additional data
  auto handle =
      std::string((const char*)(additional_data->data_handle_),
                  tagEnd - (const char*)(additional_data->data_handle_));
  return {additional_data->version_, std::move(handle)};
}

QuadTreeIndex::~QuadTreeIndex() {}

}  // namespace read
}  // namespace dataservice
}  // namespace olp
