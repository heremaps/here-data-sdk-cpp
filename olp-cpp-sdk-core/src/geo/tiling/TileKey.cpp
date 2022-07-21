/*
 * Copyright (C) 2019-2020 HERE Europe B.V.
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

#include "olp/core/geo/tiling/TileKey.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <string>

#include <olp/core/porting/warning_disable.h>

namespace olp {
namespace geo {

std::string TileKey::ToQuadKey() const {
  if (!IsValid()) {
    return {};
  }
  if (level_ == 0) {
    return "-";
  }

  std::string key;
  key.resize(level_);
  const auto morton_key = ToQuadKey64();

  for (std::uint32_t index = 0; index < level_; ++index) {
    // (morton_key / 4**i % 4) is the two bit group in base 4
    key[level_ - index - 1] = (morton_key / (1ull << (index << 1))) % 4 + '0';
  }

  return key;
}

TileKey TileKey::FromQuadKey(const std::string& quad_key) {
  if (quad_key.empty()) {
    return TileKey();
  }

  TileKey result{TileKey::FromRowColumnLevel(0, 0, 0)};
  if (quad_key == "-") {
    return result;
  }

  result.level_ = std::uint32_t(quad_key.size());
  for (size_t index = 0; index < quad_key.size(); ++index) {
    const int mask = 1 << index;
    int d = quad_key[result.level_ - index - 1] - '0';
    if (d & 0x1) {
      result.column_ |= mask;
    }
    if (d & 0x2) {
      result.row_ |= mask;
    }
  }
  return result;
}

std::string TileKey::ToHereTile() const {
  return std::to_string(ToQuadKey64());
}

TileKey TileKey::FromHereTile(const std::string& key) {
  if (key.empty()) {
    return {};
  }

  char* ptr;
  return FromQuadKey64(strtoull(key.data(), &ptr, 10));
}

std::uint64_t TileKey::ToQuadKey64() const {
  // This table interleaves the bits ex: 11 -> 1010
  static const std::uint64_t kMortonTable256[256] = {
      0x0000, 0x0001, 0x0004, 0x0005, 0x0010, 0x0011, 0x0014, 0x0015, 0x0040,
      0x0041, 0x0044, 0x0045, 0x0050, 0x0051, 0x0054, 0x0055, 0x0100, 0x0101,
      0x0104, 0x0105, 0x0110, 0x0111, 0x0114, 0x0115, 0x0140, 0x0141, 0x0144,
      0x0145, 0x0150, 0x0151, 0x0154, 0x0155, 0x0400, 0x0401, 0x0404, 0x0405,
      0x0410, 0x0411, 0x0414, 0x0415, 0x0440, 0x0441, 0x0444, 0x0445, 0x0450,
      0x0451, 0x0454, 0x0455, 0x0500, 0x0501, 0x0504, 0x0505, 0x0510, 0x0511,
      0x0514, 0x0515, 0x0540, 0x0541, 0x0544, 0x0545, 0x0550, 0x0551, 0x0554,
      0x0555, 0x1000, 0x1001, 0x1004, 0x1005, 0x1010, 0x1011, 0x1014, 0x1015,
      0x1040, 0x1041, 0x1044, 0x1045, 0x1050, 0x1051, 0x1054, 0x1055, 0x1100,
      0x1101, 0x1104, 0x1105, 0x1110, 0x1111, 0x1114, 0x1115, 0x1140, 0x1141,
      0x1144, 0x1145, 0x1150, 0x1151, 0x1154, 0x1155, 0x1400, 0x1401, 0x1404,
      0x1405, 0x1410, 0x1411, 0x1414, 0x1415, 0x1440, 0x1441, 0x1444, 0x1445,
      0x1450, 0x1451, 0x1454, 0x1455, 0x1500, 0x1501, 0x1504, 0x1505, 0x1510,
      0x1511, 0x1514, 0x1515, 0x1540, 0x1541, 0x1544, 0x1545, 0x1550, 0x1551,
      0x1554, 0x1555, 0x4000, 0x4001, 0x4004, 0x4005, 0x4010, 0x4011, 0x4014,
      0x4015, 0x4040, 0x4041, 0x4044, 0x4045, 0x4050, 0x4051, 0x4054, 0x4055,
      0x4100, 0x4101, 0x4104, 0x4105, 0x4110, 0x4111, 0x4114, 0x4115, 0x4140,
      0x4141, 0x4144, 0x4145, 0x4150, 0x4151, 0x4154, 0x4155, 0x4400, 0x4401,
      0x4404, 0x4405, 0x4410, 0x4411, 0x4414, 0x4415, 0x4440, 0x4441, 0x4444,
      0x4445, 0x4450, 0x4451, 0x4454, 0x4455, 0x4500, 0x4501, 0x4504, 0x4505,
      0x4510, 0x4511, 0x4514, 0x4515, 0x4540, 0x4541, 0x4544, 0x4545, 0x4550,
      0x4551, 0x4554, 0x4555, 0x5000, 0x5001, 0x5004, 0x5005, 0x5010, 0x5011,
      0x5014, 0x5015, 0x5040, 0x5041, 0x5044, 0x5045, 0x5050, 0x5051, 0x5054,
      0x5055, 0x5100, 0x5101, 0x5104, 0x5105, 0x5110, 0x5111, 0x5114, 0x5115,
      0x5140, 0x5141, 0x5144, 0x5145, 0x5150, 0x5151, 0x5154, 0x5155, 0x5400,
      0x5401, 0x5404, 0x5405, 0x5410, 0x5411, 0x5414, 0x5415, 0x5440, 0x5441,
      0x5444, 0x5445, 0x5450, 0x5451, 0x5454, 0x5455, 0x5500, 0x5501, 0x5504,
      0x5505, 0x5510, 0x5511, 0x5514, 0x5515, 0x5540, 0x5541, 0x5544, 0x5545,
      0x5550, 0x5551, 0x5554, 0x5555};

  // the bits of x and y are alternated, y_n-1 x_n-1 .... y_0 x_0
  return 1ull << (2 * level_) | kMortonTable256[(row_ >> 24) & 0xFF] << 49 |
         kMortonTable256[(row_ >> 16) & 0xFF] << 33 |
         kMortonTable256[(row_ >> 8) & 0xFF] << 17 |
         kMortonTable256[row_ & 0xFF] << 1 |
         kMortonTable256[(column_ >> 24) & 0xFF] << 48 |
         kMortonTable256[(column_ >> 16) & 0xFF] << 32 |
         kMortonTable256[(column_ >> 8) & 0xFF] << 16 |
         kMortonTable256[column_ & 0xFF];
}

TileKey TileKey::FromQuadKey64(std::uint64_t quad_key) {
  auto result = TileKey::FromRowColumnLevel(0, 0, 0);

  while (quad_key > 1) {
    const int mask = 1 << result.level_;

    if (quad_key & 0x1) {
      result.column_ |= mask;
    }
    if (quad_key & 0x2) {
      result.row_ |= mask;
    }

    result.level_++;
    quad_key >>= 2;
  }

  return result;
}

TileKey TileKey::FromRowColumnLevel(std::uint32_t row, std::uint32_t column,
                                    std::uint32_t level) {
  TileKey result;
  result.row_ = row;
  result.column_ = column;
  result.level_ = level;
  return result;
}

TileKey TileKey::Parent() const {
  if (level_ > 0) {
    return FromRowColumnLevel(row_ >> 1, column_ >> 1, level_ - 1);
  }

  return {};
}

bool TileKey::IsChildOf(const TileKey& tile_key) const {
  if (level_ <= tile_key.Level()) {
    return false;
  }

  const auto child_ancestor = ChangedLevelTo(tile_key.Level());
  return child_ancestor == tile_key;
}

bool TileKey::IsParentOf(const TileKey& tile_key) const {
  return tile_key.IsChildOf(*this);
}

TileKey TileKey::ChangedLevelBy(int delta) const {
  if (delta == 0) {
    return *this;
  }

  TileKey result = *this;
  int level = (level_ + delta);
  if (level < 0) {
    level = 0;
  }

  result.level_ = level;

  if (delta >= 0) {
    result.row_ <<= delta;
    result.column_ <<= delta;
  } else {
    result.row_ >>= -delta;
    result.column_ >>= -delta;
  }

  return result;
}

TileKey TileKey::ChangedLevelTo(std::uint32_t level) const {
  return ChangedLevelBy(static_cast<int>(level) - level_);
}

std::uint64_t TileKey::GetSubkey64(int delta) const {
  return QuadKey64Helper{ToQuadKey64()}.GetSubkey(delta).key;
}

TileKey TileKey::AddedSubkey64(std::uint64_t sub_quad_key) const {
  return TileKey::FromQuadKey64(QuadKey64Helper{ToQuadKey64()}
                                    .AddedSubkey(QuadKey64Helper{sub_quad_key})
                                    .key);
}

TileKey TileKey::AddedSubkey(const std::string& sub_quad_key) const {
  const TileKey& sub_quad =
      TileKey::FromQuadKey(sub_quad_key.empty() ? "-" : sub_quad_key);
  const TileKey& child = ChangedLevelBy(sub_quad.Level());
  return TileKey::FromRowColumnLevel(child.Row() + sub_quad.Row(),
                                     child.Column() + sub_quad.Column(),
                                     child.Level());
}

TileKey TileKey::AddedSubHereTile(const std::string& sub_here_tile) const {
  const TileKey& sub_quad = TileKey::FromHereTile(sub_here_tile);
  const TileKey& child = ChangedLevelBy(sub_quad.Level());
  return TileKey::FromRowColumnLevel(child.Row() + sub_quad.Row(),
                                     child.Column() + sub_quad.Column(),
                                     child.Level());
}

TileKey TileKey::NextRow() const {
  return FromRowColumnLevel(row_ + 1, column_, level_);
}

TileKey TileKey::NextColumn() const {
  return FromRowColumnLevel(row_, column_ + 1, level_);
}

TileKey TileKey::PreviousRow() const {
  return FromRowColumnLevel(row_ - 1, column_, level_);
}

TileKey TileKey::PreviousColumn() const {
  return FromRowColumnLevel(row_, column_ - 1, level_);
}

QuadKey64Helper QuadKey64Helper::GetSubkey(int delta) const {
  const std::uint64_t msb = (1ull << (delta * 2));
  const std::uint64_t mask = msb - 1;

  const std::uint64_t result = (key & mask) | msb;
  return QuadKey64Helper{result};
}

QuadKey64Helper QuadKey64Helper::AddedSubkey(QuadKey64Helper sub_key) const {
  QuadKey64Helper result{key};

  std::uint64_t mask = 0x1;
  while (sub_key.key >= (mask << 2)) {
    result.key <<= 2;
    mask <<= 2;
  }

  result.key |= sub_key.key & (mask - 1);

  return result;
}

TileKey TileKey::GetChild(std::uint8_t index) const {
  auto result = ChangedLevelBy(1);

  result.column_ |= (index & 1);
  result.row_ |= (index >> 1);

  return result;
}

TileKey TileKey::GetChild(TileKeyQuadrant direction) const {
  return GetChild(static_cast<std::uint8_t>(direction));
}

TileKey::TileKeyQuadrant TileKey::RelationshipToParent() const {
  if (level_ == 0) {
    return TileKey::TileKeyQuadrant::Invalid;
  }
  const TileKey sw_tile = Parent().GetChild(0);
  return static_cast<TileKey::TileKeyQuadrant>((Row() - sw_tile.Row()) * 2 +
                                               Column() - sw_tile.Column());
}

boost::optional<std::uint32_t> GetMinTileKeyLevel(const TileKeyLevels& levels) {
  if (levels.none()) {
    return boost::none;
  }

  const auto level_bits = levels.to_ulong();

  // See http://supertech.csail.mit.edu/papers/debruijn.pdf to dive into the
  // magic
  static const std::uint32_t kLookupTable[32] = {
      0,  1,  28, 2,  29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4,  8,
      31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6,  11, 5,  10, 9};

  PORTING_PUSH_WARNINGS();
  PORTING_MSVC_DISABLE_WARNINGS(4146);

  return kLookupTable[std::uint32_t((level_bits & -level_bits) * 0x077CB531U) >>
                      27];

  PORTING_POP_WARNINGS();
}

boost::optional<std::uint32_t> GetMaxTileKeyLevel(const TileKeyLevels& levels) {
  if (levels.none()) {
    return boost::none;
  }

  return static_cast<std::uint32_t>(std::log2(levels.to_ulong()));
}

boost::optional<std::uint32_t> GetNearestAvailableTileKeyLevel(
    const TileKeyLevels& levels, const std::uint32_t reference_level) {
  const auto min_level = geo::GetMinTileKeyLevel(levels);
  const auto max_level = geo::GetMaxTileKeyLevel(levels);
  if (!min_level || !max_level) {
    return boost::none;
  }

  int level = std::max(std::min(reference_level, *max_level), *min_level);
  const int max_distance = std::max(abs(static_cast<int>(level - *min_level)),
                                    abs(static_cast<int>(level - *max_level)));
  const int storage_levels_size = static_cast<int>(levels.size());
  for (int distance = 0; distance <= max_distance; distance++) {
    if ((level + distance) < storage_levels_size && levels[level + distance]) {
      level += distance;
      break;
    }
    if ((level >= distance) && levels[level - distance]) {
      level -= distance;
      break;
    }
  }

  return level;
}

std::ostream& operator<<(std::ostream& out, const geo::TileKey& tile_key) {
  out << "(l:" << tile_key.Level() << " r:" << tile_key.Row()
      << " c:" << tile_key.Column() << ")";
  return out;
}

}  // namespace geo
}  // namespace olp
