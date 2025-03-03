/*
 * Copyright (C) 2025 HERE Europe B.V.
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

#include <olp/core/geo/tiling/PathTiling.h>

#include <unordered_set>

#include <olp/core/geo/tiling/TileKey.h>
#include <olp/core/geo/tiling/TileKeyUtils.h>

#include <deque>

#include "olp/core/geo/tiling/ISubdivisionScheme.h"
#include "olp/core/geo/tiling/ITilingScheme.h"

namespace olp {
namespace geo {
namespace {
class BresenhamLineGenerator {
 public:
  BresenhamLineGenerator(const int x0, const int y0, const int x1, const int y1,
                         const bool slope_reversed, const int radius)
      : x_end_(x1),
        is_slope_reversed_(slope_reversed),
        radius_(radius),
        delta_x_(x1 - x0),
        delta_y_(std::abs(y1 - y0)),
        y_step_(y0 > y1 ? -1 : 1),
        x_(x0),
        y_(y0),
        error_(0),
        a_(-radius_),
        b_(-radius_) {}

  bool Next(int& out_x, int& out_y) {
    if (x_ > x_end_) {
      return false;
    }

    out_x = x_ + a_;
    out_y = y_ + b_;

    if (is_slope_reversed_)
      std::swap(out_x, out_y);

    if (++b_ > radius_) {
      b_ = -radius_;
      if (++a_ > radius_) {
        a_ = -radius_;

        error_ += delta_y_;
        if (2 * error_ >= delta_x_) {
          y_ += y_step_;
          error_ -= delta_x_;
        }

        ++x_;
      }
    }

    return true;
  }

 private:
  int x_end_;
  bool is_slope_reversed_;
  int radius_;
  int delta_x_;
  int delta_y_;
  int y_step_;
  int x_;
  int y_;
  int error_;
  int a_;
  int b_;
};

boost::optional<BresenhamLineGenerator> MakeBresenhamLineGenerator(
    boost::optional<GeoSegmentsGeneratorBase::Segment> segment,
    const int radius, const ITilingScheme& tiling_scheme,
    const uint32_t tile_level) {
  if (!segment) {
    return boost::none;
  }

  const auto segment_start = olp::geo::TileKeyUtils::GeoCoordinatesToTileKey(
      tiling_scheme, std::get<0>(*segment), tile_level);
  const auto segment_end = olp::geo::TileKeyUtils::GeoCoordinatesToTileKey(
      tiling_scheme, std::get<1>(*segment), tile_level);

  int x0 = segment_start.Column();
  int y0 = segment_start.Row();
  int x1 = segment_end.Column();
  int y1 = segment_end.Row();

  const bool reversed_slope = std::abs(y1 - y0) > std::abs(x1 - x0);
  if (reversed_slope) {
    std::swap(x0, y0);
    std::swap(x1, y1);
  }
  if (x0 > x1) {
    std::swap(x0, x1);
    std::swap(y0, y1);
  }

  return BresenhamLineGenerator(x0, y0, x1, y1, reversed_slope, radius);
}

}  // namespace

// GeoPath -> GeoPairs -> TilePairs -> LineAlgorithm -> Expanding around

class PathTilingGenerator::PathTilingGeneratorImpl {
 public:
  PathTilingGeneratorImpl(
      std::shared_ptr<GeoSegmentsGeneratorBase> segments_generator,
      std::shared_ptr<ITilingScheme> tiling_scheme, uint32_t tile_level,
      uint32_t area_offset)
      : segments_generator_(std::move(segments_generator)),
        tiling_scheme_(std::move(tiling_scheme)),
        tile_level_(tile_level),
        area_offset_(area_offset),
        bresenham_line_generator_(InitNextGenerator()) {}

  boost::optional<TileKey> Next() {
    if (!bresenham_line_generator_) {
      return boost::none;
    }

    int x = 0, y = 0;
    while (bresenham_line_generator_->Next(x, y)) {
      const auto& sub_division_scheme = tiling_scheme_->GetSubdivisionScheme();
      WrapAround(x, y, sub_division_scheme.GetLevelSize(tile_level_));

      const auto tile = TileKey::FromRowColumnLevel(y, x, tile_level_);
      const auto quad_key = tile.ToQuadKey64();

      if (visited_tiles_hashes_.count(quad_key) != 0) {
        continue;
      }
      // Remove excess tiles from queue
      if (visited_tiles_hashes_queue_.size() >
          area_offset_ * area_offset_ * 5) {
        visited_tiles_hashes_.erase(visited_tiles_hashes_queue_.front());
        visited_tiles_hashes_queue_.pop_front();
      }

      visited_tiles_hashes_.insert(quad_key);
      return tile;
    }

    bresenham_line_generator_ = InitNextGenerator();

    return Next();
  }

 private:
  boost::optional<BresenhamLineGenerator> InitNextGenerator() const {
    return MakeBresenhamLineGenerator(segments_generator_->Next(), area_offset_,
                                      *tiling_scheme_, tile_level_);
  }

  static void WrapAround(int& x, int& y, const math::Size2u& dimensions) {
    if (x < 0) {
      x = x % dimensions.Width() + dimensions.Width();
    } else if (x >= dimensions.Width()) {
      x = x % dimensions.Width();
    }

    if (y < 0) {
      y = y % dimensions.Height() + dimensions.Height();
    } else if (y >= dimensions.Height()) {
      y = y % dimensions.Height();
    }
  }

  std::shared_ptr<GeoSegmentsGeneratorBase> segments_generator_;
  std::shared_ptr<ITilingScheme> tiling_scheme_;
  const uint32_t tile_level_;
  const uint32_t area_offset_;

  boost::optional<BresenhamLineGenerator> bresenham_line_generator_;

  std::unordered_set<uint64_t> visited_tiles_hashes_;
  std::deque<uint64_t> visited_tiles_hashes_queue_;
};

PathTilingGenerator::PathTilingGenerator(
    std::shared_ptr<GeoSegmentsGeneratorBase> segments_generator,
    std::shared_ptr<ITilingScheme> tiling_scheme, uint32_t tile_level,
    uint32_t area_offset)
    : impl_(std::make_shared<PathTilingGeneratorImpl>(
          std::move(segments_generator), std::move(tiling_scheme), tile_level,
          area_offset)) {}

boost::optional<TileKey> PathTilingGenerator::Next() { return impl_->Next(); }

}  // namespace geo
}  // namespace olp
