/*
 * Copyright (C) 2019-2025 HERE Europe B.V.
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

#include <cassert>
#include <cstddef>
#include <functional>
#include <map>
#include <utility>

#include <olp/core/porting/try_emplace.h>

namespace olp {
namespace utils {
/**
 * @brief The default cost operator for `LruCache`.
 *
 * The default implementation returns "1" for every object,
 * which means that each object in the cache is treated as equally sized.
 *
 * @return The non-zero value for the specified object.
 */
template <typename T>
struct CacheCost {
  /// Gets the size of the specified object.
  std::size_t operator()(const T&) const { return 1; }
};

/**
 * @brief A generic key-value LRU cache.
 *
 * This cache stores elements in a map up to the specified maximum size.
 * The cache eviction follows the LRU principle: the element that was accessed
 * last is evicted last.
 *
 * The specializations return a non-zero value for any given object.
 *
 * @tparam Key The `LruCache` key type.
 * @tparam Value The `LruCache` value type.
 * @tparam CacheCostFunc The cache cost functor.
 * The specializations should return a non-zero value for any given object.
 * The default implementation returns "1" as the size for each object.
 * @tparam Compare The comparison function to be used for sorting keys.
 * The default value of `std::less` is used, which sorts the keys in ascending
 * order.
 * @tparam Alloc The allocator to be used for allocating internal data.
 * The default value of `std::allocator` is used.
 */
template <typename Key, typename Value,
          typename CacheCostFunc = CacheCost<Value>,
          typename Compare = std::less<Key>,
          template <typename> class Alloc = std::allocator>
class LruCache {
  struct Bucket;
  using MapType =
      std::map<Key, Bucket, Compare, Alloc<std::pair<const Key, Bucket> > >;

 public:
  /// An alias for the eviction function.
  using EvictionFunction = std::function<void(const Key&, Value&&)>;

  /// An alias for the key comparison function.
  using CompareType = typename MapType::key_compare;

  /// An alias for the cache allocator type.
  using AllocType = typename MapType::allocator_type;

  /**
   * @brief A type of objects to be stored.
   *
   * Each object is defined by a key-value pair.
   */
  class ValueType {
   public:
    /**
     * @brief Gets the key of the `ValueType` object.
     *
     * @return The key of the `ValueType` object.
     */
    inline const Key& key() const;

    /**
     * @brief Gets the value of the `ValueType` object.
     *
     * @return The value of the `ValueType` object.
     */
    inline const Value& value() const;

   protected:
    /// A typename of the map constant iterator.
    typename MapType::const_iterator m_it;
  };

  /// A constant iterator of the `LruCache` object.
  class const_iterator : public ValueType {
   public:
    /// A typedef for the iterator category.
    typedef std::bidirectional_iterator_tag iterator_category;
    /// A typedef for the difference type.
    typedef std::ptrdiff_t difference_type;
    /// A typedef for the `ValueType` type.
    typedef ValueType value_type;
    /// A typedef for the `ValueType` constant reference.
    typedef const value_type& reference;
    /// A typedef for the `ValueType` constant pointer.
    typedef const value_type* pointer;

    /// Creates a constant iterator object.
    const_iterator() = default;
    /// Creates a constant iterator object.
    const_iterator(const const_iterator&) = default;
    /**
     * @brief Copies this and the specified iterator to this.
     *
     * @return A refenrence to this object.
     */
    const_iterator& operator=(const const_iterator&) = default;

    /**
     * @brief Checks whether the values of the `const_iterator`
     * parameter are the same as the values of the `other` parameter.
     *
     * @param other The `const_iterator` instance.
     *
     * @return True if the values are the same; false otherwise.
     */
    inline bool operator==(const const_iterator& other) const;
    /**
     * @brief Checks whether the values of the `const_iterator`
     * parameter are not the same as the values of the `other` parameter.
     *
     * @param other The `const_iterator` instance.
     *
     * @return True if the values are not the same; false otherwise.
     */
    inline bool operator!=(const const_iterator& other) const {
      return !operator==(other);
    }

