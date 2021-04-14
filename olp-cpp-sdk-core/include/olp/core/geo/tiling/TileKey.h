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

#pragma once

#include <bitset>
#include <string>

#include <boost/optional.hpp>

#include <olp/core/CoreApi.h>

namespace olp {
namespace geo {

/**
 * @brief Addresses a tile in a quadtree.
 *
 * Each tile key is defined by a row, a column, and a level.
 * The tree has a root at level 0 with one single tile. At every level,
 * each tile is divided into four child tiles (hence the name quadtree).
 *
 * Within a level, each tile has its unique row and column numbers.
 * The number of rows and columns in each level is 2 to the power of the level.
 * It means that on level 0, there is only one tile in
 * one row and one column. At level 1, there are four tiles
 * in two rows and two columns. At level 2, there are
 * 16 tiles in four rows and four columns. And so on.
 *
 * To create a tile key, use `FromRowColumnLevel()`.
 *
 * For vertical navigation within the tree, use the following functions:
 * `Parent()`, `ChangedLevelBy()`, and `ChangedLevelTo()`. To navigate within a level, use
 * the `HasNextRow()`, `NextRow()`, `HasNextColumn()`, and `NextColumn()` functions.
 * To get the number of available rows and columns on the tile level, use `RowCount()` and
 * `ColumnCount()`.
 *
 * You can also create tile keys from and converted them into various alternative formats:
 *
 *  - `ToQuadKey()` / `FromQuadKey()` – 4-based string representation.
 *
 *  - `ToHereTile()` / `FromHereTile()` – 10-based string representation.
 *
 *  - `ToQuadKey64()` / `FromQuadKey64()` – 64-bit Morton code representation.
 */
class CORE_API TileKey {
 public:
  enum { LevelCount = 32 };
  enum { MaxLevel = LevelCount - 1 };

  /**
   * @brief The main direction used to find a child node or 
   * the relationship to the parent.
   *
   * Corresponds directly to the index in the `GetChild` function.
   */
  enum class TileKeyQuadrant : uint8_t { SW, SE, NW, NE, Invalid };

  /// Creates an invalid tile key.
  constexpr TileKey() : row_(0), column_(0), level_(LevelCount) {}

  /// The default copy constructor.
  constexpr TileKey(const TileKey&) = default;

  /// The default copy operator.
  TileKey& operator=(const TileKey&) = default;

  /// Checks whether the tile key is valid.
  constexpr bool IsValid() const {
    return level_ < LevelCount && row_ < (1u << level_) &&
           column_ < (1u << level_);
  }

  /**
   * @brief Checks whether the tile keys are equal.
   *
   * @param other The other tile key.
   */
  constexpr bool operator==(const TileKey& other) const {
    return level_ == other.level_ && row_ == other.row_ &&
           column_ == other.column_;
  }

  /**
   * @brief Checks whether the tile keys are not equal.
   *
   * @param other The other tile key.
   */
  constexpr bool operator!=(const TileKey& other) const {
    return !operator==(other);
  }

  /**
   * @brief Implements the following order on tile keys so they
   * can be used in maps: first level, row, and then column.
   *
   * If you need more locality, use the 64-bit Morton encoding
   * instead (`ToQuadKey64()`).
   */
  constexpr bool operator<(const TileKey& other) const {
    return level_ != other.level_
               ? level_ < other.level_
               : row_ != other.row_ ? row_ < other.row_
                                    : column_ < other.column_;
  }

  /**
   * @brief Creates a quad string from a tile key to later use it in REST API calls.
   *
   * If the tile is the root tile, the quadkey is '-'.
   * Otherwise, the string is a number to the base of 4 without the leading
   * 1 and has the following properties:
   *  - The number of digits is equal to the level.
   *  - Removing the last digit gives the parent tile's quadkey string.
   * For example, to get the child tile, add 0, 1, 2, or 3 to a quadkey string.
   *
   * To convert the quadkey string back into the tile key, use `FromQuadKey()`.
   */
  std::string ToQuadKey() const;

