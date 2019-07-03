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

#include <streambuf>
#include <string>
#include <vector>

// std::numeric_limits<int>::max() is not in all supported implementations a
// constexpr INT_MAX is required
#include <climits>

namespace olp {
namespace network {
/**
 * @brief A generic in-memory stream buffer for use by network clients.
 *
 * The template uses a higher level storage type such as std::string or
 * std::vector for storing data instead of just raw memory as
 * std::basic_stringbuf does. The rationale is that the higher level types are
 * the ones which are actually used inside of network client code and that there
 * is currently no way to construct such types without copying the data.
 *
 * This template makes some assumptions about the underlying storage type.
 * The type must implement the methods:
 * - capacity(), returning the current allocated memory
 * - data(), returning a pointer to raw memory
 * - append(char_type*, size_type), add data, see below
 *
 * Data is appended by calling storage_append. By default it expects append() to
 * be implemented. If this is not the case a specialization is required. Such a
 * specialization is provided for std::vector.
 *
 * Get and put areas both encompass the allocated memory buffer as a whole. They
 * are not views into a larger object. This implicitly limits the size of the
 * memory buffer to INT_MAX which is on almost all systems 2147483647 bytes, way
 * too large for the needs here.
 *
 * The maximum size of allocated memory must be limited when constructing a
 * buffer to avoid unbounded memory consumption. Storage will be allocated,
 * a.k.a reserved, only when the first data packet is written to the buffer.
 *
 */
template <class Storage>
class NetworkStreamBuf
    : public std::basic_streambuf<typename Storage::value_type> {
 public:
  using storage_type = Storage;                    //!< Underlying storage type
  using char_type = typename Storage::value_type;  //!< Must be a of size 1
  using base_type = std::basic_streambuf<char_type>;  //!< Parent type
  using int_type = typename base_type::int_type;      //!< int_type of parent
  using pos_type = typename base_type::pos_type;      //!< pos_type of parent
  using off_type = typename base_type::off_type;      //!< off_type of parent
  using traits_type =
      typename base_type::traits_type;  //!< traits_type of parent
  using streamsize = std::streamsize;   //!< std::streamsize
  using size_type =
      typename storage_type::size_type;  //!< size_type of underlying storage
                                         //!< type
  using ios_base = std::ios_base;        //!< std::ios_base
  static_assert(1 == sizeof(char_type), "char_type must be of size 1");

  enum : size_type {
    MAX_SIZE = INT_MAX,  //!< Maximum memory size of the underlying storage
    MAX_INCR = 10 * 1024 * 1024  //!< Maximum memory increase
  };

 private:
  /**
   * @brief Initialize underlying storage as well as get and put area
   * @param size_hint - desired size of the underlying storage object
   */
  void Init(size_type size_hint);

 public:
  /**
   * @brief NetworkStreamBuf constructor
   * @param size_hint - desired size of the underlying storage object
   * @param incr - number of bytes to reserve if capacity is exceeded
   * @param max_size - maximum memory consumption
   */
  NetworkStreamBuf(size_type size_hint, unsigned int incr, size_type max_size);

  /**
   * @brief NetworkStreamBuf constructor
   * @param size_hint - desired size of the underlying storage object
   * @param factor - grow factor of reserved bytes if capacity is exceeded
   * @param max_size - maximum memory consumption
   */
  NetworkStreamBuf(size_type size_hint, double factor, size_type max_size);

  /**
   * @brief NetworkStreamBuf move constructor
   * NetworkStreamBUf is movable but not copyable.
   */
  NetworkStreamBuf(NetworkStreamBuf&& that) noexcept;

  /**
   * @brief Move assignment operator
   * NetworkStreamBUf is movable but not copyable.
   */
  NetworkStreamBuf& operator=(NetworkStreamBuf&& rhs) noexcept;

  /**
   * @brief swap specialization
   */
  void swap(NetworkStreamBuf& rhs);

  /**
   * @brief Set the desired storage size
   * @param new_desired_size - the new desired size value
   * @return true if desired_size is in the allowed range otherwise false
   */
  bool set_desired_size(size_type new_desired_size);

  /**
   * @brief Set the desired storage size
   * @param new_increment - the new size increment value
   * @return true if new_increment is in the allowed range otherwise false
   * Implicitly set grow_factor_ to zero.
   */
  bool set_size_increment(size_type new_increment);

  /**
   * @brief Set the storage grow factor
   * @param new_factor - the new grow factor value
   * @return true if new_factor is in the allowed range otherwise false
   * Implicitly set incr_ to zero.
   */
  bool set_grow_factor(double new_factor);

  /**
   * @brief Set the maximum storage size
   * @param new_max_size - the new maximum size value
   * @return true if new_max_size is in the allowed range otherwise false
   */
  bool set_max_size(size_type new_max_size);

  /**
   * @brief Return the currently set desired storage size
   */
  size_type desired_size() const { return desired_size_; }

  /**
   * @brief Return the currently set storage size increment
   */
  size_type size_increment() const { return incr_; }

  /**
   * @brief Return the currently set storage grow factor
   */
  double grow_factor() const { return grow_factor_; }

  /**
   * @brief Return the currently set maximum storage size
   */
  size_type max_size() const { return max_size_; }

  /**
   * @brief Return the current capacity
   */
  size_type capacity() const { return storage_off_ + storage_avl_; }

  /**
   * @brief Return the current data size
   */
  size_type size() const { return storage_off_; }

  /**
   * @brief Return a const reference to the underlying storage
   */
  const storage_type& cdata() const { return storage_; }

  /**
   * @brief Return a reference to the underlying storage
   */
  storage_type& data() { return storage_; }

 protected:
  // put area
  //

  /**
   * @brief Implementation of the output stream part
   * @param data - data to append to the underlying storage
   * @param count - number of elements in data
   * Override of std::basic_streambuf::xsputn (§27.6.3.4.5).
   * http://en.cppreference.com/w/cpp/io/basic_streambuf/sputn
   */
  virtual streamsize xsputn(const char_type* data, streamsize count) override;

  /**
   * @brief Implementation of the output stream part
   * @param ch - the character to store in the put area
   * Override of std::basic_streambuf::overflow (§27.6.3.4.5).
   * http://en.cppreference.com/w/cpp/io/basic_streambuf/overflow
   */
  virtual int_type overflow(int_type ch = traits_type::eof()) override;

  // get area
  //

  /**
   * @brief Implementation of the input stream part
   * @param buffer - where to store the data
   * @param count - number of elements to copy out
   * Override of std::basic_streambuf::xsgetn (§27.6.3.4.3).
   * http://en.cppreference.com/w/cpp/io/basic_streambuf/sgetn
   */
  virtual streamsize xsgetn(char_type* buffer, streamsize count) override;

  // positioning
  //

  /**
   * @brief Positioning of get and put area
   * @param off - offset relative to dir
   * @param dir - seek start point
   * @param which - designates the target area ios_base::in or ios_base::out
   * Override of std::basic_streambuf::seekoff (§27.6.3.4.2).
   * http://en.cppreference.com/w/cpp/io/basic_streambuf/pubseekoff
   *
   * This function may be called on the put area only if the resulting position
   * is the same as the current one. E.g. dir is ios_base::cur and off is 0.
   * This is to support std::basic_ostream::tellp.
   */
  virtual pos_type seekoff(off_type off, ios_base::seekdir dir,
                           ios_base::openmode which = ios_base::in) override;

  /**
   * @brief Positioning of get and put area
   * @param pos - new absolute postion
   * @param which - designates the target area ios_base::in or ios_base::out
   * Override of std::basic_streambuf::seekpos (§27.6.3.4.2).
   * http://en.cppreference.com/w/cpp/io/basic_streambuf/pubseekpos
   *
   * This implementation just calls seekoff(pos, std::ios_base::beg, which).
   */
  virtual pos_type seekpos(pos_type pos,
                           ios_base::openmode which = ios_base::in) override;

 private:
  // integer overrun protection
  //

  /**
   * @brief Increment an integer with another integer and check if an overflow
   * occurred
   * @param cur - an integer
   * @param incr - the increment
   * @param result - store the result here
   * @return true if an overflow occurred
   */
  static bool CheckOverflow(size_type cur, size_type incr, size_type& result);

  /**
   * @brief Multiply an integer with a double and check if an integer overflow
   * occurred
   * @param cur - an integer
   * @param factor - the multiplier
   * @param result - store the result here
   * @return true if an overflow occurred
   */
  static bool CheckOverflow(size_type cur, double factor, size_type& result);

  // storage handling
  //

  /**
   * @brief Synchronize the get area with the current state of the underlying
   * storage
   * @param off - current end of stored data
   */
  void StoragegSync(off_type off);

  /**
   * @brief Allocate space in the underlying storage
   * @param size - number of bytes to reserve
   */
  void StorageReserve(size_type size);

  /**
   * @brief Grow allocated storage
   * @param size - bytes
   * @param absolute - Wether size is the new overall size or an increment
   */
  bool StorageGrow(size_type size, bool absolute = false);

  /*
   * @brief Append data
   * @param data - Pointer into a char array
   * @param count - length of data
   */
  void StorageAppend(const char_type* data, size_type count);

  // positioning
  //

  /**
   * @brief Calculate seek offset of the put area from offset value and seek
   * direction
   * @param off - offset value
   * @param dir - seek direction
   */
  off_type SeekpOffset(off_type off, ios_base::seekdir dir) const;

  /**
   * @brief Return number of bytes available in the get area
   */
  size_type gavl() const;

  /**
   * @brief Return current position in the get area
   */
  off_type goff() const;

  /**
   * @brief Calculate seek offset of the get area from offset value and seek
   * direction
   * @param off - offset value
   * @param dir - seek direction
   */
  off_type SeekgOffset(off_type off, ios_base::seekdir dir) const;

  /**
   * @brief Update get area pointers
   * @param off - new stored data size
   */
  void UpdateGetArea(off_type off);

  // maximum capacity of storage_
  size_type max_size_{0};
  // desired size, storage will be allocated on first write
  size_type desired_size_{0};
  // capacity increment
  size_type incr_{0};
  // grow factor
  double grow_factor_{0.0};

  // underlying storage
  storage_type storage_;

  // put area
  //
  // we can't use pbase, pptr and epptr of base_type because sputc is not
  // itself a virtual function and does not call one if there is space in the
  // put area
  //

  // current offset in storage_
  off_type storage_off_ = -1;
  // available space in storage_
  off_type storage_avl_ = -1;
  // invariant: storage_off_ + storage_avl_ == storage_.capacity()

  /**
   * @brief Internal state transfer helper
   */
  struct StateTransfer;
};

/**
 * @brief NetworkStreamBuf swap specialization
 */
template <class Storage>
void swap(NetworkStreamBuf<Storage>& lhs, NetworkStreamBuf<Storage>& rhs) {
  lhs.swap(rhs);
}

/**
 * @brief A std::string backed NetworkStringBuf
 */
using NetworkStringBuf = NetworkStreamBuf<std::string>;

/**
 * @brief A std::vector backed NetworkStringBuf
 */
using NetworkVectorBuf = NetworkStreamBuf<std::vector<char> >;

}  // namespace network
}  // namespace olp
