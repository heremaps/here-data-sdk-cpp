/*
 * Copyright (C) 2019 HERE Europe B.V.
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
#include <cstdint>
#include <functional>
#include <ostream>
#include <string>

#include <olp/core/CoreApi.h>
#include <boost/optional.hpp>

namespace olp {
namespace geo {
/**
 * @brief The TileKey instances used to address a tile in a quad tree.
 *
 * A tile key is defined by a row, a column, and a level.
 * The tree has a root at level 0, with one single tile. On every level,
 * each tile is divided into four children (ergo the name quad tree).
 *
 * Within each level(), any particular tile is addressed with Row() and
 * Column().
 * The number of rows and columns in each level is 2 to the power of the level.
 * This means: On level 0, only one tile exists, ColumnCount() and RowCount()
 * are both 1. On level 1, 4 tiles exist, in 2 rows and 2 columns. On level 2 we
 * have 16 tiles,in 4 rows and 4 columns. And so on.
 *
 * A tile key is usually created FromRowColumnLevel().

 * Utility functions like Parent(), ChangedLevelBy(), and ChangedLevelTo() allow
 * for easy vertical navigation of the tree. Within a level you can navigate
 * with HasNextRow(),NextRow(), HasNextColumn(), and NextColumn(). The number of
 * available rows and columns in the tile's level is given with RowCount() and
 * ColumnCount().
 *
 * Tile keys can be created from and converted into various alternative formats:
 *
 *  - ToQuadKey() / FromQuadKey() - string representation 4-based
 *
 *  - ToHereTile() / FromHereTile() - string representation 10-based
 *
 *  - ToQuadKey64() / FromQuadKey64() - 64-bit morton code representation.
 *
 */
class CORE_API TileKey {
 public:
  enum { LevelCount = 32 };
  enum { MaxLevel = LevelCount - 1 };

  /**
   * @brief Creates an invalid tile key.
   */
  constexpr TileKey() : row_(), column_(), level_(LevelCount) {}

  /**
   * @brief Default copy constructor.
   */
  constexpr TileKey(const TileKey&) = default;

  /**
   * @brief Default copy operator.
   */
  TileKey& operator=(const TileKey&) = default;

  /**
   * @brief Check whether the tile key is valid.
   */
  constexpr bool IsValid() const {
    return level_ < LevelCount && row_ < (1u << level_) &&
           column_ < (1u << level_);
  }

  /**
   * Operator ==.
   * \param other TileKey to be compared to this.
   */
  constexpr bool operator==(const TileKey& other) const {
    return level_ == other.level_ && row_ == other.row_ &&
           column_ == other.column_;
  }

  /**
   * Operator !=.
   * \param other TileKey to be compared to this.
   */
  constexpr bool operator!=(const TileKey& other) const {
    return !operator==(other);
  }

  /**
   * Implements an order on tile keys, so they can be used in maps:
   * first level, then row, then column.
   *
   * If you need more locality, you should use the 64bit morton encoding
   * instead (ToQuadKey64()).
   */
  constexpr bool operator<(const TileKey& other) const {
    return level_ != other.level_
               ? level_ < other.level_
               : row_ != other.row_ ? row_ < other.row_
                                    : column_ < other.column_;
  }

  /**
   * @brief Converts the tile key into a string for using in REST API calls
   *
   * If the tile is the root tile, the quad key is '-'.
   * Otherwise the string is a number to the base of 4, but without the leading
   * 1, with the following properties:
   *  1. the number of digits equals the level
   *  2. removing the last digit gives the parent tile's quad key string, i.e.
   *     appending 0,1,2,3 to a quad key string gives the tile's children
   *
   * You can convert back from a quad key string with FromQuadKey().
   */
  std::string ToQuadKey() const;

  /**
   * @brief Creates a tile key from a quadstring.
   *
   * The quadstring can be created with ToQuadKey().
   */
  static TileKey FromQuadKey(const std::string& quad_key);

  /**
   * @brief Converts the tile key into a string for using in REST API calls
   *
   * The string is quad key morton code representation as a string.
   *
   * You can convert back from a quad key string with FromHereTile().
   */
  std::string ToHereTile() const;

  /**
   * @brief Creates a tile key from a heretile codes string.
   *
   * The string can be created with ToHereTile().
   */
  static TileKey FromHereTile(const std::string& key);

  /**
   * @brief Converts the tile key to a 64-bit morton code representation.
   *
   * You can create a tile key from a 64-bit morton code with FromQuadKey64().
   */
  std::uint64_t ToQuadKey64() const;

  /**
   * @brief Creates a tile key from a 64-bit morton code representation.
   *
   * You can convert a tile key into 64-bit morton code with ToQuadKey64().
   */
  static TileKey FromQuadKey64(std::uint64_t quad_key);

