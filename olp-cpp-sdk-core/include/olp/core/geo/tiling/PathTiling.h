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

#pragma once

#include <memory>
#include <tuple>

#include <olp/core/geo/coordinates/GeoCoordinates.h>
#include <olp/core/geo/tiling/TileKey.h>

#include <boost/optional/optional.hpp>
#include <vector>

namespace olp {
namespace geo {

class GeoSegmentsGeneratorBase {
 public:
  using Segment = std::tuple<GeoCoordinates, GeoCoordinates>;

  virtual ~GeoSegmentsGeneratorBase() = default;

  virtual boost::optional<Segment> Next() = 0;
};

template <typename InputIterator>
class GeoSegmentsGenerator final : public GeoSegmentsGeneratorBase {
 public:
  GeoSegmentsGenerator(InputIterator begin, InputIterator end)
      : pos_(begin), end_(end) {}

  ~GeoSegmentsGenerator() override = default;

  boost::optional<Segment> Next() override {
    if (pos_ == end_) {
      return boost::none;
    }
    const auto start = static_cast<GeoCoordinates>(*pos_);
    std::advance(pos_, 1);
    if (pos_ == end_) {
      return boost::none;
    }
    const auto end = static_cast<GeoCoordinates>(*pos_);
    return std::make_tuple(start, end);
  }

 private:
  InputIterator pos_;
  InputIterator end_;
};

template <typename InputIterator>
std::shared_ptr<GeoSegmentsGeneratorBase> MakeGeoSegmentsGenerator(
    InputIterator begin, InputIterator end) {
  return std::make_shared<GeoSegmentsGenerator<InputIterator>>(begin, end);
}

class PathTilingGenerator {
 public:
  PathTilingGenerator(
      std::shared_ptr<GeoSegmentsGeneratorBase> segments_generator,
      std::shared_ptr<ITilingScheme> tiling_scheme, uint32_t tile_level,
      uint32_t area_offset);

  boost::optional<TileKey> Next();

 private:
  class PathTilingGeneratorImpl;
  std::shared_ptr<PathTilingGeneratorImpl> impl_;
};

}  // namespace geo
}  // namespace olp