  /**
   * @brief Creates a tile key from a quad string.
   *
   * To convert the tile key back into the quad string, use `ToQuadKey()`.
   */
  static TileKey FromQuadKey(const std::string& quad_key);

  /**
   * @brief Creates a HERE tile code string from a tile key to later use it in REST API calls.
   *
   * The string is a quadkey Morton code.
   *
   * To convert the HERE tile code string back into the tile key, use `FromHereTile()`.
   */
  std::string ToHereTile() const;

  /**
   * @brief Creates a tile key from a HERE tile code string.
   *
   * To convert the tile key back into the string, use `ToHereTile()`.
   */
  static TileKey FromHereTile(const std::string& key);

  /**
   * @brief Creates a 64-bit Morton code from a tile key.
   *
   * To convert the 64-bit Morton code back into the tile key, use `FromQuadKey64()`.
   */
  std::uint64_t ToQuadKey64() const;

  /**
   * @brief Creates a tile key from a 64-bit Morton code.
   *
   * To convert a tile key back into a 64-bit Morton code, use `ToQuadKey64()`.
   */
  static TileKey FromQuadKey64(std::uint64_t quad_key);

  /**
   * @brief Creates a tile key.
   *
   * @param row The requested row. Must be less than 2 to the power of the level.
   * @param column The requested column. Must be less than 2 to the power of
   * the level.
   * @param level The requested level.
   */
  static TileKey FromRowColumnLevel(std::uint32_t row, std::uint32_t column,
                                    std::uint32_t level);

  /**
   * @brief Gets the tile level.
   *
   * @return The tile level.
   */
  constexpr std::uint32_t Level() const { return level_; }

  /**
   * @brief Gets the tile row.
   *
   * To get the number of rows at a level, use `RowCount()`.
   *
   * @return The tile row.
   */
  constexpr std::uint32_t Row() const { return row_; }

  /**
   * @brief Gets the number of available rows at the tile level.
   *
   * It is 2 to the power of the level.
   *
   * @return The number of available rows.
   */
  constexpr std::uint32_t RowCount() const { return 1 << level_; }

  /**
   * @brief Gets the tile column.
   *
   * To get the number of available columns on a
   * tile level, use `ColumnCount()`.
   *
   * @return The tile column.
   */
  constexpr std::uint32_t Column() const { return column_; }

  /**
   * @brief Gets the number of available columns at the tile level.
   *
   * It is 2 to the power of the level.
   *
   * @returns The number of available columns.
   */
  constexpr std::uint32_t ColumnCount() const { return 1 << level_; }

  /**
   * @brief Gets the key of the parent tile.
   *
   * If the tile is the root tile, an invalid tile key is returned.
   *
   * @return The key of the parent tile.
   */
  TileKey Parent() const;

  /**
   * @brief Checks whether the current tile is a child of another tile.
   *
   * @param[in] tile_key The key of the possible parent tile.
   *
   * @return A value indicating whether the current tile is a child of
   * the specified tile.
   */
  bool IsChildOf(const TileKey& tile_key) const;

  /**
   * @brief Checks whether the current tile is a parent of another tile.
   *
   * @param[in] tile_key The key of the child tile.
   *
   * @return A value indicating whether the current tile is a parent of
   * the specified tile.
   */
  bool IsParentOf(const TileKey& tile_key) const;

  /**
   * @brief Gets a new tile key at a level that differs from this tile
   * level by delta.
   *
   * Equivalent to `changedLevelTo(Level() + delta)`
   *
   * @param delta The numeric difference between the current level and
   * the requested level.
   *
   * @return The tile key at the level that differs from this tile
   * level by delta.
   */
  TileKey ChangedLevelBy(int delta) const;

  /**
   * @brief Gets a new tile key at the requested level.
   *
   * @param level The requested level.
   *
   * @return If the requested level is smaller than the tile level,
   * the key of the tile ancestor is returned.
   * If the requested level is larger than the tile level,
   * the returned value is the key of the first child or grandchild,
   * which is the child with the lowest row and column number.
   * If the requested level equals the level of this tile, the tile key itself
   * is returned.
   */
  TileKey ChangedLevelTo(std::uint32_t level) const;