    /**
     * @brief Iterates to the next `LruCache` object.
     *
     * @return A reference to this.
     */
    inline const_iterator& operator++();
    /**
     * @brief Iterates the specified number of times to the next `LruCache`
     * object.
     *
     * @return A new constant iterator.
     */
    inline const_iterator operator++(int);

    /**
     * @brief Iterates to the previous `LruCache` object.
     *
     * @return A reference to this.
     */
    inline const_iterator& operator--();
    /**
     * @brief Iterates the specified number of times to the previous `LruCache`
     * object.
     *
     * @return A new constant iterator.
     */
    inline const_iterator operator--(int);

    /**
     * @brief Gets a reference to this object.
     *
     * @return The reference to this.
     */
    reference operator*() const { return *this; }

    /**
     * @brief Gets a pointer to this object.
     *
     * @return The pointer to this.
     */
    pointer operator->() const { return this; }

   private:
    template <typename, typename, typename, typename, template <typename> class>
    friend class LruCache;

    explicit const_iterator(const typename MapType::const_iterator& it) {
      this->m_it = it;
    }

    explicit const_iterator(const typename MapType::iterator& it) {
      this->m_it = it;
    }
  };

  /**
   * @brief Creates an `LruCache` instance.
   *
   * Creates an invalid `LruCache` with the maximum size of `0`
   * that caches nothing.
   *
   * @param alloc The allocator for the cache.
   */
  explicit LruCache(const AllocType& alloc = AllocType())
      : map_(alloc),
        first_(map_.end()),
        last_(map_.end()),
        max_size_(0),
        size_(0) {}

  /**
   * @brief Creates an `LruCache` instance.
   *
   * @param maxSize The maximum size of values this cache can keep.
   * @param cacheCostFunc The function this cache uses to compute the
   *        caching cost of each cached value.
   * @param compare The function object for comparing keys.
   * @param alloc The allocator for the cache.
   */
  LruCache(std::size_t maxSize, CacheCostFunc cacheCostFunc = CacheCostFunc(),
           const CompareType& compare = CompareType(),
           const AllocType& alloc = AllocType())
      : cache_cost_func_(std::move(cacheCostFunc)),
        map_(compare, alloc),
        first_(map_.end()),
        last_(map_.end()),
        max_size_(maxSize),
        size_(0) {}

  /// The deleted copy constructor.
  LruCache(const LruCache&) = delete;

  /// The default move constructor.
  LruCache(LruCache&& other) noexcept
      : eviction_callback_(std::move(other.eviction_callback_)),
        cache_cost_func_(std::move(other.cache_cost_func_)),
        map_(std::move(other.map_)),
        first_(other.first_ != map_.end() ? map_.find(other.first_->first)
                                          : map_.end()),
        last_(other.last_ != map_.end() ? map_.find(other.last_->first)
                                        : map_.end()),
        max_size_(other.max_size_),
        size_(other.size_) {}

  /// The deleted assignment operator.
  LruCache& operator=(const LruCache&) = delete;

  /// The default move assignment operator.
  LruCache& operator=(LruCache&& other) noexcept {
    eviction_callback_ = std::move(other.eviction_callback_);
    cache_cost_func_ = std::move(other.cache_cost_func_);
    map_ = std::move(other.map_);
    first_ = other.first_ != map_.end() ? map_.find(other.first_->first)
                                        : map_.end();
    last_ =
        other.last_ != map_.end() ? map_.find(other.last_->first) : map_.end();
    std::swap(max_size_, other.max_size_);
    std::swap(size_, other.size_);

    return *this;
  }

