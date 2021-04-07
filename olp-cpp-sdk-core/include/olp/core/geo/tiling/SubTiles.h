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

#include <algorithm>
#include <cstdint>
#include <iterator>

#include <olp/core/CoreApi.h>
#include <olp/core/geo/tiling/TileKey.h>

namespace olp {
namespace geo {

/**
 * @brief A container of child tiles.
 */
class CORE_API SubTiles {
  friend class Iterator;

 public:
  /// The tile key iterator.
  class Iterator : public std::iterator<std::forward_iterator_tag, TileKey> {
    friend class SubTiles;

   public:
    /// An alias for the tile key.
    using ValueType = TileKey;

    /**
     * @brief Gets a reference to the tile key.
     *
     * @return The reference to the tile key.
     */
    ValueType operator*() const;

    /**
     * @brief Iterates to the next tile.
     *
     * @return A reference to this.
     */
    Iterator& operator++();

    /**
     * @brief Iterates the specified number of times to the next tile.
     *
     * @return A new iterator.
     */
    Iterator operator++(int);

    /**
     * @brief Checks whether the iterators are equal.
     *
     * @param other The other iterator.
     *
     * @return True if the iterators are equal; false otherwise.
     */
    bool operator==(Iterator& other) const;

    /**
     * @brief Checks whether the iterators are not equal.
     *
     * @param other The other iterator.
     *
     * @return True if the iterators are not equal; false otherwise.
     */
    bool operator!=(Iterator& other) const;

   private:
    explicit Iterator(const SubTiles& parent, std::uint32_t index = 0);

    const SubTiles& parent_;
    std::uint32_t index_{0};
  };

  /// An alias for the iterator.
  using ConstIterator = Iterator;

  /**
   * @brief Creates a `SubTiles` instance.
   *
   * @param tile_key The tile key.
   * @param level The tile level.
   * @param mask The mask that indicates how many tiles should be included.
   */
  SubTiles(const TileKey& tile_key, std::uint32_t level = 1,
           std::uint16_t mask = ~0);

  /**
   * @brief Gets the size of the child tile.
   *
   * @return The size of the child tile.
   */
  size_t Size() const;

  /// Returns an iterator to the beginning.
  Iterator begin();
  /// Returns an iterator to the end.
  Iterator end();

  /// Returns a constant iterator to the beginning.
  ConstIterator begin() const;
  /// Returns a constant iterator to the end.
  ConstIterator end() const;

  /// Returns a constant iterator to the beginning of the container.
  ConstIterator cbegin() const;
  /// Returns a constant iterator to the end of the container.
  ConstIterator cend() const;

 private:
  void Skip(std::uint32_t& index) const;

  const TileKey tile_key_;
  const std::uint32_t level_{0};
  const std::uint32_t count_{0};
  const std::uint16_t mask_{0};
  const std::uint32_t shift_{0};
};

inline SubTiles::Iterator::Iterator(const SubTiles& parent, std::uint32_t index)
    : parent_(parent), index_(index) {
  // Skip zeros
  parent_.Skip(index_);
}

inline SubTiles::Iterator::ValueType SubTiles::Iterator::operator*() const {
  const TileKey& parent_key = parent_.tile_key_;
  const std::uint32_t sub_level = parent_.level_;

  return TileKey::FromRowColumnLevel(
      (parent_key.Row() << sub_level) | (index_ >> sub_level),
      (parent_key.Column() << sub_level) | (index_ & ((1 << sub_level) - 1)),
      parent_key.Level() + sub_level);
}

inline SubTiles::Iterator& SubTiles::Iterator::operator++() {
  parent_.Skip(++index_);
  return *this;
}

inline SubTiles::Iterator SubTiles::Iterator::operator++(int) {
  const Iterator copy(*this);
  parent_.Skip(++index_);
  return copy;
}

inline bool SubTiles::Iterator::operator==(Iterator& other) const {
  return parent_.tile_key_ == other.parent_.tile_key_ && index_ == other.index_;
}

inline bool SubTiles::Iterator::operator!=(Iterator& other) const {
  return !(*this == other);
}

inline SubTiles::SubTiles(const TileKey& tile_key, std::uint32_t level,
                          std::uint16_t mask)
    : tile_key_(tile_key),
      level_(level),
      count_(1 << (level_ << 1)),
      mask_(mask),
      shift_(level_ > 2 ? (level_ - 2) << 1 : 0) {}

inline size_t SubTiles::Size() const { return count_; }

inline SubTiles::Iterator SubTiles::begin() { return Iterator(*this); }

inline SubTiles::Iterator SubTiles::end() { return Iterator(*this, count_); }

inline SubTiles::ConstIterator SubTiles::begin() const {
  return ConstIterator(*this);
}

inline SubTiles::ConstIterator SubTiles::end() const {
  return ConstIterator(*this, count_);
}

inline SubTiles::ConstIterator SubTiles::cbegin() const {
  return ConstIterator(*this);
}

inline SubTiles::ConstIterator SubTiles::cend() const {
  return ConstIterator(*this, count_);
}

inline void SubTiles::Skip(std::uint32_t& index) const {
  if (mask_ == static_cast<std::uint16_t>(-1))
    return;

  while (index < count_ && (mask_ & (1 << (index >> shift_))) == 0)
    ++index;
}

}  // namespace geo
}  // namespace olp
