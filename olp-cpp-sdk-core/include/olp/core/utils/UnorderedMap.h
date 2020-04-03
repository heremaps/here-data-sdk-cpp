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

#pragma once

#include <algorithm>
#include <climits>
#include <type_traits>
#include <vector>

#include <olp/core/utils/EmptyBaseClassOptimizer.h>

namespace olp {
namespace utils {
template <typename T>
void reserve(T&, size_t) {
  /* do nothing for all other containers */
}

template <typename T>
void reserve(std::vector<T>& container, size_t capacity) {
  container.reserve(capacity);
}

struct NoData {
  bool operator==(const NoData) const { return true; }
  bool operator!=(const NoData data) const { return !operator==(data); }
};

/**
 * A simple hash table implementation for mapping values to data.
 * Usually an order of magnitude faster than std::unordered_map, since it avoids
 * copious amounts of memory allocations It uses an internal array for the hash
 * as well as the object storage. Memory is only free'd on clear It does not
 * support multiple mappings to the same key. It uses chaining to store elements
 * non-const function invalidate iterators (in contrast to std::unordered_map)
 *
 * For advanced use cases: Indices in the begin/end list are stable unless erase
 * is called Erase has also a version that allows the user to be informed about
 * index changes
 * @tparam Key_ Key used to access the hash map
 * @tparam Index_ Index type used internally in the hash map. Make sure that it
 * supports the amount of elements you want to insert
 * @tparam Data_ Additional Data associated with each entry
 * @tparam Hash_ Hash function used to map Key_ to uint32_t
 */
template <typename Key_, typename Data_ = NoData, typename Index_ = uint32_t,
          class Hash_ = std::hash<Key_>,
          template <typename, typename> class Cont_ = std::vector,
          template <typename> class Alloc_ = std::allocator>
class UnorderedMap {
  template <typename T>
  using Cont = Cont_<T, Alloc_<T> >;

 public:
  typedef Key_ Key;
  typedef Data_ Data;
  typedef Hash_ Hash;
  typedef Index_ Index;

  class Entry : private EmptyBaseClassOptimizer<Data_> {
    friend class UnorderedMap;

    Key key_;

    Index next_;

   public:
    template <typename... Args>
    Entry(Key key, Args&&... args)
        : EmptyBaseClassOptimizer<Data_>(std::forward<Args>(args)...),
          key_(std::move(key)),
          next_(kSentinel) {}

    Entry() : key_(), next_(kSentinel) {}

    const Key& key() const { return key_; }

    // provide access to underlying user data
    using EmptyBaseClassOptimizer<Data_>::data;
  };

 public:
  typedef typename Cont<Entry>::iterator iterator;
  typedef typename Cont<Entry>::const_iterator const_iterator;

 public:
  /** Constructs a hash table with given size. May have to reallocate if
   * exceeding this size */
  explicit UnorderedMap(size_t numEntries = 1024,
                        const Alloc_<Entry>& alloc = Alloc_<Entry>());

  explicit UnorderedMap(std::initializer_list<std::pair<Key, Data> > values,
                        const Alloc_<Entry>& alloc = Alloc_<Entry>());

  /** Puts an entry to a hash table. Must not yet exist in the hashmap */
  template <typename KeyArg, typename T = Data>
  iterator Insert(KeyArg&& key, T&& data = T());

  template <typename KeyArg, typename... Args>
  std::pair<iterator, bool> TryEmplace(KeyArg&& key, Args&&... args);

  template <typename KeyArg, typename T>
  std::pair<iterator, bool> InsertOrAssign(KeyArg&& key, T&& data);

  iterator begin();
  iterator end();

  const_iterator begin() const;
  const_iterator end() const;

  /** Gets the entry corresponding to a given key. ASSERTs when not found*/
  Data& At(const Key& key);
  const Data& At(const Key& key) const;

  Data& operator[](const Key& key);

  iterator Find(const Key& key);
  const_iterator Find(const Key& key) const;

  Index Count(const Key& key) const;

  /* removes all entries, but keeps capacity around for re-use */
  void Clear();

  /* reserve enough capacity for the required number of elements*/
  void Reserve(size_t Size);

  /* returns the amount of elements erased (0 or 1)
   * May move other stored items, invalidating their iterators
   */
  size_t Erase(const Key& key);

  /* Version of erase that tells the user which items changed their
   * index/position in the being()/end() list
   */
  template <typename F>
  size_t Erase(const Key& key, F&& remap_callback);

  size_t Size() const;
  bool Empty() const;

  // reports the amount of memory the UnorderedMap allocated
  size_t SizeInBytes() const {
    return storage_.capacity() * sizeof(Index) +
           entries_.capacity() * sizeof(Entry);
  }

 private:
  void Init(size_t Size);
  Index GetIndex(const Key& key) const;
  Index FindElement(const Key& key) const;

