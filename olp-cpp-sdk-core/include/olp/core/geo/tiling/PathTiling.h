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
#include <utility>
#include <vector>

#include <olp/core/geo/coordinates/GeoCoordinates.h>
#include <olp/core/geo/tiling/TileKey.h>
#include <olp/core/geo/tiling/TileKeyUtils.h>
#include <boost/optional/optional.hpp>

namespace olp {
namespace geo {

namespace detail {

/**
 * @class LineEvaluator
 * @brief Implements Bresenham's line algorithm with a square sliding window.
 *
 * This class provides a way to iterate over a line while considering a
 * sliding window to enable line width.
 */
class LineEvaluator {
 public:
  /**
   * @struct State
   * @brief Holds the state of the line traversal.
   */
  struct State {
    int32_t x_end;
    bool is_slope_reversed;
    int32_t sliding_window_half_size;
    int32_t delta_x;
    int32_t delta_y;
    int32_t y_step;
    int32_t x;
    int32_t y;
    int32_t error;
    int32_t sliding_offset_x;
    int32_t sliding_offset_y;
    uint32_t tile_level;

    bool operator==(const State& other) const {
      return x_end == other.x_end &&
             is_slope_reversed == other.is_slope_reversed &&
             sliding_window_half_size == other.sliding_window_half_size &&
             delta_x == other.delta_x && delta_y == other.delta_y &&
             y_step == other.y_step && x == other.x && y == other.y &&
             error == other.error &&
             sliding_offset_x == other.sliding_offset_x &&
             sliding_offset_y == other.sliding_offset_y &&
             tile_level == other.tile_level;
    }
  };

  /**
   * @brief Evaluates the current tile key from the given state.
   *
   * @param state The current state of the line traversal.
   *
   * @return The tile key corresponding to the current position.
   */
  static TileKey Value(const State& state) {
    int tile_x = state.x + state.sliding_offset_x;
    int tile_y = state.y + state.sliding_offset_y;

    if (state.is_slope_reversed)
      std::swap(tile_x, tile_y);

    return TileKey::FromRowColumnLevel(tile_y, tile_x, state.tile_level);
  }

  /**
   * @brief Iterates the state towards the end of the line.
   *
   * @param state The current state of the line traversal.
   *
   * @return True if the iteration continues, false if it ends.
   */
  static bool Iterate(State& state) {  // NOLINT
    if (state.x > state.x_end) {
      return false;
    }

    if (++state.sliding_offset_y > state.sliding_window_half_size) {
      state.sliding_offset_y = -state.sliding_window_half_size;
      if (++state.sliding_offset_x > state.sliding_window_half_size) {
        state.sliding_offset_x = -state.sliding_window_half_size;

        state.error += state.delta_y;
        if (state.error * 2 >= state.delta_x) {
          state.y += state.y_step;
          state.error -= state.delta_x;
        }

        ++state.x;
      }
    }

    return true;
  }

  /**
   * @brief Initializes the line state between two tiles.
   *
   * @param start_tile The starting tile.
   * @param end_tile The ending tile.
   * @param sliding_window_half_size The half-size of the sliding window.
   *
   * @return The initialized line state.
   */
  static State Init(const TileKey start_tile, const TileKey end_tile,
                    const int32_t sliding_window_half_size) {
    int32_t x0 = static_cast<int32_t>(start_tile.Column());
    int32_t y0 = static_cast<int32_t>(start_tile.Row());
    int32_t x1 = static_cast<int32_t>(end_tile.Column());
    int32_t y1 = static_cast<int32_t>(end_tile.Row());

    const uint32_t tile_level = start_tile.Level();

    const bool should_reverse_slope = std::abs(y1 - y0) > std::abs(x1 - x0);

    if (should_reverse_slope) {
      std::swap(x0, y0);
      std::swap(x1, y1);
    }

    if (x0 > x1) {
      std::swap(x0, x1);
      std::swap(y0, y1);
    }

    const int32_t line_end = x1;
    const int32_t delta_x = x1 - x0;
    const int32_t delta_y = std::abs(y1 - y0);
    const int32_t y_step = y0 > y1 ? -1 : 1;
    const int32_t initial_x = x0;
    const int32_t initial_y = y0;

    return State{line_end,
                 should_reverse_slope,
                 sliding_window_half_size,
                 delta_x,
                 delta_y,
                 y_step,
                 initial_x,
                 initial_y,
                 0,
                 -sliding_window_half_size,
                 -sliding_window_half_size,
                 tile_level};
  }
};
}  // namespace detail

/**
 * @class TilingIterator
 * @brief Iterator for transforming input coordinates into TileKeys using a
 * TilingScheme.
 */
template <typename InputIterator, typename TilingScheme>
class TilingIterator {
 public:
  using value_type = TileKey;
  using difference_type = std::ptrdiff_t;
  using pointer = TileKey*;
  using reference = TileKey&;
  using iterator_category = std::forward_iterator_tag;