  /**
   * @brief Inserts a key-value pair in the cache.
   *
   * @note If the key already exists in the cache, it is promoted in the
   * LRU, but its value and cost are not updated. To update or insert existing
   * values, use `InsertOrAssign` instead.
   *
   * If the key or value is an rvalue reference, they are moved;
   * copied otherwise.
   *
   * @note This function behaves analogously to `std::map`. Even if the
   * insertion fails, the key and value can be moved. Do not access them
   * further.
   *
   * @param key The key to add.
   * @param value The value to add.
   *
   * @return A pair of bool and an iterator, analogously to
   * `std::map::insert()`. If the bool is true, the item is inserted, and the
   * iterator points to the newly inserted item. If the bool is false and the
   * iterator points to `end()`, the item cannot be inserted. Otherwise, the
   * bool is false, and the iterator points to the item that prevented the
   * insertion.
   */
  template <typename _Key, typename _Value>
  std::pair<const_iterator, bool> Insert(_Key&& key, _Value&& value);

  /**
   * @brief Inserts a key-value pair in the cache or updates an existing
   * key-value pair.
   *
   * @note If the key already exists in the cache, its value and cost are
   * updated. Not to update the existing key-value pair, use `Insert` instead.
   *
   * @param key The key to add.
   * @param value The value to add.
   *
   * @return A pair of bool and an iterator, analogously to
   * `std::map::insert_or_assign()`. If the bool is true, the item is inserted,
   * and the iterator points to the newly inserted item. If the bool is false
   * and the iterator points to `end()`, the item cannot be inserted. Otherwise,
   * the bool is false, and the iterator points to the item that is assigned.
   */
  template <typename _Value>
  std::pair<const_iterator, bool> InsertOrAssign(Key key, _Value&& value);

  /**
   * @brief Removes a key from the cache.
   *
   * @param key The key to remove.
   *
   * @return True if the key exists and is removed from the cache; false
   * otherwise.
   */
  bool Erase(const Key& key) {
    auto it = map_.find(key);
    if (it == map_.end())
      return false;

    Erase(it, false);
    return true;
  }

  /**
   * @brief Removes a key from the cache.
   *
   * @param it The iterator of the key that should be removed.
   *
   * @return A new iterator.
   */
  const_iterator Erase(const_iterator& it) {
    auto prev = it++;

    Erase(prev->key());

    return it;
  }

  /**
   * @brief Gets the current size of the cache.
   *
   * @return The current cache size.
   */
  std::size_t Size() const { return size_; }

  /**
   * @brief Gets the maximum size of the cache.
   *
   * @return The maximum cache size.
   */
  std::size_t GetMaxSize() const { return max_size_; }

  /**
   * @brief Sets the new maximum size of the cache.
   *
   * If the new maximum size is smaller than the current size, items are evicted
   * until the cache shrinks to less than or equal to the new maximum size.
   *
   * @param maxSize The new maximum size of the cache.
   */
  void Resize(size_t maxSize) {
    max_size_ = maxSize;
    evict();
  }

  /**
   * @brief Finds a value in the cache.
   *
   * @note This function promotes the item pointed to by a key if found.
   *
   * @param key The key to find.
   *
   * @return If found, the iterator to the value; the iterator pointing
   * to `end()` otherwise.
   */
  const_iterator Find(const Key& key) {
    auto it = map_.find(key);
    if (it != map_.end())
      Promote(it);
    return const_iterator{it};
  }

  /**
   * @brief Finds a value in the cache.
   *
   * @note This function does NOT promote the item pointed to by a key if found.
   *
   * @param key The key to find.
   *
   * @return If found, the iterator to the value; the iterator pointing
   * to `end()` otherwise.
   */
  const_iterator FindNoPromote(const Key& key) const {
    auto it = map_.find(key);
    return const_iterator{it};
  }

  /**
   * @brief Finds a value in the cache.
   *
   * @note This function promotes the item pointed to by a key if found.
   *
   * @param key The key to find.
   * @param nullValue The value to return if the key-value pair is not in the
   * cache
   * @return If found, a constant reference to the value; `nullValue` otherwise.
   */
  const Value& Find(const Key& key, const Value& nullValue) {
    auto it = Find(key);
    return it == end() ? nullValue : it.value();
  }

  /// Returns a constant iterator to the beginning.
  const_iterator begin() const { return const_iterator{first_}; }

  /// Returns a constant iterator to the end.
  const_iterator end() const { return const_iterator{map_.end()}; }