  /**
   * @copydoc QuadKey64Helper::GetSubkey()
   */
  std::uint64_t GetSubkey64(int delta) const;

  /// @copydoc TileKey::AddedSubkey()
  TileKey AddedSubkey64(std::uint64_t sub_quad_key) const;

  /**
   * @brief Gets the absolute quadkey that is constructed from its subquadkey.
   *
   * It is the reverse function of `GetSubkey()`. It returns the absolute
   * quadkey in the tree based on the subquadkey.
   *
   * @param sub_quad_key The subquadkey generated with `ToSubQuadKey()`.
   *
   * @return The absolute quadkey in the quadtree.
   */
  TileKey AddedSubkey(const std::string& sub_quad_key) const;

  /**
   * @brief Gets the absolute quadkey that is constructed
   * from its HERE child tile key.
   *
   * @param sub_here_tile The HERE child tile key.
   *
   * @return The absolute tile key in the quadtree.
   */
  TileKey AddedSubHereTile(const std::string& sub_here_tile) const;

  /**
   * @brief Checks whether there is the next row at this level.
   *
   * @return True if there is the next row; false otherwise.
   */
  constexpr bool HasNextRow() const { return row_ < (1u << level_) - 1; }

  /**
   * @brief Gets the key of the tile that has the same level
   * and column numbers but belongs to the next row.
   *
   * @return The tile key with the row number increased by one.
   */
  TileKey NextRow() const;

  /**
   * @brief Checks whether there is the next column at this level.
   *
   * @return True if there is the next column; false otherwise.
   */
  constexpr bool HasNextColumn() const { return column_ < (1u << level_) - 1; }

  /**
   * @brief Gets the key of the tile that has the same level
   * and row numbers but belongs to the next column.
   *
   * @return The tile key with the column number increased by one.
   */
  TileKey NextColumn() const;

  /**
   * @brief Checks whether there is the previous row at this level.
   *
   * @return True if there is the previous row; false otherwise.
   */
  constexpr bool HasPreviousRow() const { return (row_ > 0); }

  /**
   * @brief Gets the key of the tile that has the same level
   * and column numbers but belongs to the previous row.
   *
   * @return The tile key with the row number decreased by one.
   */
  TileKey PreviousRow() const;

  /**
   * @brief Checks whether there is the previous column at this level.
   *
   * @return True if there is the previous column; false otherwise.
   */
  constexpr bool HasPreviousColumn() const { return (column_ > 0); }

  /**
   * @brief Gets the key of the tile that has the same level
   * and row numbers but belongs to the previous column.
   *
   * @return The tile key with the column number decreased by one.
   */
  TileKey PreviousColumn() const;

  /**
   * @brief Gets the child of the current tile by its index.
   *
   * @param index The child index from the [0, 3] range.
   *
   * @return The child of the current child.
   */
  TileKey GetChild(std::uint8_t index) const;

  /**
   * @brief Gets the child of the current tile using a direction.
   *
   * @param direction The direction to get the child tile.
   *
   * @return The child of the current tile.
   */
  TileKey GetChild(TileKeyQuadrant direction) const;

  /**
   * @brief Computes the direction of the relationship to the parent.
   *
   * @return `Invalid` if the tile key represents the root.
   */
  TileKeyQuadrant RelationshipToParent() const;

 private:
  std::uint32_t row_{0};
  std::uint32_t column_{0};
  std::uint32_t level_{LevelCount};
};

/**
 * @brief A helper structure for basic operations on 64-bit Morton quadkeys.
 *
 * This class can be used to prevent conversions between tile keys and quadkeys
 * for basic operations.
 */
struct CORE_API QuadKey64Helper {
  /**
   * @brief The default constructor.
   */
  explicit constexpr QuadKey64Helper(std::uint64_t key) : key(key) {}