  /**
   * @brief Constructs a TilingIterator.
   *
   * @param iterator The input iterator.
   * @param tile_level The tile level (default: 0).
   */
  explicit TilingIterator(InputIterator iterator, const uint32_t tile_level = 0)
      : iterator_(iterator), tiling_scheme_(), tile_level_(tile_level) {}

  /**
   * @brief Dereference operator.
   *
   * @return The TileKey corresponding to the current iterator position.
   */
  value_type operator*() const {
    return TileKeyUtils::GeoCoordinatesToTileKey(
        tiling_scheme_, static_cast<GeoCoordinates>(*iterator_), tile_level_);
  }

  TilingIterator& operator++() {
    ++iterator_;
    return *this;
  }

  bool operator==(const TilingIterator& other) const {
    return iterator_ == other.iterator_;
  }

  bool operator!=(const TilingIterator& other) const {
    return !(*this == other);
  }

 private:
  InputIterator iterator_;
  TilingScheme tiling_scheme_;
  uint32_t tile_level_;
};

/**
 * @brief Helper function to create a TilingIterator.
 * @param iterator The input iterator.
 * @param tile_level The tile level.
 * @return A new TilingIterator instance.
 */
template <typename TilingScheme, typename InputIterator>
auto MakeTilingIterator(InputIterator iterator, uint32_t tile_level = 0)
    -> TilingIterator<InputIterator, TilingScheme> {
  return TilingIterator<InputIterator, TilingScheme>(iterator, tile_level);
}

/**
 * @class AdjacentPairIterator
 * @brief Iterator for iterating over adjacent pairs in a sequence.
 */
template <typename InputIterator>
class AdjacentPairIterator {
 public:
  using value_type = std::pair<typename InputIterator::value_type,
                               typename InputIterator::value_type>;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type*;
  using reference = value_type&;
  using iterator_category = std::forward_iterator_tag;

  /**
   * @brief Constructs an AdjacentPairIterator.
   *
   * @param segment_it The input iterator.
   */
  explicit AdjacentPairIterator(InputIterator segment_it)
      : current_value_(), next_it_(segment_it) {}

  /**
   * @brief Constructs an AdjacentPairIterator with an initial value.
   *
   * @param initial_value The initial value.
   * @param segment_it The input iterator.
   */
  AdjacentPairIterator(typename InputIterator::value_type initial_value,
                       InputIterator segment_it)
      : current_value_(initial_value), next_it_(segment_it) {}

  value_type operator*() const {
    return std::make_pair(current_value_, *next_it_);
  }

  AdjacentPairIterator& operator++() {
    current_value_ = *next_it_;
    ++next_it_;
    return *this;
  }

  bool operator==(const AdjacentPairIterator& other) const {
    return next_it_ == other.next_it_;
  }

  bool operator!=(const AdjacentPairIterator& other) const {
    return !(*this == other);
  }

 private:
  typename InputIterator::value_type current_value_;
  InputIterator next_it_;
};

/*
 * @brief Creates an AdjacentPairIterator from a given iterator.
 *
 * @param iterator The input iterator.
 *
 * @return An AdjacentPairIterator initialized with the given iterator.
 */
template <typename InputIterator>
auto MakeAdjacentPairIterator(InputIterator iterator)
    -> AdjacentPairIterator<InputIterator> {
  return AdjacentPairIterator<InputIterator>(iterator);
}

/*
 * @brief Creates an AdjacentPairIterator from a given iterator.
 *
 * @param initial_value The initial value for the adjacent pair.
 * @param iterator The input iterator.
 *
 * @return An AdjacentPairIterator initialized with the given iterator.
 */
template <typename InputIterator,
          typename ValueType = typename InputIterator::value_type>
auto MakeAdjacentPairIterator(ValueType initial_value, InputIterator iterator)
    -> AdjacentPairIterator<InputIterator> {
  return AdjacentPairIterator<InputIterator>(initial_value, iterator);
}

/*
 * @brief Creates a range of AdjacentPairIterators from the given begin and end
 * iterators.
 *
 * @param begin The beginning of the input range.
 * @param end The end of the input range.
 * @return A pair of AdjacentPairIterators representing the range.
 */
template <typename InputIterator,
          typename IteratorType = AdjacentPairIterator<InputIterator>>
auto MakeAdjacentPairsRange(InputIterator begin, InputIterator end)
    -> std::pair<IteratorType, IteratorType> {
  if (begin == end) {
    return std::make_pair(MakeAdjacentPairIterator(begin),
                          MakeAdjacentPairIterator(end));
  }
  return std::make_pair(MakeAdjacentPairIterator(*begin, std::next(begin)),
                        MakeAdjacentPairIterator(end));
}

/**
 * @brief An iterator that slices a line into tile segments.
 *
 * This iterator takes an input iterator of adjacent tile segments and
 * applies a line slicing algorithm to generate individual tile keys
 * along the path.
 */
template <typename InputIterator>
class LineSliceIterator {
 public:
  using value_type = typename InputIterator::value_type;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type*;
  using reference = value_type&;
  using iterator_category = std::forward_iterator_tag;