  /// Returns a reverse constant iterator to the beginning.
  const_iterator rbegin() const { return const_iterator{last_}; }

  /// Returns a reverse constant iterator to the end.
  const_iterator rend() const { return const_iterator{map_.end()}; }

  /**
   * @brief Removes all items from the cache.
   *
   * Removes all content but does not reset the eviction callback
   * or maximum size.
   */
  void Clear() {
    map_.clear();
    first_ = last_ = map_.end();
    size_ = 0u;
  }

  /**
   * @brief Sets a function that is invoked when a value is
   * evicted from the cache.
   *
   * @note The function must not modify the cache in the
   * callback. The value can be safely moved. If not, it is destroyed when
   * the function returns.
   *
   * To reset the eviction callback, pass `nullptr`.
   *
   * @param func The function to be called on eviction.
   */
  void SetEvictionCallback(EvictionFunction func) {
    eviction_callback_ = std::move(func);
  }

 private:
  friend struct CacheLruChecker;  // for unit-tests

  EvictionFunction eviction_callback_;
  CacheCostFunc cache_cost_func_;
  MapType map_;
  typename MapType::iterator first_;
  typename MapType::iterator last_;
  std::size_t max_size_;
  std::size_t size_;

  // Checks whether two keys are equal using only the comparison
  // operator.
  inline bool IsEqual(const Key& key1, const Key& key2) {
    auto comp = map_.key_comp();
    return !comp(key1, key2) && !comp(key2, key1);
  }

  // The bucket that contains the value and the pointer to the previous
  // and next items in the LRU chain.
  struct Bucket {
    using Iterator = typename MapType::iterator;

    // note - these constructors are only here because MSVC2013 doesn't
    // auto-generate them as the standard mandates
#if defined(_MSC_VER) && _MSC_VER < 1900
    inline Bucket(Iterator next, Iterator previous, Value&& value)
        : next_(std::move(next)),
          previous_(std::move(previous)),
          value_(std::move(value)) {}
    inline Bucket(Iterator next, Iterator previous, const Value& value)
        : next_(std::move(next)),
          previous_(std::move(previous)),
          value_(value) {}
    inline Bucket(const Bucket&) = delete;
    inline Bucket(Bucket&& other)
        : next_(std::move(other.next_)),
          previous_(std::move(other.previous_)),
          value_(std::move(other.value_)) {}
#endif

    Iterator next_;
    Iterator previous_;
    Value value_;

    inline void setNext(Iterator it) { next_ = std::move(it); }

    inline void setPrevious(Iterator it) { previous_ = std::move(it); }
  };

  void Erase(typename MapType::iterator it, bool);
  void AddInternal(const typename MapType::iterator& it, std::size_t cost,
                   std::size_t* oldCost = nullptr);
  void Promote(const typename MapType::iterator& it);
  void PopLast();
  void evict() {
    while (size_ > max_size_)
      PopLast();
  }
};

template <typename Key, typename Value, typename CacheCostFunc,
          typename Compare, template <typename> class Alloc>
template <typename _Key, typename _Value>

/**
 * @brief Inserts a key-value pair in the cache.
 *
 * @note If the key already exists in the cache, it is promoted in the
 * LRU, but its value and cost are not updated. To update or insert existing values,
 * use `InsertOrAssign` instead.
 *
 * If the key or value is an rvalue reference, they are moved;
 * copied otherwise.
 *
 * @note This function behaves analogously to `std::map`. Even if the insertion
 * fails, the key and value can be moved. Do not access them further.
 *
 * @param key The key to add.
 * @param value The value to add.
 *
 * @return A pair of bool and an iterator, analogously to `std::map::insert()`.
 *         If the bool is true, the item is inserted, and the iterator points
 * to the newly inserted item. If the bool is false and the iterator points to
 * `end()`, the item cannot be inserted. Otherwise, the bool is false, and
 * the iterator points to the item that prevented the insertion.
 */