  /**
   * @brief Creates a tile key.
   *
   * @param row the requested row. Must be less than 2 to the power of level.
   * @param column the requested column. Must be less than 2 to the power of
   * level.
   * @param level the requested level.
   */
  static TileKey FromRowColumnLevel(std::uint32_t row, std::uint32_t column,
                                    std::uint32_t level);

  /**
   * @brief Returns the tile's level.
   */
  constexpr std::uint32_t Level() const { return level_; }

  /**
   * @brief Returns the tile's row. The number of available rows in the tile's
   * level() is returned by RowCount().
   */
  constexpr std::uint32_t Row() const { return row_; }

  /**
   * @brief Returns the number of available rows in the tile's Level().
   *
   * This is 2 to the power of the level.
   */
  constexpr std::uint32_t RowCount() const { return 1 << level_; }

  /**
   * @brief Returns the tile's column. The number of available columns in the
   * tile's Level() is returned by ColumnCount().
   */
  constexpr std::uint32_t Column() const { return column_; }

  /**
   * @brief Returns the number of available columns in the tile's Level().
   *
   * This is 2 to the power of the level.
   */
  constexpr std::uint32_t ColumnCount() const { return 1 << level_; }

  /**
   * @brief Returns a tile key representing the paren of the tile addressed by
   * this tile key
   *
   * If the tile was the root tile, the invalid tile key is returned.
   */
  TileKey Parent() const;

  /**
   * @brief Checks the current tile is a child of another tile.
   * @param[in] tile_key tile key of a possible parent.
   * @return a value indicating whether the current tile is a child of the
   * specified tile.
   */
  bool IsChildOf(const TileKey& tile_key) const;

  /**
   * @brief Checks the current tile is a parent of another tile.
   * @param[in] tile_key tile key of a possible child.
   * @return a value indicating whether the current tile is a parent of the
   * specified tile.
   */
  bool IsParentOf(const TileKey& tile_key) const;

  /**
   *
   * @brief Returns a new tile key at a level that differs from this tile's
   * level by delta.
   *
   * Equivalent to changedLevelTo(Level() + delta)
   *
   * @param delta the numeric difference between the current level and the
   * requested level
   */
  TileKey ChangedLevelBy(int delta) const;

  /**
   * @brief Returns a new tile key at the requested level.
   *
   * If the requested level is smaller then the tile's level, then
   * the key of an ancestor of this tile is returned.
   * If the requested level is larger then the tile's level, then
   * the key of first child or grandchild of this tile is returned, i.e. the
   * child with the lowest row and column number.
   * If the requested level equals this tile's level, then the tile key itself
   * is returned.
   *
   * @param level the requested level
   */
  TileKey ChangedLevelTo(std::uint32_t level) const;

  /**
   * @brief see QuadKey64Helper::GetSubkey()
   * @param delta see Quad64::GetSubkey()
   * @return The subquad key
   */
  std::uint64_t GetSubkey64(int delta) const;

  /**
   * @brief see QuadKey64Helper::AddedSubkey()
   * @param sub_quad_key the sub quad key to apply
   * @return The resulting quad
   */
  TileKey AddedSubkey64(std::uint64_t sub_quad_key) const;

  /**
   * @brief see QuadKey64Helper::AddedSubkey()
   * @param sub_quad_key the sub quad key to apply
   * @return The resulting tile key
   */
  TileKey AddedSubkey(const std::string& sub_quad_key) const;

  /**
   * @brief see QuadKey64Helper::AddedSubkey()
   * @param sub_here_tile the sub quad here tile key to apply
   * @return The resulting tile key
   */
  TileKey AddedSubHereTile(const std::string& sub_here_tile) const;

  /**
   * @brief Returns true if there is a NextRow() in this level; otherwise
   * returns false.
   */
  constexpr bool HasNextRow() const { return row_ < (1u << level_) - 1; }

  /**
   * @brief Returns a tile key with identical Level() and Column(), but Row()
   * plus one.
   */
  TileKey NextRow() const;

  /**
   * @brief Returns true if there is a NextColumn() in this level; otherwise
   * returns false.
   */
  constexpr bool HasNextColumn() const { return column_ < (1u << level_) - 1; }

  /**
   * @brief Returns a tile key with identical Level() and Row(), but Column()
   * plus one.
   */
  TileKey NextColumn() const;

  /**
   * @brief Returns true if there is a PreviousRow() in this level; otherwise
   * returns false.
   */
  constexpr bool HasPreviousRow() const { return (row_ > 0); }

  /**
   * @brief Returns a tile key with identical Level() and Column(), but Row()
   * minus one.
   */
  TileKey PreviousRow() const;

  /**
   * @brief Returns true if there is a PreviousColumn() in this level; otherwise
   * returns false.
   */
  constexpr bool HasPreviousColumn() const { return (column_ > 0); }

  /**
   * @brief Returns a tile key with identical Level() and Row(), but Column()
   * minus one.
   */
  TileKey PreviousColumn() const;