  /**
   * @brief Constructs a LineSliceIterator.
   *
   * @param segment_it The iterator over tile segments.
   * @param line_width The width of the line in tiles.
   */
  explicit LineSliceIterator(InputIterator segment_it,
                             const uint32_t line_width)
      : segment_it_(segment_it),
        half_line_width_(static_cast<int32_t>(line_width) / 2) {}

  TileKey operator*() {
    if (!line_state_) {
      TileKey begin, end;
      std::tie(begin, end) = *segment_it_;
      line_state_ = detail::LineEvaluator::Init(begin, end, half_line_width_);
    }
    return detail::LineEvaluator::Value(*line_state_);
  }

  LineSliceIterator& operator++() {
    if (!line_state_ || !detail::LineEvaluator::Iterate(*line_state_)) {
      line_state_ = boost::none;
      ++segment_it_;
    }
    return *this;
  }

  bool operator==(const LineSliceIterator& other) const {
    return segment_it_ == other.segment_it_ &&
           half_line_width_ == other.half_line_width_ &&
           line_state_ == other.line_state_;
  }

  bool operator!=(const LineSliceIterator& other) const {
    return !(*this == other);
  }

 private:
  InputIterator segment_it_;
  int32_t half_line_width_;
  boost::optional<detail::LineEvaluator::State> line_state_;
};

/**
 * @brief Creates a LineSliceIterator from an input iterator.
 *
 * @param iterator The input iterator over tile segments.
 * @param line_width The width of the line in tiles.
 * @return A LineSliceIterator initialized with the given parameters.
 */
template <typename InputIterator>
auto MakeLineSliceIterator(InputIterator iterator, const uint32_t line_width)
    -> LineSliceIterator<InputIterator> {
  return LineSliceIterator<InputIterator>(iterator, line_width);
}

/**
 * @brief Defines an iterator type that slices a tiled path into line segments.
 *
 * @tparam InputIterator The type of input iterator that iterates over
 * geo-coordinates.
 * @tparam TilingScheme The tiling scheme used to map geo-coordinates to tiles.
 */
template <typename InputIterator, typename TilingScheme>
using TiledPathIterator = LineSliceIterator<
    AdjacentPairIterator<TilingIterator<InputIterator, TilingScheme>>>;

/**
 * @brief Creates an iterator range for traversing a tiled path with a specified
 * width.
 *
 * @note The result range has no ownership over the input range.
 *
 * This function constructs an iterator range that slices a path into tiles,
 * using a tiling scheme and a specified path width.
 *
 * @tparam TilingScheme The tiling scheme used to map geo-coordinates to tiles.
 * @tparam InputIterator The type of input iterator that iterates over
 * geo-coordinates.
 *
 * @param begin The beginning iterator of the input range.
 * @param end The ending iterator of the input range.
 * @param level The tile level to be used for tiling.
 * @param path_width The width of the path in tiles.
 *
 * @return A pair of iterators defining the range for traversing the tiled path.
 */
template <typename TilingScheme, typename InputIterator>
auto MakeTiledPathRange(InputIterator begin, InputIterator end,
                        const uint32_t level, const uint32_t path_width)
    -> std::pair<TiledPathIterator<InputIterator, TilingScheme>,
                 TiledPathIterator<InputIterator, TilingScheme>> {
  auto adjacent_pairs_range =
      MakeAdjacentPairsRange(MakeTilingIterator<TilingScheme>(begin, level),
                             MakeTilingIterator<TilingScheme>(end));
  return {MakeLineSliceIterator(adjacent_pairs_range.first, path_width),
          MakeLineSliceIterator(adjacent_pairs_range.second, path_width)};
}

}  // namespace geo
}  // namespace olp
