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

#include <string>
#include <utility>
#include <vector>

#include <olp/core/generated/serializer/SerializerWrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace {
void WriteSubquadsToJson(rapidjson::Document& doc,
                         const olp::geo::TileKey& root_tile,
                         const std::vector<std::uint16_t>& sub_quads,
                         rapidjson::Document::AllocatorType& allocator) {
  rapidjson::Value sub_quads_value;
  sub_quads_value.SetArray();
  for (auto quad : sub_quads) {
    const auto partition = root_tile.AddedSubkey64(quad).ToHereTile();
    const auto data_handle =
        mockserver::ReadDefaultResponses::GenerateDataHandle(partition);

    rapidjson::Value item_value;
    item_value.SetObject();
    olp::serializer::serialize("subQuadKey", std::to_string(quad), item_value,
                               allocator);
    olp::serializer::serialize("version", 0, item_value, allocator);
    olp::serializer::serialize("dataHandle", data_handle, item_value,
                               allocator);
    olp::serializer::serialize("dataSize", 100, item_value, allocator);
    sub_quads_value.PushBack(std::move(item_value), allocator);
  }
  doc.AddMember("subQuads", std::move(sub_quads_value), allocator);
}

void WriteParentquadsToJson(rapidjson::Document& doc,
                            const std::vector<std::uint64_t>& parent_quads,
                            rapidjson::Document::AllocatorType& allocator) {
  rapidjson::Value parent_quads_value;
  parent_quads_value.SetArray();
  for (auto parent : parent_quads) {
    const auto partition = std::to_string(parent);
    const auto data_handle =
        mockserver::ReadDefaultResponses::GenerateDataHandle(partition);

    rapidjson::Value item_value;
    item_value.SetObject();
    olp::serializer::serialize("partition", std::to_string(parent), item_value,
                               allocator);
    olp::serializer::serialize("version", 0, item_value, allocator);
    olp::serializer::serialize("dataHandle", data_handle, item_value,
                               allocator);
    olp::serializer::serialize("dataSize", 100, item_value, allocator);
    parent_quads_value.PushBack(std::move(item_value), allocator);
  }
  doc.AddMember("parentQuads", std::move(parent_quads_value), allocator);
}
}  // namespace

namespace mockserver {

std::string ReadDefaultResponses::GenerateQuadTreeResponse(
    olp::geo::TileKey root_tile, std::uint32_t depth,
    const std::vector<std::uint32_t>& available_levels) {
  std::vector<std::uint16_t> sub_quads;
  std::vector<std::uint64_t> parent_quads;

  // generate data
  for (auto level : available_levels) {
    if (level < root_tile.Level()) {
      auto key = root_tile.ChangedLevelTo(level);
      parent_quads.push_back(key.ToQuadKey64());
    } else {
      const auto level_depth = level - root_tile.Level();
      if (level_depth > depth) {
        continue;
      }

      const auto sub_tile =
          olp::geo::TileKey::FromRowColumnLevel(0, 0, level_depth);
      const auto start_level_id = sub_tile.ToQuadKey64();
      const auto tiles_count =
          olp::geo::QuadKey64Helper::ChildrenAtLevel(level_depth);

      std::vector<std::uint64_t> layer_ids(tiles_count);
      std::iota(layer_ids.begin(), layer_ids.end(), start_level_id);
      sub_quads.insert(sub_quads.end(), layer_ids.begin(), layer_ids.end());
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

}  // namespace mockserver