  /**
   * @brief Returns n-th child of the current tile.
   * @param index the child index from [0, 3] range
   */
  TileKey GetChild(std::uint8_t index) const;

  /**
   * Cardinal direction, used to find child node by such direction or for the
   * relationship to the parent, corresponds directly to the idx in the function
   * GetChild.
   */
  enum class TileKeyQuadrant : uint8_t { SW, SE, NW, NE, Invalid };

  /**
   * @brief Returns child of the current tile by direction.
   * @param direction which child to return
   */
  TileKey GetChild(TileKeyQuadrant direction) const;

  /**
   * Computes the direction in relationship to the parent, returns Invalid if
   * the TileKey represents the root
   */
  TileKeyQuadrant RelationshipToParent() const;

 private:
  std::uint32_t row_{0};
  std::uint32_t column_{0};
  std::uint32_t level_{LevelCount};
};

/**
 * @brief The QuadKey64Helper struct is a helper structure for basic operations
 * on 64 bit morton quad keys.
 *
 * This class can be used to prevent conversions between tile keys and quad keys
 * for basic operations.
 */
struct CORE_API QuadKey64Helper {
  /**
   * @brief value/default constructor
   */
  explicit constexpr QuadKey64Helper(std::uint64_t key)
    : key(key) {}

  /**
   * @brief key the representation of this quad key
   */
  std::uint64_t key{0};

  /**
   * @brief returns the quad key representing this quad key's parent
   * @return the parent
   */
  constexpr QuadKey64Helper Parent() const { return QuadKey64Helper{key >> 2}; }

  /**
   * @brief returns the quad key representing this quad's first child
   * @return the child
   */
  constexpr QuadKey64Helper Child() const { return QuadKey64Helper{key << 2}; }

  /**
   * @brief Returns a sub quad key that is relative to its parent
   *
   * This function can be used to generate sub keys that are relative to a
   * parent that is delta levels up in the sub tree.
   *
   * This function can be used to create shortened keys for quads on lower
   * levels if the parent is known.
   *
   * Note - the sub quad keys fit in a 16 bit unsigned integer if the delta is
   * smaller than 8. If delta is smaller than 16, the sub quad key fits into an
   * unsigned 32 bit integer.
   *
   * @param delta the amount of levels relative to its parent quad key. Must be
   *              greater or equal to 0 and smaller than 32.
   * @return the quad key relative to its parent that is delta levels up the
   * tree
   */
  QuadKey64Helper GetSubkey(int delta) const;

  /**
   * @brief Returns a quad key that is constructed from its subkey
   *
   * This function is the reverse of GetSubkey(). It returns the absolute quad
   * key in the tree based on a subKey.
   *
   * @param sub_key the subKey that was generated with ToSubQuadKey()
   * @return the absolute quad key in the quad tree
   */
  QuadKey64Helper AddedSubkey(QuadKey64Helper sub_key) const;

  /** @brief Returns the rows at a given level
   * This is 2 to the power of the level.
   *
   * @param level the level
   * @return Amount of rows on the level
   */
  static inline constexpr std::uint32_t RowsAtLevel(std::uint32_t level) {
    return 1u << level;
  }

  /** @brief Returns the amount of children at the given level
   * This is 4 to the power of the level.
   *
   * @param level the level
   * @return Amount of children on the level
   */
  static inline constexpr std::uint32_t ChildrenAtLevel(std::uint32_t level) {
    return 1u << (level << 1u);
  }
};

using TileKeyLevels = std::bitset<TileKey::LevelCount>;

/**
 * @brief Return the minimum level in the given level set.
 * @param levels
 * @return The minimum level or core::None if the set is empty.
 */
CORE_API boost::optional<std::uint32_t> GetMinTileKeyLevel(
    const TileKeyLevels& levels);

/**
 * @brief Return the maximum level in the given level set.
 * @param levels
 * @return The maximum level or core::None if the set is empty.
 */
CORE_API boost::optional<std::uint32_t> GetMaxTileKeyLevel(
    const TileKeyLevels& levels);

/**
 * @brief Return the tile level of the given level set that is nearest to a
 * given reference level. If distance is equal to the next upper and lower
 * level, then the upper level is returned.
 * @return The nearest level or core::None if the set is empty.
 */
CORE_API boost::optional<std::uint32_t> GetNearestAvailableTileKeyLevel(
    const TileKeyLevels& levels, const std::uint32_t reference_level);

CORE_API std::ostream& operator<<(std::ostream& out,
                                  const geo::TileKey& tile_key);

}  // namespace geo
}  // namespace olp

namespace std {
/**
 * @brief specialization of std::hash
 */

template <>
struct hash<olp::geo::TileKey> {
  /**
   * Hash function for tile keys, uses the 64-bit morton code
   * (TileKey::ToQuadKey64()).
   */
  std::size_t operator()(const olp::geo::TileKey& tile_key) const {
    return std::hash<std::uint64_t>()(tile_key.ToQuadKey64());
  }
};

}  // namespace std