  /**
   * @brief Gets the quadkey of the parent.
   *
   * @return The parent quadkey.
   */
  constexpr QuadKey64Helper Parent() const { return QuadKey64Helper{key >> 2}; }

  /**
   * @brief Gets the quadkey representing the first child of this quad.
   *
   * @return The quadkey of the first child.
   */
  constexpr QuadKey64Helper Child() const { return QuadKey64Helper{key << 2}; }

  /**
   * @brief Gets a subquadkey that is a relative of its parent.
   *
   * Use this function to generate subkeys that are relatives of
   * a parent that is delta levels up in the subtree.
   *
   * You can also use this function to create shortened keys for quads
   * on lower levels if the parent is known.
   *
   * @note The subquadkeys fit in a 16-bit unsigned integer if the delta is
   * smaller than 8. If the delta is smaller than 16, the subquadkey fits into
   * a 32-bit unsigned integer.
   *
   * @param delta The number of levels that are relatives of the parent quadkey.
   * Must be greater or equal to 0 and smaller than 32.
   *
   * @return The quadkey relative of its parent that is delta levels up the
   * tree.
   */
  QuadKey64Helper GetSubkey(int delta) const;

  /**
   * @brief Gets the absolute quadkey that is constructed from its subquadkey.
   *
   * It is the reverse function of `GetSubkey()`. It returns the absolute quad
   * key in the tree based on a subquadkey.
   *
   * @param sub_key The subquadkey generated with `ToSubQuadKey()`.
   *
   * @return The absolute quadkey in the quadtree.
   */
  QuadKey64Helper AddedSubkey(QuadKey64Helper sub_key) const;

  /**
   * @brief Gets the number of rows at a given level.
   *
   * It is 2 to the power of the level.
   *
   * @param level The requested level.
   *
   * @return The number of rows at the level.
   */
  static inline constexpr std::uint32_t RowsAtLevel(std::uint32_t level) {
    return 1u << level;
  }

  /**
   * @brief Gets the number of children at a level.
   *
   * It is 4 to the power of the level.
   *
   * @param level The requested level.
   *
   * @return The number of children at the level.
   */
  static inline constexpr std::uint32_t ChildrenAtLevel(std::uint32_t level) {
    return 1u << (level << 1u);
  }

  /// The representation of this quadkey.
  std::uint64_t key{0};
};

using TileKeyLevels = std::bitset<TileKey::LevelCount>;

/**
 * @brief Gets the minimum level in a level set.
 *
 * @param levels The set of levels.
 *
 * @return The minimum level or `core::None` if the set is empty.
 */
CORE_API boost::optional<std::uint32_t> GetMinTileKeyLevel(
    const TileKeyLevels& levels);

/**
 * @brief Gets the maximum level in a level set.
 *
 * @param levels The set of levels.
 *
 * @return The maximum level or `core::None` if the set is empty.
 */
CORE_API boost::optional<std::uint32_t> GetMaxTileKeyLevel(
    const TileKeyLevels& levels);

/**
 * @brief Gets the tile level of a level set that is nearest to a
 * reference level.
 *
 * @param levels The set of levels.
 * @param reference_level The reference level.
 *
 * @return The nearest level or `core::None` if the set is empty.
 */
CORE_API boost::optional<std::uint32_t> GetNearestAvailableTileKeyLevel(
    const TileKeyLevels& levels, const std::uint32_t reference_level);

/**
 * @brief The stream operator to print or serialize the given tile key.
 */
CORE_API std::ostream& operator<<(std::ostream& out,
                                  const geo::TileKey& tile_key);

}  // namespace geo
}  // namespace olp

namespace std {

///@brief The specialization of `std::hash`.
template <>
struct hash<olp::geo::TileKey> {
  /**
   * @brief The hash function for tile keys.
   *
   * Uses the 64-bit Morton code (`TileKey::ToQuadKey64()`).
   */
  std::size_t operator()(const olp::geo::TileKey& tile_key) const {
    return std::hash<std::uint64_t>()(tile_key.ToQuadKey64());
  }
};

}  // namespace std
