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

#include "ReadDefaultResponses.h"

#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include <olp/core/generated/serializer/SerializerWrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace {

void WriteSubquadsToJson(
    rapidjson::Document& doc, const olp::geo::TileKey& root_tile,
    const std::map<std::uint64_t, mockserver::TileMetadata>& sub_quads,
    rapidjson::Document::AllocatorType& allocator) {
  rapidjson::Value sub_quads_value;
  sub_quads_value.SetArray();
  for (auto quad : sub_quads) {
    const auto partition = root_tile.AddedSubkey64(quad.first).ToHereTile();
    const auto& data_handle = quad.second.data_handle;
    const auto version = quad.second.version;

    rapidjson::Value item_value;
    item_value.SetObject();
    olp::serializer::serialize("subQuadKey", std::to_string(quad.first),
                               item_value, allocator);
    olp::serializer::serialize("version", version, item_value, allocator);
    olp::serializer::serialize("dataHandle", data_handle, item_value,
                               allocator);
    sub_quads_value.PushBack(std::move(item_value), allocator);
  }
  doc.AddMember("subQuads", std::move(sub_quads_value), allocator);
}

void WriteParentquadsToJson(
    rapidjson::Document& doc,
    const std::map<std::uint64_t, mockserver::TileMetadata>& parent_quads,
    rapidjson::Document::AllocatorType& allocator) {
  rapidjson::Value parent_quads_value;
  parent_quads_value.SetArray();
  for (auto parent : parent_quads) {
    const auto partition = std::to_string(parent.first);
    const auto version = parent.second.version;
    const auto& data_handle = parent.second.data_handle;

    rapidjson::Value item_value;
    item_value.SetObject();
    olp::serializer::serialize("partition", partition, item_value, allocator);
    olp::serializer::serialize("version", version, item_value, allocator);
    olp::serializer::serialize("dataHandle", data_handle, item_value,
                               allocator);
    olp::serializer::serialize("dataSize", 100, item_value, allocator);
    parent_quads_value.PushBack(std::move(item_value), allocator);
  }
  doc.AddMember("parentQuads", std::move(parent_quads_value), allocator);
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
      parent_quads[key] = {GenerateDataHandle(std::to_string(key)), 0};
    } else {
      const auto level_depth = level - root_tile.Level();
      if (level_depth > depth) {
        continue;
      }

      std::vector<std::uint16_t> sub_quads_vector;
      FillSubQuads(level_depth, sub_quads_vector);

      for (const auto& sub_quad : sub_quads_vector) {
        const auto partition = root_tile.AddedSubkey64(sub_quad).ToHereTile();
        sub_quads[sub_quad] = {GenerateDataHandle(partition), 0};
      }
    }
  }

  rapidjson::Document doc;
  auto& allocator = doc.GetAllocator();
  doc.SetObject();
  WriteSubquadsToJson(doc, root_tile, sub_quads, allocator);
  WriteParentquadsToJson(doc, parent_quads, allocator);

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);
  return buffer.GetString();
}

QuadTreeBuilder::QuadTreeBuilder(olp::geo::TileKey root_tile,
                                 boost::optional<int32_t> version)
    : root_tile_(root_tile), base_version_(version) {}

QuadTreeBuilder& QuadTreeBuilder::WithParent(olp::geo::TileKey parent,
                                             std::string datahandle,
                                             boost::optional<int32_t> version) {
  assert(root_tile_.IsChildOf(parent));

  // Make sure to set version when the base_version is set
  if (version != boost::none) {
    assert(base_version_ != boost::none);
  } else if (base_version_) {
    version = base_version_;
  }

  parent_quads_[parent.ToQuadKey64()] = {datahandle, version};

  return *this;
}

QuadTreeBuilder& QuadTreeBuilder::FillParents() {
  auto key = root_tile_.Parent();
  while (key.IsValid()) {
    auto quad_key = key.ToQuadKey64();
    if (parent_quads_.find(quad_key) == parent_quads_.end()) {
      parent_quads_[quad_key] = {GenerateRandomString(32), base_version_};
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

  sub_quads_[sub_quad] = {datahandle, version};

  return *this;
}

QuadTreeBuilder& QuadTreeBuilder::FillSubquads(uint32_t depth) {
  assert(depth <= 4);

  std::vector<std::uint16_t> sub_quads;
  for (uint32_t i = 0; i <= depth; i++) {
    FillSubQuads(i, sub_quads);
  }

  for (const auto& sub_quad : sub_quads) {
    if (sub_quads_.find(sub_quad) == sub_quads_.end()) {
      sub_quads_[sub_quad] = {GenerateRandomString(32), base_version_};
    }
  }

  return *this;
}

std::string QuadTreeBuilder::BuildJson() const {
  rapidjson::Document doc;
  auto& allocator = doc.GetAllocator();
  doc.SetObject();
  WriteSubquadsToJson(doc, root_tile_, sub_quads_, allocator);
  WriteParentquadsToJson(doc, parent_quads_, allocator);

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);
  return buffer.GetString();
}

olp::geo::TileKey QuadTreeBuilder::Root() const { return root_tile_; }

}  // namespace mockserver
