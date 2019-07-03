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

#include <algorithm>
#include <cstring>  // memcpy
#include <utility>

#include "NetworkStreamBuf.h"

#if defined(__linux) && defined(__GNUC__) && __GNUC__ < 5
#define BROKEN_STREAMS_IMPL 1
#else
#define BROKEN_STREAMS_IMPL 0
#endif

namespace olp {
namespace network {
namespace {
constexpr double default_grow_factor = 1.5;
};

// ctors
//
template <class Storage>
inline void NetworkStreamBuf<Storage>::Init(size_type size_hint) {
  storage_off_ = 0;
  desired_size_ = std::min(size_hint, max_size_);
  StorageReserve(0);
}

template <class Storage>
NetworkStreamBuf<Storage>::NetworkStreamBuf(size_type size_hint,
                                            unsigned int incr,
                                            size_type max_size)
    : base_type(),
      max_size_(std::min(max_size, (size_type)MAX_SIZE)),
      incr_(std::min((size_type)incr, (size_type)MAX_INCR)),
      grow_factor_(0) {
  Init(size_hint);
}

template <class Storage>
NetworkStreamBuf<Storage>::NetworkStreamBuf(size_type size_hint, double factor,
                                            size_type max_size)
    : base_type(),
      max_size_(std::min(max_size, (size_type)MAX_SIZE)),
      incr_(0),
      grow_factor_(factor) {
  Init(size_hint);
}

// move and swap
//
template <class Storage>
NetworkStreamBuf<Storage>::NetworkStreamBuf(NetworkStreamBuf&& that) noexcept
#if BROKEN_STREAMS_IMPL
    : base_type()
#else
    : base_type(static_cast<const base_type&>(that))
#endif
      ,
      max_size_(that.max_size_),
      desired_size_(that.desired_size_),
      incr_(that.incr_),
      grow_factor_(that.grow_factor_),
      storage_(std::move(that.storage_)) {
  StateTransfer l(that, this);
  StateTransfer r(&that);
}

template <class Storage>
NetworkStreamBuf<Storage>& NetworkStreamBuf<Storage>::operator=(
    NetworkStreamBuf&& rhs) noexcept {
  if (this == &rhs) return *this;
  StateTransfer l(rhs, this);
  StateTransfer r(&rhs);
#if !BROKEN_STREAMS_IMPL
  base_type::operator=(static_cast<const base_type&>(rhs));
#endif
  max_size_ = rhs.max_size_;
  desired_size_ = rhs.desired_size_;
  incr_ = rhs.incr_;
  grow_factor_ = rhs.grow_factor_;
  storage_ = std::move(rhs.storage_);
  return *this;
}

template <class Storage>
void NetworkStreamBuf<Storage>::swap(NetworkStreamBuf& rhs) {
  if (this == &rhs) return;
  StateTransfer l(rhs, this);
  StateTransfer r(*this, &rhs);
#if !BROKEN_STREAMS_IMPL
  base_type::swap(static_cast<base_type&>(rhs));
#endif
  std::swap(max_size_, rhs.max_size_);
  std::swap(desired_size_, rhs.desired_size_);
  std::swap(incr_, rhs.incr_);
  std::swap(grow_factor_, rhs.grow_factor_);
  std::swap(storage_, rhs.storage_);
}

// storage modifiers
//
template <class Storage>
bool NetworkStreamBuf<Storage>::set_desired_size(size_type new_desired_size) {
  if (max_size_ < new_desired_size) return false;
  desired_size_ = new_desired_size;
  return true;
}

template <class Storage>
bool NetworkStreamBuf<Storage>::set_size_increment(size_type new_increment) {
  if ((size_type)MAX_INCR < new_increment) return false;
  incr_ = new_increment;
  grow_factor_ = .0;
  return true;
}

template <class Storage>
bool NetworkStreamBuf<Storage>::set_grow_factor(double new_factor) {
  if (1. >= new_factor) return false;
  grow_factor_ = new_factor;
  incr_ = 0;
  return true;
}

template <class Storage>
bool NetworkStreamBuf<Storage>::set_max_size(size_type new_max_size) {
  if (MAX_SIZE < new_max_size) return false;
  max_size_ = new_max_size;
  return true;
}

// virtual functions put area
//
template <class Storage>
std::streamsize NetworkStreamBuf<Storage>::xsputn(const char_type* data,
                                                  streamsize count) {
  if (0 >= count) return 0;
  if (!storage_off_ && desired_size_ > (size_type)storage_avl_ &&
      desired_size_ > (size_type)count && !StorageGrow(desired_size_, true)) {
    return traits_type::eof();
  }
  if (storage_avl_ < count && !StorageGrow(count - storage_avl_)) {
    return traits_type::eof();
  }
  StorageAppend(data, count);
  storage_avl_ -= count;
  storage_off_ += count;
  UpdateGetArea(goff());
  return count;
}

template <class Storage>
typename NetworkStreamBuf<Storage>::int_type
NetworkStreamBuf<Storage>::overflow(int_type ch) {
  // check if called from sputc
  if (!traits_type::eq_int_type(ch, traits_type::eof()) &&
      1 == xsputn(reinterpret_cast<char_type*>(&ch), 1)) {
    return ch;
  }
  return traits_type::eof();
}

// virtual functions get area
//
template <class Storage>
std::streamsize NetworkStreamBuf<Storage>::xsgetn(char_type* buffer,
                                                  streamsize count) {
  if (0 >= count) return 0;
  count = std::min((size_type)count, gavl());
  if (!count) return traits_type::eof();
  std::memcpy(buffer, this->gptr(), (size_type)count * sizeof(char_type));
  this->gbump(static_cast<int>(count));
  return count;
}

// virtual functions positioning
//
template <class Storage>
typename NetworkStreamBuf<Storage>::pos_type NetworkStreamBuf<Storage>::seekoff(
    off_type off, ios_base::seekdir dir, ios_base::openmode which) {
  if ((which & ios_base::out) == ios_base::out) {
    off_type new_off = SeekpOffset(off, dir);
    // no support for positioning of the put area
    // only std::ostream::tellp is supported
    return new_off == storage_off_ ? new_off : -1;
  }
  if ((which & ios_base::in) == ios_base::in) {
    off_type new_off = SeekgOffset(off, dir);
    if (0 <= new_off && new_off <= storage_off_) {
      UpdateGetArea(new_off);
      return new_off;
    }
  }
  return -1;
}

template <class Storage>
typename NetworkStreamBuf<Storage>::pos_type NetworkStreamBuf<Storage>::seekpos(
    pos_type pos, ios_base::openmode which) {
  return seekoff(pos, ios_base::beg, which);
}

// integer overrun protection
//
template <class Storage>
inline bool NetworkStreamBuf<Storage>::CheckOverflow(size_type cur,
                                                     size_type incr,
                                                     size_type& result) {
  result = cur + incr;
  return result < cur;
}

template <class Storage>
inline bool NetworkStreamBuf<Storage>::CheckOverflow(size_type cur,
                                                     double factor,
                                                     size_type& result) {
  result = cur * factor;
  return result < cur;
}

// storage handling
//
template <class Storage>
inline void NetworkStreamBuf<Storage>::StoragegSync(off_type off) {
  char_type* beg = const_cast<char_type*>(storage_.data());
  this->setg(beg, beg + off, beg + storage_off_);
}

template <class Storage>
inline void NetworkStreamBuf<Storage>::StorageReserve(size_type size) {
  if (size) storage_.reserve(size);  // size may be 0 on construction
  storage_avl_ = storage_.capacity() - storage_off_;
  StoragegSync(goff());
}

template <class Storage>
inline bool NetworkStreamBuf<Storage>::StorageGrow(size_type size,
                                                   bool absolute) {
  size_type cur_size = storage_off_ + storage_avl_;
  size_type new_size;
  if (absolute) {
    if (max_size_ < size || cur_size >= size) return false;
    new_size = size;
  } else {
    size_type req_size;
    if (CheckOverflow(cur_size, size, req_size) || max_size_ < req_size)
      return false;
    if (incr_) {
      if (CheckOverflow(cur_size, incr_, new_size)) new_size = 0;
    } else {
      if (CheckOverflow(cur_size, grow_factor_, new_size)) new_size = 0;
    }
    if (max_size_ < new_size || new_size < req_size) new_size = req_size;
  }
  StorageReserve(new_size);
  return true;
}

template <class Storage>
inline void NetworkStreamBuf<Storage>::StorageAppend(const char_type* data,
                                                     size_type count) {
  storage_.append(data, count);
}

// positioning: put area
//
template <class Storage>
typename NetworkStreamBuf<Storage>::off_type inline NetworkStreamBuf<
    Storage>::SeekpOffset(off_type off, ios_base::seekdir dir) const {
  if (ios_base::beg == dir) return off;
  if (ios_base::cur == dir) return storage_off_ + off;
  return storage_off_ + storage_avl_ + off;
}

// positioning: get area
//
template <class Storage>
inline typename NetworkStreamBuf<Storage>::size_type
NetworkStreamBuf<Storage>::gavl() const {
  return this->egptr() - this->gptr();
}

template <class Storage>
inline typename NetworkStreamBuf<Storage>::off_type
NetworkStreamBuf<Storage>::goff() const {
  return this->gptr() - this->eback();
}

template <class Storage>
typename NetworkStreamBuf<Storage>::off_type inline NetworkStreamBuf<
    Storage>::SeekgOffset(off_type off, ios_base::seekdir dir) const {
  if (ios_base::beg == dir) return off;
  if (ios_base::cur == dir) return goff() + off;
  return storage_off_ + off;
}

template <class Storage>
inline void NetworkStreamBuf<Storage>::UpdateGetArea(off_type off) {
  char_type* beg = this->eback();
  this->setg(beg, beg + off, beg + storage_off_);
}

// specializations
//
template <>
inline void NetworkVectorBuf::StorageAppend(const char_type* s,
                                            size_type count) {
#if defined(__linux) && defined(__GNUC__) && __GNUC__ == 4 && __GNUC_MINOR__ < 9
  storage_.insert(storage_.end(), s, s + count);
#else
  storage_.insert(storage_.cend(), s, s + count);
#endif
}

/**
 * @brief Internal RAII based state transfer helper
 */
template <class Storage>
struct NetworkStreamBuf<Storage>::StateTransfer {
  /**
   * @brief StateTransfer move state from source object to destination object
   * @param src - the sorce object
   * @param dst - the destination object
   */
  StateTransfer(const NetworkStreamBuf<Storage>& src,
                NetworkStreamBuf<Storage>* dst)
      : g_off_(src.goff()),
        s_off_(src.storage_off_),
        s_avl_(src.storage_avl_),
        dst_(dst) {}

  /**
   * @brief StateTransfer reset state of destination object
   * @param dst - the destination object
   */
  StateTransfer(NetworkStreamBuf<Storage>* dst)
      : g_off_(0), s_off_(-1), s_avl_(-1), dst_(dst) {}

  /**
   * @brief StateTransfer the destructor actually transfers state
   */
  ~StateTransfer() {
    if (0 > s_off_) {
      dst_->storage_off_ = 0;
      dst_->storage_avl_ = dst_->storage_.capacity();
    } else {
      dst_->storage_off_ = s_off_;
      dst_->storage_avl_ = s_avl_;
    }
    dst_->StoragegSync(g_off_);
  }

 private:
  off_type g_off_;
  off_type s_off_;
  off_type s_avl_;
  NetworkStreamBuf<Storage>* dst_;
};

}  // namespace network
}  // namespace olp