auto LruCache<Key, Value, CacheCostFunc, Compare, Alloc>::Insert(_Key&& key,
                                                                 _Value&& value)
    -> std::pair<const_iterator, bool> {
  Bucket bucket{map_.end(), map_.end(), std::forward<_Value>(value)};

  // If the item is too large, do not insert it.
  std::size_t valueCost = cache_cost_func_(bucket.value_);
  if (valueCost > max_size_)
    return std::make_pair(const_iterator{end()}, false);

  auto it =
      porting::try_emplace(map_, std::forward<_Key>(key), std::move(bucket));

  if (it.second) {
    AddInternal(it.first, valueCost);
    return std::make_pair(const_iterator{it.first}, true);
  }
  Promote(it.first);
  return std::make_pair(const_iterator{it.first}, false);
}

template <typename Key, typename Value, typename CacheCostFunc,
          typename Compare, template <typename> class Alloc>
template <typename _Value>

/**
 * @brief Inserts a key-value pair in the cache or updates an existing key-value pair.
 *
 * @note If the key already exists in the cache, its value and cost are
 * updated. Not to update existing key-value pairs, use `Insert` instead.
 *
 * @param key The key to add.
 * @param value The value to add.
 *
 * @return A pair of bool and an iterator, analogously to
 * `std::map::insert_or_assign()`. If the bool is true, the item is inserted,
 * and the iterator points to the newly inserted item. If the bool is false
 * and the iterator points to `end()`, the item cannot be inserted. Otherwise,
 * the bool is false, and the iterator points to the item that is assigned.
 */
auto LruCache<Key, Value, CacheCostFunc, Compare, Alloc>::InsertOrAssign(
    Key key, _Value&& value) -> std::pair<const_iterator, bool> {
  // as we need "key" with the right type a couple of times, create a temp
  // copy to prevent implicit conversions below
  auto it = map_.lower_bound(key);
  if (it != map_.end() && IsEqual(key, it->first)) {
    // element already exists, update it
    std::size_t oldCost = cache_cost_func_(it->second.value_);
    it->second.value_ = std::forward<_Value>(value);
    std::size_t newCost = cache_cost_func_(it->second.value_);
    AddInternal(it, newCost, &oldCost);
    return std::make_pair(const_iterator{it}, false);
  } else {
    // element doesn't exist, insert it
    Bucket bucket{map_.end(), map_.end(), std::forward<_Value>(value)};
    std::size_t newCost = cache_cost_func_(bucket.value_);
    if (newCost > max_size_)
      return std::make_pair(const_iterator{end()}, false);

    auto new_it =
        map_.insert(it, std::make_pair(std::move(key), std::move(bucket)));
    AddInternal(new_it, newCost);
    return std::make_pair(const_iterator{new_it}, true);
  }
}

template <typename Key, typename Value, typename CacheCostFunc,
          typename Compare, template <typename> class Alloc>
void LruCache<Key, Value, CacheCostFunc, Compare, Alloc>::Promote(
    const typename MapType::iterator& it) {
  if (it == first_)
    return;  // nothing to do

  // re-link previous and next buckets together
  it->second.previous_->second.setNext(it->second.next_);
  if (it->second.next_ != map_.end())
    it->second.next_->second.setPrevious(it->second.previous_);
  else
    last_ = it->second.previous_;

  // re-link our bucket
  it->second.setPrevious(map_.end());
  it->second.setNext(first_);

  // update current head to ourselves
  first_->second.setPrevious(it);
  first_ = it;
}

template <typename Key, typename Value, typename CacheCostFunc,
          typename Compare, template <typename> class Alloc>
void LruCache<Key, Value, CacheCostFunc, Compare, Alloc>::AddInternal(
    const typename MapType::iterator& it, std::size_t cost,
    std::size_t* oldCost) {
  if (!oldCost) {
    // new bucket added
    if (map_.size() == 1) {
      // we're the first and only one.
      assert(last_ == map_.end() && first_ == map_.end());
      last_ = first_ = it;
    } else {
      // relink first bucket
      it->second.setNext(first_);
      first_->second.setPrevious(it);
      first_ = it;
    }
    size_ += cost;
  } else {
    // key already in map - only promote
    size_ += cost - *oldCost;
    Promote(it);
  }

  evict();
}