  void Grow();

  void Rehash(size_t new_size);

  template <typename KeyArg, typename... Args>
  typename Cont<Entry>::iterator EmplaceBack(KeyArg&& key, Args&&... args);

  // helper for constructing a key if required, or forwarding it otherwise
  template <
      typename KeyArg,
      typename = typename std::enable_if<std::is_same<
          Key, typename std::remove_reference<KeyArg>::type>::value>::type>
  KeyArg&& ConvertToKey(KeyArg&& key) {
    return std::forward<KeyArg>(key);
  }

  template <
      typename KeyArg,
      typename = typename std::enable_if<!std::is_same<
          Key, typename std::remove_reference<KeyArg>::type>::value>::type>
  Key ConvertToKey(KeyArg&& key) {
    return Key(std::forward<KeyArg>(key));
  }

  size_t RoundUpToPowerOf2(size_t v) {
    v--;
    for (size_t i = 1; i < sizeof(v) * CHAR_BIT; i *= 2) {
      v |= v >> i;
    }
    return ++v;
  }

 private:
  Hash hash_fun_;
  Cont<Index> storage_;
  Cont<Entry> entries_;

  size_t mask_;

  static constexpr Index kSentinel = std::numeric_limits<Index>::max();
};

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
Index UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::FindElement(
    const Key& key) const {
  Index index = GetIndex(key);
  Index position = storage_[index];
  while (position != kSentinel && !(entries_[position].key() == key)) {
    position = entries_[position].next_;
  }

  return position;
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
Data& UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::At(const Key& key) {
  Index position = FindElement(key);

  return entries_[position].data();
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
const Data& UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::At(
    const Key& key) const {
  Index position = FindElement(key);

  return entries_[position].data();
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
Data& UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::operator[](
    const Key& key) {
  return TryEmplace(key).first->data();
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
typename UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::iterator
UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::Find(const Key& key) {
  const Index position = FindElement(key);
  if (position != kSentinel) {
    return entries_.begin() + position;
  }
  return end();
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
typename UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::const_iterator
UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::Find(const Key& key) const {
  const Index position = FindElement(key);
  if (position != kSentinel) {
    return entries_.begin() + position;
  }
  return end();
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
Index UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::Count(
    const Key& key) const {
  return FindElement(key) != kSentinel ? 1u : 0u;
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
Index UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::GetIndex(
    const Key& key) const {
  return static_cast<Index>(Hash()(key) & mask_);
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::UnorderedMap(
    size_t numEntries, const Alloc<Entry>& alloc)
    : storage_(alloc), entries_(alloc) {
  utils::reserve(entries_, numEntries);
  Init(numEntries);
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::UnorderedMap(
    std::initializer_list<std::pair<Key, Data> > values,
    const Alloc<Entry>& alloc)
    : UnorderedMap(values.size(), alloc) {
  for (const std::pair<Key, Data>& value : values) {
    TryEmplace(value.first, value.second);
  }
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
size_t UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::Size() const {
  return entries_.size();
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
bool UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::Empty() const {
  return Size() == 0;
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
void UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::Reserve(size_t size) {
  // let's ensure size is power of 2 and multiply by 2 for load factor 0.5
  size = 2 * RoundUpToPowerOf2(size);
  if (size > entries_.size()) {
    Rehash(size);
  }
  utils::reserve(entries_, size);
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
void UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::Init(size_t size) {
  // let's ensure size is power of 2 and multiply by 2 for load factor 0.5
  size = 2 * RoundUpToPowerOf2(size);
  size = std::max(size, static_cast<size_t>(4u));

  mask_ = size - 1;

  entries_.clear();

  auto sentinel = kSentinel;
  storage_.clear();
  storage_.resize(size, sentinel);
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
void UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::Rehash(
    size_t new_size) {
  // resize the hash table
  mask_ = new_size - 1;

  // reset hash table
  auto sentinel = kSentinel;
  storage_.clear();
  storage_.resize(new_size, sentinel);

  // move entries from old hash table to new one
  for (Index i = 0; i < entries_.size(); ++i) {
    // clear next pointer
    entries_[i].next_ = kSentinel;

    // find empty slot with chaining
    Index index = GetIndex(entries_[i].key());
    Index* pointer = &storage_[index];
    while (*pointer != kSentinel) {
      pointer = &entries_[*pointer].next_;
    }
    *pointer = i;
  }
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
void UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::Grow() {
  // load factor 0.5 maximum
  if (entries_.size() * 2 <= storage_.size()) {
    return;
  }

  // resize the hash table
  size_t new_size = storage_.size() * 2;
  Rehash(new_size);
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
template <typename KeyArg, typename T>
typename UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::iterator
UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::Insert(KeyArg&& key,
                                                          T&& data) {
  // Convert key to Key if required, otherwise just reference it
  auto&& key_ref = ConvertToKey(std::forward<KeyArg>(key));

  Grow();

  const Index index = GetIndex(key_ref);
  auto pos = EmplaceBack(std::forward<decltype(key_ref)>(key_ref),
                         std::forward<T>(data));

  // find empty slot with chaining
  Index* pointer = &storage_[index];
  while (*pointer != kSentinel) {
    pointer = &entries_[*pointer].next_;
  }
  *pointer = static_cast<Index>(pos - entries_.begin());

  return pos;
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
template <typename KeyArg, typename T>
std::pair<typename UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::iterator,
          bool>
UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::InsertOrAssign(KeyArg&& key,
                                                                  T&& data) {
  auto result = TryEmplace(std::forward<KeyArg>(key), data);
  if (!result.second) {
    result.first->data() = std::forward<T>(data);
  }
  return result;
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
template <typename KeyArg, typename... Args>
std::pair<typename UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::iterator,
          bool>
UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::TryEmplace(KeyArg&& key,
                                                              Args&&... args) {
  // grow first, otherwise we cannot reuse the index
  Grow();

  // Convert key to Key if required, otherwise just reference it
  auto&& key_ref = ConvertToKey(std::forward<KeyArg>(key));

  // find slot with chaining
  Index index = GetIndex(key_ref);
  Index* pointer = &storage_[index];
  while (*pointer != kSentinel) {
    if (entries_[*pointer].key() == key_ref) {
      return {entries_.begin() + *pointer, false};
    }
    pointer = &entries_[*pointer].next_;
  }

  auto pos = EmplaceBack(std::forward<decltype(key_ref)>(key_ref),
                         std::forward<Args>(args)...);

  // re-use index we calculated earlier
  // we cannot re-use pointer, since entries_ might have moved (push_back)
  pointer = &storage_[index];
  while (*pointer != kSentinel) {
    pointer = &entries_[*pointer].next_;
  }
  *pointer = static_cast<Index>(pos - entries_.begin());
  return {pos, true};
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
size_t UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::Erase(
    const Key& key) {
  return Erase(key, [](Index, Index) {});
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
template <typename F>
size_t UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::Erase(
    const Key& key, F&& remap_callback) {
  // find slot with chaining
  Index index = GetIndex(key);
  Index* pointer = &storage_[index];
  while (*pointer != kSentinel) {
    if (entries_[*pointer].key() == key) {
      Index deleted_item = *pointer;
      // remove element from linked list
      *pointer = entries_[*pointer].next_;
      if (deleted_item != entries_.size() - 1) {
        // swap with last element
        // move data of last element
        Index old_position = static_cast<Index>(entries_.size() - 1);
        entries_[deleted_item].~Entry();
        new (&entries_[deleted_item]) Entry(std::move(entries_[old_position]));
        // move list pointers to last element
        Index* moved_item = &storage_[GetIndex(entries_[deleted_item].key())];
        while (*moved_item != old_position) {
          moved_item = &entries_[*moved_item].next_;
        }
        // some classes build more complex logic on top of the UnorderedMaps
        // internal structure, inform them of moved data
        remap_callback(*moved_item, deleted_item);
        *moved_item = deleted_item;
      }
      entries_.pop_back();
      return 1;
    }
    pointer = &entries_[*pointer].next_;
  }
  return 0;
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
template <typename KeyArg, typename... Args>
typename UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::iterator
UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::EmplaceBack(KeyArg&& key,
                                                               Args&&... args) {
  entries_.emplace_back(std::forward<KeyArg>(key), std::forward<Args>(args)...);
  return entries_.end() - 1;
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
typename UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::iterator
UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::begin() {
  return entries_.begin();
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
typename UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::iterator
UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::end() {
  return entries_.end();
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
typename UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::const_iterator
UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::begin() const {
  return entries_.begin();
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
typename UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::const_iterator
UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::end() const {
  return entries_.end();
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
void UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>::Clear() {
  Init(entries_.size());
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
bool operator==(const UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>& lhs,
                const UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>& rhs) {
  if (lhs.Size() != rhs.Size()) {
    return false;
  }

  for (auto lhs_it = lhs.begin(); lhs_it != lhs.end(); ++lhs_it) {
    auto rhs_it = rhs.Find(lhs_it->key());
    if (rhs_it == rhs.end() || lhs_it->data() != rhs_it->data()) {
      return false;
    }
  }

  return true;
}

template <typename Key, typename Data, typename Index, typename Hash,
          template <typename, typename> class Cont,
          template <typename> class Alloc>
bool operator!=(const UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>& lhs,
                const UnorderedMap<Key, Data, Index, Hash, Cont, Alloc>& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace utils
}  // namespace olp
