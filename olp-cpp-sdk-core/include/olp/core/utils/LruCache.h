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

#include <cstddef>
#include <functional>
#include <map>

#include <olp/core/utils/UnorderedMap.h>

namespace olp {
namespace utils {

/**
 * @brief Cost operator for LruCache
 *
 * Specializations should return a non-zero value for any given object.
 *
 * Default implementation returns "1" for every object, meaning that each object
 * in the cache is treated as equally sized.
 */
template <typename T>
struct CacheCost {
  std::size_t operator()(const T&) const { return 1; }
};

/**
 * @brief An eviction callback that does nothing
 *
 * @param Key key used to access a cached element.
 * @param Value value stored for a cached element.
 */
struct NullEvictionCallback {
  template <class Key, class Value>
  void operator()(const Key&, Value&&) const {}
};

/**
 * @brief Generic key-value LRU cache
 *
 * This cache stores elements in a map. If the maximum size of the cache is
 * exceeded, the least recently used element will be evicted until the size is
 * less or equal to the maximum size.
 *
 * Implementation: The cache is implemented using UnorderedMap and an intrusive
 * doubly-linked list, exploiting the fact that the order of the entries in the
 * UnorderedMap is stable under most operations (excluding erase).
 *
 * @tparam Key Type of the key used to access a cached element.
 * @tparam Value Type of the value stored for a cached element.
 * @tparam CacheCostFunc Functor that maps a value to its cost.
 * The cost of a value must never change while the value is stored in the cache.
 *
 * @tparam HashFunc Functor that maps a key to a hash value.
 * @tparam Index Index type used internally in the underlying hash map.
 *  Make sure that it supports the amount of elements you want to insert
 */
template <typename Key, typename Value, class CacheCostFunc = CacheCost<Value>,
          class HashFunc = std::hash<Key>, typename Index = uint32_t,
          template <typename, typename> class Cont = std::vector,
          template <typename> class Alloc = std::allocator>
class LruCache {
 private:
  static constexpr Index kHeadIndex = 0;

  struct ListEntry {
    Index prev_ = kHeadIndex;
    Index next_ = kHeadIndex;
  };

  struct EntryWithData : public ListEntry {
    template <typename T>
    explicit EntryWithData(T&& value) : value_(std::forward<T>(value)) {}

    Value value_;
  };

  using MapType =
      UnorderedMap<Key, EntryWithData, Index, HashFunc, Cont, Alloc>;
  using AllocType = const Alloc<typename MapType::Entry>;

 public:
  using EvictionFunction = std::function<void(const Key&, Value&&)>;
  /**
   * @brief Bidirectional iterator over the cache content.
   *
   * Iteration is in eviction order from most recently used to least recently
   * used.
   */
  class const_iterator {
   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = const_iterator;

    bool operator==(const const_iterator& other) const {
      return idx_ == other.idx_;
    }

    bool operator!=(const const_iterator& other) const {
      return !operator==(other);
    }

    const_iterator& operator++() {
      idx_ = cache_->ListGet(idx_).next_;
      return *this;
    }

    const_iterator operator++(int) {
      const_iterator old_it = *this;
      operator++();
      return old_it;
    }

    const_iterator& operator--() {
      idx_ = cache_->ListGet(idx_).prev_;
      return *this;
    }

    const_iterator operator--(int) {
      const_iterator old_it = *this;
      operator--();
      return old_it;
    }

    const value_type& operator*() const { return *this; }

    const value_type* operator->() const { return this; }

    const Key& key() const { return (cache_->map_.begin() + idx_ - 1)->key(); }

    const Value& value() const {
      return (cache_->map_.begin() + idx_ - 1)->data().value_;
    }

   private:
    friend class LruCache;

    const_iterator(const Index idx, const LruCache& cache)
        : idx_(idx), cache_(&cache) {}

   private:
    Index idx_;
    const LruCache* cache_;
  };

  /**
   * @brief Creates an empty cache.
   * @param maxSize The maximum size of the cache.
   * @param cacheCostFunc the function this cache uses to compute the
   *        caching cost of each cached value
   * @param alloc The allocator used to allocate memory for the elements.
   */
  explicit LruCache(size_t maxSize,
                    CacheCostFunc cacheCostFunc = CacheCostFunc(),
                    const AllocType& alloc = AllocType())
      : map_(0, alloc),
        head_(),
        max_size_(maxSize),
        size_(0),
        cache_cost_func_(std::move(cacheCostFunc)),
        eviction_func_(NullEvictionCallback()) {}

  ~LruCache() = default;

  /// deleted copy constructor
  LruCache(const LruCache&) = delete;

  /// default move constructor
  LruCache(LruCache&& other)
      : map_(std::move(other.map_)),
        head_(other.head_),
        max_size_(other.max_size_),
        size_(other.size_),
        cache_cost_func_(std::move(other.cache_cost_func_)),
        eviction_func_(std::move(other.eviction_func_)) {
    other.map_.Clear();
    other.head_ = ListEntry();
    other.size_ = 0;
  }

  /// deleted assignment operator
  LruCache& operator=(const LruCache&) = delete;