template <typename Key, typename Value, typename CacheCostFunc,
          typename Compare, template <typename> class Alloc>
void LruCache<Key, Value, CacheCostFunc, Compare, Alloc>::Erase(
    typename MapType::iterator it, bool doEvictionCallback) {
  std::size_t cost = cache_cost_func_(it->second.value_);

  if (it->second.next_ == map_.end())
    last_ = it->second.previous_;
  else
    it->second.next_->second.setPrevious(it->second.previous_);

  if (it->second.previous_ == map_.end())
    first_ = it->second.next_;
  else
    it->second.previous_->second.setNext(it->second.next_);

  if (doEvictionCallback && eviction_callback_)
    eviction_callback_(it->first, std::move(it->second.value_));

  map_.erase(it);
  size_ -= cost;
}

template <typename Key, typename Value, typename CacheCostFunc,
          typename Compare, template <typename> class Alloc>
void LruCache<Key, Value, CacheCostFunc, Compare, Alloc>::PopLast() {
  // assert if the cache is empty
  assert(last_ != map_.end());
  // assert that our item's 'next' is pointing indeed to the end of the cache
  assert(last_->second.next_ == map_.end());

  Erase(last_, true);
}

template <typename Key, typename Value, typename CacheCostFunc,
          typename Compare, template <typename> class Alloc>
inline const Key&
LruCache<Key, Value, CacheCostFunc, Compare, Alloc>::ValueType::key() const {
  return m_it->first;
}

template <typename Key, typename Value, typename CacheCostFunc,
          typename Compare, template <typename> class Alloc>
inline const Value&
LruCache<Key, Value, CacheCostFunc, Compare, Alloc>::ValueType::value() const {
  return m_it->second.value_;
}

template <typename Key, typename Value, typename CacheCostFunc,
          typename Compare, template <typename> class Alloc>
inline bool
LruCache<Key, Value, CacheCostFunc, Compare, Alloc>::const_iterator::operator==(
    const const_iterator& other) const {
  return this->m_it == other.m_it;
}

template <typename Key, typename Value, typename CacheCostFunc,
          typename Compare, template <typename> class Alloc>
inline typename LruCache<Key, Value, CacheCostFunc, Compare,
                         Alloc>::const_iterator&
LruCache<Key, Value, CacheCostFunc, Compare, Alloc>::const_iterator::
operator++() {
  this->m_it = this->m_it->second.next_;
  return *this;
}

template <typename Key, typename Value, typename CacheCostFunc,
          typename Compare, template <typename> class Alloc>
inline
    typename LruCache<Key, Value, CacheCostFunc, Compare, Alloc>::const_iterator
    LruCache<Key, Value, CacheCostFunc, Compare, Alloc>::const_iterator::
    operator++(int) {
  typename MapType::const_iterator old_value = this->m_it;
  this->m_it = this->m_it->second.next_;
  return const_iterator{old_value};
}

template <typename Key, typename Value, typename CacheCostFunc,
          typename Compare, template <typename> class Alloc>
inline typename LruCache<Key, Value, CacheCostFunc, Compare,
                         Alloc>::const_iterator&
LruCache<Key, Value, CacheCostFunc, Compare, Alloc>::const_iterator::
operator--() {
  this->m_it = this->m_it->second.previous_;
  return *this;
}

template <typename Key, typename Value, typename CacheCostFunc,
          typename Compare, template <typename> class Alloc>
inline
    typename LruCache<Key, Value, CacheCostFunc, Compare, Alloc>::const_iterator
    LruCache<Key, Value, CacheCostFunc, Compare, Alloc>::const_iterator::
    operator--(int) {
  typename MapType::const_iterator old_value = this->m_it;
  this->m_it = this->m_it->second.previous_;
  return const_iterator{old_value};
}

}  // namespace utils
}  // namespace olp
