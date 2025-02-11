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

#include "ReadDefaultResponses.h"

#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include <olp/core/generated/serializer/SerializerWrapper.h>

#include <boost/json/serialize.hpp>
#include <boost/json/value.hpp>

namespace {

void WriteSubquadsToJson(
    boost::json::object& doc, const olp::geo::TileKey& root_tile,
    const std::map<std::uint64_t, mockserver::TileMetadata>& sub_quads) {
  boost::json::array sub_quads_value;
  for (const auto& quad : sub_quads) {
    const mockserver::TileMetadata& metadata = quad.second;

    const auto partition = root_tile.AddedSubkey64(quad.first).ToHereTile();

    boost::json::object item;
    olp::serializer::serialize("subQuadKey", std::to_string(quad.first), item);
    olp::serializer::serialize("version", metadata.version, item);
    olp::serializer::serialize("dataHandle", metadata.data_handle, item);
    olp::serializer::serialize("crc", metadata.crc, item);
    olp::serializer::serialize("checksum", metadata.checksum, item);
    olp::serializer::serialize("dataSize", metadata.data_size, item);
    olp::serializer::serialize("compressedDataSize",
                               metadata.compressed_data_size, item);

    sub_quads_value.emplace_back(std::move(item));
  }
  doc.emplace("subQuads", std::move(sub_quads_value));
}

void WriteParentquadsToJson(
    boost::json::object& doc,
    const std::map<std::uint64_t, mockserver::TileMetadata>& parent_quads) {
  boost::json::array parent_quads_value;
  for (const auto& parent : parent_quads) {
    const auto partition = std::to_string(parent.first);
    const mockserver::TileMetadata& metadata = parent.second;

    boost::json::object item;
    olp::serializer::serialize("partition", partition, item);
    olp::serializer::serialize("version", metadata.version, item);
    olp::serializer::serialize("dataHandle", metadata.data_handle, item);
    olp::serializer::serialize("crc", metadata.crc, item);
    olp::serializer::serialize("checksum", metadata.checksum, item);
    olp::serializer::serialize("dataSize", metadata.data_size, item);
    olp::serializer::serialize("compressedDataSize",
                               metadata.compressed_data_size, item);
    parent_quads_value.emplace_back(std::move(item));
  }

  doc.emplace("parentQuads", std::move(parent_quads_value));
}

std::string GenerateRandomString(size_t length) {
  std::string letters =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  std::random_device device;
  std::mt19937 generator(device());
  std::uniform_int_distribution<unsigned int> dis(0u, letters.length() - 1);

  std::string result;
  result.resize(length);
  for (auto i = 0u; i < length; ++i) {
    result[i] += letters[dis(generator)];
  }

  return result;
}

void FillSubQuads(std::int32_t depth, std::vector<std::uint16_t>& sub_quads_) {
  const auto sub_tile = olp::geo::TileKey::FromRowColumnLevel(0, 0, depth);
  const auto start_level_id = sub_tile.ToQuadKey64();
  const auto tiles_count = olp::geo::QuadKey64Helper::ChildrenAtLevel(depth);

  std::vector<std::uint64_t> layer_ids(tiles_count);
  std::iota(layer_ids.begin(), layer_ids.end(), start_level_id);
  sub_quads_.insert(sub_quads_.end(), layer_ids.begin(), layer_ids.end());
}

mockserver::TileMetadata MakePartition(std::string data_handle,
                                       boost::optional<int32_t> version) {
  mockserver::TileMetadata partition;
  partition.data_handle = std::move(data_handle);
  partition.version = version;
  partition.crc = GenerateRandomString(6);
  partition.checksum = GenerateRandomString(32);
  partition.data_size = 100;
  partition.compressed_data_size = 10;
  return partition;
}

}  // namespace

namespace mockserver {

std::string ReadDefaultResponses::GenerateData(size_t length) {
  return GenerateRandomString(length);
}

std::string ReadDefaultResponses::GenerateQuadTreeResponse(
    olp::geo::TileKey root_tile, std::uint32_t depth,
    const std::vector<std::uint32_t>& available_levels) {
  std::map<std::uint64_t, TileMetadata> sub_quads;
  std::map<std::uint64_t, TileMetadata> parent_quads;

  // generate data
  for (auto level : available_levels) {
    if (level < root_tile.Level()) {
      auto key = root_tile.ChangedLevelTo(level).ToQuadKey64();
      parent_quads[key] =
          MakePartition(GenerateDataHandle(std::to_string(key)), 0);
    } else {
      const auto level_depth = level - root_tile.Level();
      if (level_depth > depth) {
        continue;
      }

      std::vector<std::uint16_t> sub_quads_vector;
      FillSubQuads(level_depth, sub_quads_vector);

      for (const auto& sub_quad : sub_quads_vector) {
        const auto partition = root_tile.AddedSubkey64(sub_quad).ToHereTile();
        sub_quads[sub_quad] = MakePartition(GenerateDataHandle(partition), 0);
      }
    }
  }

  boost::json::object doc;
  WriteSubquadsToJson(doc, root_tile, sub_quads);
  WriteParentquadsToJson(doc, parent_quads);

  return boost::json::serialize(doc);
}

QuadTreeBuilder::QuadTreeBuilder(olp::geo::TileKey root_tile,
                                 boost::optional<int32_t> version)
    : root_tile_(root_tile), base_version_(version) {}

QuadTreeBuilder& QuadTreeBuilder::WithParent(olp::geo::TileKey parent,
                                             std::string data_handle,
                                             boost::optional<int32_t> version) {
  assert(root_tile_.IsChildOf(parent));

  // Make sure to set version when the base_version is set
  if (version != boost::none) {
    assert(base_version_ != boost::none);
  } else if (base_version_) {
    version = base_version_;
  }

  parent_quads_[parent.ToQuadKey64()] =
      MakePartition(std::move(data_handle), version);

  return *this;
}

QuadTreeBuilder& QuadTreeBuilder::FillParents() {
  auto key = root_tile_.Parent();
  while (key.IsValid()) {
    auto quad_key = key.ToQuadKey64();
    if (parent_quads_.find(quad_key) == parent_quads_.end()) {
      parent_quads_[quad_key] =
          MakePartition(GenerateRandomString(32), base_version_);
    }
  }
  return *this;
}

QuadTreeBuilder& QuadTreeBuilder::WithSubQuad(
    olp::geo::TileKey tile, std::string datahandle,
    boost::optional<int32_t> version) {
  assert(tile.IsChildOf(root_tile_) || tile == root_tile_);
  assert((tile.Level() - root_tile_.Level()) <= 4);
  if (version != boost::none) {
    assert(base_version_ != boost::none);
  }

  auto origin = root_tile_.ChangedLevelTo(tile.Level());
  auto sub_quad =
      olp::geo::TileKey::FromRowColumnLevel(tile.Row() - origin.Row(),
                                            tile.Column() - origin.Column(),
                                            tile.Level() - root_tile_.Level())
          .ToQuadKey64();

  sub_quads_[sub_quad] = MakePartition(std::move(datahandle), version);

  return *this;
}

std::string QuadTreeBuilder::BuildJson() const {
  boost::json::object doc;
  WriteSubquadsToJson(doc, root_tile_, sub_quads_);
  WriteParentquadsToJson(doc, parent_quads_);

  return boost::json::serialize(doc);
}

olp::geo::TileKey QuadTreeBuilder::Root() const { return root_tile_; }

}  // namespace mockserver