  /// default move assignment operator
  LruCache& operator=(LruCache&& other) {
    map_ = std::move(other.map_);
    head_ = other.head_;
    max_size_ = other.max_size_;
    size_ = other.size_;
    eviction_func_ = std::move(other.eviction_func_);

    other.map_.Clear();
    other.head_ = ListEntry();
    other.size_ = 0;

    return *this;
  }

  /**
   * @brief Inserts a key/value to the cache.
   *
   * If the key already exists, the key is promoted to the front of the LRU
   * list, but its value is not updated. If the cost of the value exceeds the
   * max size, this operation does nothing.
   *
   * @param Key The key to be inserted.
   * @param Value The value to be assigned to the key.
   * @return A pair consisting of an iterator to the inserted element (or to the
   * element that prevented the insertion) and a bool indicating if the
   * insertion took place. If the cost of the value exceeded the max size of
   * the cache, a pair of end( ) and false is returned.
   */
  template <typename KeyArg, typename T>
  std::pair<const_iterator, bool> Insert(KeyArg&& key, T&& value) {
    const size_t cost = cache_cost_func_(value);
    if (cost > max_size_) {
      return std::make_pair(end(), false);
    }

    typename MapType::iterator iter;
    bool inserted;
    std::tie(iter, inserted) =
        map_.tryEmplace(std::forward<KeyArg>(key), std::forward<T>(value));
    Index index = GetIndex(iter);
    if (!inserted) {
      ListUnlink(iter);
      ListPushFront(iter);
      return std::make_pair(const_iterator(index, *this), false);
    }
    if (cache_cost_func_(iter->data().value_) == cost) {
      ListPushFront(iter);
      size_ += cost;
      Prune(index);
      return std::make_pair(const_iterator(index, *this), true);
    } else {
      return std::make_pair(end(), false);
    }
  }

  /**
   * @brief Inserts a key/value to the cache or updates an existing key/value
   * pair.
   *
   * If the key already exists it is updated with the new value and the key is
   * promoted to the front of the LRU list. If the cost of the value exceeds
   * the max size, this operation does nothing.
   *
   * In the case of an assignment the eviction callback is called for the old
   * key/value.
   *
   * @param Key The key to be inserted or updated.
   * @param Value The value to be assigned to the key.
   * @return A pair consisting of an iterator to the inserted or updated element
   * and a bool which is true if the insertion took place and false if the
   * assignment took place or the elements cost exceeded the max size of the
   * cache. If the cost of the value exceeded the max size of the cache, a
   * pair of end( ) and false is returned.
   */
  template <typename KeyArg, typename T>
  std::pair<const_iterator, bool> InsertOrAssign(KeyArg&& key, T&& value) {
    const size_t cost = cache_cost_func_(value);
    if (cost > max_size_) {
      return std::make_pair(end(), false);
    }

    typename MapType::iterator iter;
    bool inserted;
    std::tie(iter, inserted) =
        map_.TryEmplace(std::forward<KeyArg>(key), std::forward<T>(value));
    Index index = GetIndex(iter);
    if (!inserted) {
      const size_t old_cost = cache_cost_func_(iter->data().value_);
      eviction_func_(iter->key(), std::move(iter->data().value_));
      ListUnlink(iter);
      size_ -= old_cost;
      iter->data().value_ = std::forward<T>(value);
    }
    if (cache_cost_func_(iter->data().value_) == cost) {
      ListPushFront(iter);
      size_ += cost;
      Prune(index);
      return std::make_pair(const_iterator(index, *this), inserted);
    } else {
      return std::make_pair(end(), false);
    }
  }

  /**
   * @brief Force eviction of a specific element.
   *
   * @param key Key of the iterator to be evicted.
   * @return true if an element with the given key existed and was evicted.
   */
  bool Erase(const Key& key) {
    auto iter = map_.Find(key);
    if (iter != map_.end()) {
      Index index = kHeadIndex;
      EraseInternal(iter, index);
      return true;
    } else {
      return false;
    }
  }

  /**
   * @brief Forces eviction of a specific element.
   *
   * @param iter Iterator to the element to be evicted.
   * @return An iterator pointing to the element after the evicted element in
   * LRU order.
   */
  const_iterator Erase(const_iterator iter) {
    const auto map_iter = map_.begin() + iter.idx_ - 1;
    Index index = map_iter->data().next_;
    EraseInternal(map_iter, index);
    return const_iterator(index, *this);
  }

  /**
   * @brief size the current size of the cache
   * @return the size
   */
  size_t Size() const { return size_; }

  /**
   * @brief size the maximum size of the cache
   * @return the maximum size
   */
  size_t GetMaxSize() const { return max_size_; }

  /**
   * @brief resize sets a new maximum size of the cache.
   *
   * If the new maximum size is smaller than the current size, items are evicted
   * until the cache shrinks to less than or equal the new maximum size.
   *
   * @param maxSize the new maximum size of the cache
   */
  void Resize(size_t maxSize) {
    max_size_ = maxSize;
    Index index = kHeadIndex;
    Prune(index);
  }

