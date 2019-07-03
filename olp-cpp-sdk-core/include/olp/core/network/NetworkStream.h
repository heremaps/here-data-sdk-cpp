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

#include <functional>
#include <istream>
#include <ostream>

#include "Network.h"
#include "NetworkStreamBuf.h"

namespace olp {
namespace network {
/**
 * A (i)ostream implementation for use by network clients.
 *
 * This is meant as a replacement for std::stringstream.
 * The template is based on a NetworkStreamBuf instantiated with template
 * parameter "Storage".
 *
 * The basic idea is to not allocate any storage before the actual size of a
 * network resource is known. It registers a callback with the network protocol
 * handler. This function is called whenever a response header is read. When the
 * "Content-Length" header is encountered its value will be used as the desired
 * size of the stream buffer. Thus only a single memory allocation is needed.
 *
 * The stream must be initialized with a desired size and a maximum limit, or
 * with a separately constructed stream buffer where all limits are already set.
 *
 * It is important that a reasonable maximum is chosen to prevent memory
 * exhaustion. If e.g. payload data averages at around 64 KiB, INT_MAX (= 2GiB)
 * is probably a bad choice.
 *
 * Existing header callbacks can be chained to the one of this template.
 *
 */
template <class Storage,
          template <class, class> class Base = std::basic_ostream>
class NetworkStreamBase
    : public Base<typename Storage::value_type,
                  std::char_traits<typename Storage::value_type> > {
 public:
  using storage_type =
      Storage;  //!< Underlying storage type of NetworkStreamBuf
  using char_type =
      typename Storage::value_type;  //!< Type is restricted by NetworkStreamBuf
  using base_type =
      Base<char_type, std::char_traits<char_type> >;  //!< Parent type
  using streambuf_type =
      NetworkStreamBuf<storage_type>;  //!< NetworkStreamBuf instantiation type
  using size_type =
      typename streambuf_type::size_type;  //!< size_type of streambuf_type
  static_assert(std::is_base_of<
                    std::basic_ostream<char_type, std::char_traits<char_type> >,
                    base_type>::value,
                "Base must be derived from std::basic_ostream");

  enum : size_type {
    /**
     * @brief Size increment set on the stream buffer when a Content-Length
     * header is read Actually there should not be a need to resize the buffer
     * once a Content-Length header is read, but are all webservers out there
     * standard compliant?
     */
    SIZE_INCREMENT = 4096U
  };

  /**
   * @brief NetworkStreamBase constructor, initialize stream buffer from limits
   * @param size_hint - desired size of the stream buffer
   * @param max_length - maximum size of the stream buffer
   * Note that unlike std::stringstream there is no default constructor.
   */
  NetworkStreamBase(std::size_t size_hint, std::size_t max_length);

  /**
   * @brief NetworkStreamBase constructor, initialize stream buffer from rvalue
   * reference
   * @param buf - Rvalue reference to a stream buffer
   */
  NetworkStreamBase(streambuf_type&& buf);

  /**
   * @brief NetworkStreamBase move constructor
   * NetworkStreamBase is movable but not copyable.
   */
  NetworkStreamBase(NetworkStreamBase&& that);

  /**
   * @brief Move assignment operator
   * NetworkStreamBase is movable but not copyable.
   */
  NetworkStreamBase& operator=(NetworkStreamBase&& rhs);

  /**
   * @brief swap specialization
   */
  void swap(NetworkStreamBase& rhs);

  /**
   * @brief Return a const reference to the underlying storage of the stream
   * buffer
   */
  const storage_type& cdata() const { return streambuf_.cdata(); }

  /**
   * @brief Return a reference to the underlying storage of the stream buffer
   */
  storage_type& data() { return streambuf_.data(); }

  /**
   * @brief Return a pointer to the stream buffer
   * Shadow both rdbuf() overloads in std::basic_ios (§27.5.5.3.5).
   * http://en.cppreference.com/w/cpp/io/basic_ios/rdbuf
   */
  streambuf_type* rdbuf() { return &streambuf_; }

  /**
   * @brief Get a lambda to register as header callback
   * Check the header key if it matches Content-Length and set the desired size
   * of the stream buffer.
   */
  Network::HeaderCallback header_func();

  /**
   * @brief Get a lambda to register as header callback
   * @param chain - another call back
   * Check the header key if it matches Content-Length and set the desired size
   * of the stream buffer. For each key value pair chain is called.
   */
  Network::HeaderCallback header_func(const Network::HeaderCallback& chain);

 private:
  /**
   * @brief Try to set the desired size of the stream buffer.
   * @param key - a HTTP header name
   * @param value - a header value
   * Check if key equals Content-Length and if so set the desired size of the
   * stream buffer.
   */
  void TryStoragePrepare(const std::string& key, const std::string& value);

  // the stream buffer
  streambuf_type streambuf_;
};

/**
 * @brief NetworkStreamBase swap specialization
 */
template <class Storage, template <class, class> class Base>
void swap(NetworkStreamBase<Storage, Base>& lhs,
          NetworkStreamBase<Storage, Base>& rhs) {
  lhs.swap(rhs);
}

/**
 * @brief Base type for output streams
 */
template <class Storage>
using NetworkOStream = NetworkStreamBase<Storage, std::basic_ostream>;

/**
 * @brief Base type for input/output streams
 */
template <class Storage>
using NetworkIOStream = NetworkStreamBase<Storage, std::basic_iostream>;

/**
 * @brief std::string backed output stream
 */
using NetworkStringOStream = NetworkOStream<std::string>;

/**
 * @brief std::string backed input/output stream
 */
using NetworkStringIOStream = NetworkIOStream<std::string>;

/**
 * @brief std::vector backed output stream
 */
using NetworkVectorOStream = NetworkOStream<std::vector<char> >;

/**
 * @brief std::vector backed input/output stream
 */
using NetworkVectorIOStream = NetworkIOStream<std::vector<char> >;

}  // namespace network
}  // namespace olp