  /**
   * @brief Finds a key in the cache.
   *
   * The element is promoted to the front of the cache.
   *
   * @param key The key of the element to be found.
   * @return An iterator to the element or end( ) if the key is not in the
   * cache.
   */
  const_iterator Find(const Key& key) {
    auto iter = map_.Find(key);
    if (iter != map_.end()) {
      ListUnlink(iter);
      ListPushFront(iter);
      return const_iterator(GetIndex(iter), *this);
    } else {
      return end();
    }
  }

  /**
   * @brief find an value in the cache
   *
   * Note - this function will NOT promote the item pointed to by key if found
   *
   * @param key the key to find
   * @return an iterator to the value if found, otherwise, an iterator pointing
   * to end()
   */
  const_iterator FindNoPromote(const Key& key) const {
    auto iter = map_.Find(key);
    if (iter != map_.end()) {
      return const_iterator(GetIndex(iter), *this);
    } else {
      return end();
    }
  }

  /**
   * @brief Finds a key in the cache.
   *
   * The element is promoted to the front of the cache.
   * If the key is not in the map, the provided default value is returned.
   *
   * @param key The key of the element to be found.
   * @param default_value The value to be returned if the key was not found in
   * the cache.
   * @return The value assigned to the key or the provided default if the key is
   * not in the cache.
   */
  const Value& Find(const Key& key, const Value& default_value) {
    auto it = Find(key);
    return it != end() ? it->value() : default_value;
  }

  /**
   * @return An iterator to the beginning of the cache, pointing to the most
   * recently used cache element.
   */
  const_iterator begin() const { return const_iterator(head_.next_, *this); }

  /**
   * @return An iterator to the end of the cache, pointing after the last
   * recently used cache element.
   */
  const_iterator end() const { return const_iterator(kHeadIndex, *this); }

  /**
   * @brief Evicts all elements from the cache.
   */
  void Clear() {
    for (auto& entry : map_) {
      eviction_func_(entry.key(), std::move(entry.data().value_));
    }
    map_.Clear();
    head_ = ListEntry();
    size_ = 0;
  }
  /**
   * @brief setEvictionCallback set a function that is invoked when a value is
   * evicted from the cache Note - the function must not modify the cache in the
   * callback. The value can be safely moved, if not, it will be destroyed when
   * the function returns.
   *
   * To reset the eviction callback, pass a nullptr
   *
   * @param func the function to be called on eviction
   */
  void SetEvictionCallback(EvictionFunction func) {
    if (func) {
      eviction_func_ = std::move(func);
    }
  }

  /// Returns 1 if an element with key exists, 0 otherwise.
  Index Count(const Key& key) const { return map_.Count(key); }

  /// Returns true if the cache is empty, false otherwise.
  bool Empty() const { return map_.Empty(); }

  /// Returns the amount of memory allocated by the cache in bytes.
  size_t SizeInBytes() const { return map_.SizeInBytes(); }

  /// Returns the number of entries in the cache.
  size_t ItemsCount() const { return map_.Size(); }

 private:
  void Prune(Index& tracked_index) {
    while (size_ > max_size_) {
      auto least_recently_used = map_.begin() + head_.prev_ - 1;
      EraseInternal(least_recently_used, tracked_index);
    }
  }

  void EraseInternal(typename MapType::iterator iter, Index& tracked_index) {
    const size_t cost = cache_cost_func_(iter->data().value_);
    eviction_func_(iter->key(), std::move(iter->data().value_));
    ListUnlink(iter);
    size_ -= cost;
    map_.Erase(iter->key(), [&](Index moved_item, Index deleted_item) {
      auto deleted_iter = map_.begin() + deleted_item;

      // If the entry on the tracked index is moved, we replace it with the new
      // index
      if (tracked_index == moved_item + 1) {
        tracked_index = deleted_item + 1;
      }

      ListGet(deleted_iter->data().next_).prev_ = deleted_item + 1;
      ListGet(deleted_iter->data().prev_).next_ = deleted_item + 1;
    });
  }

  Index GetIndex(typename MapType::iterator iter) {
    return static_cast<Index>(iter - map_.begin() + 1);
  }

  ListEntry& ListGet(Index idx) {
    return idx == kHeadIndex ? head_ : (map_.begin() + idx - 1)->data();
  }

  const ListEntry& ListGet(Index idx) const {
    return idx == kHeadIndex ? head_ : (map_.begin() + idx - 1)->data();
  }

  void ListPushFront(typename MapType::iterator iter) {
    EntryWithData& entry = iter->data();
    const Index idx = GetIndex(iter);

    entry.next_ = head_.next_;
    ListGet(head_.next_).prev_ = idx;
    head_.next_ = idx;
    entry.prev_ = 0;
  }

  void ListUnlink(typename MapType::const_iterator iter) {
    const EntryWithData& entry = iter->data();
    ListGet(entry.prev_).next_ = entry.next_;
    ListGet(entry.next_).prev_ = entry.prev_;
  }

 private:
  MapType map_;
  ListEntry head_;
  size_t max_size_;
  size_t size_;
  CacheCostFunc cache_cost_func_;
  EvictionFunction eviction_func_;
};

}  // namespace utils
}  // namespace olp
