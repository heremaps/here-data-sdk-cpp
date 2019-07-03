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

#include <utility>

#include "NetworkStream.h"
#include "NetworkStreamBuf_inl.h"

namespace olp {
namespace network {
template <class Storage, template <class, class> class Base>
NetworkStreamBase<Storage, Base>::NetworkStreamBase(std::size_t size_hint,
                                                    std::size_t max_length)
    : base_type(&streambuf_),
      streambuf_(size_hint, default_grow_factor, max_length) {}

template <class Storage, template <class, class> class Base>
NetworkStreamBase<Storage, Base>::NetworkStreamBase(streambuf_type&& buf)
    : base_type(&streambuf_), streambuf_(std::move(buf)) {}

template <class Storage, template <class, class> class Base>
NetworkStreamBase<Storage, Base>::NetworkStreamBase(NetworkStreamBase&& that)
#if BROKEN_STREAMS_IMPL
    : base_type(&streambuf_)
#else
    : base_type(std::forward<base_type>(that))
#endif
      ,
      streambuf_(std::move(that.streambuf_)) {
#if BROKEN_STREAMS_IMPL
  base_type::setstate(that.rdstate());
#else
  base_type::set_rdbuf(&streambuf_);
#endif
}

template <class Storage, template <class, class> class Base>
NetworkStreamBase<Storage, Base>& NetworkStreamBase<Storage, Base>::operator=(
    NetworkStreamBase&& rhs) {
  if (this == &rhs) return *this;
#if BROKEN_STREAMS_IMPL
  base_type::setstate(rhs.rdstate());
#else
  base_type::operator=(std::move(rhs));
#endif
  streambuf_ = std::move(rhs.streambuf_);
  return *this;
}

template <class Storage, template <class, class> class Base>
void NetworkStreamBase<Storage, Base>::swap(NetworkStreamBase& rhs) {
  if (this == &rhs) return;
#if BROKEN_STREAMS_IMPL
  std::ios_base::iostate tmp = base_type::rdstate();
  base_type::setstate(rhs.rdstate());
  rhs.setstate(tmp);
#else
  base_type::swap(rhs);
#endif
  streambuf_.swap(rhs.streambuf_);
}

namespace {
inline bool IsContentLengthHeader(const std::string& key) {
  static constexpr char content_length[] = "content-length";
  if (sizeof(content_length) - 1 != key.size()) return false;
  const char* p = content_length;
  for (const char& c : key) {
    if (*p++ != (0x20 | c)) return false;
  }
  return true;
}

inline bool StrToSize_t(const std::string& value, std::size_t& result) {
  const unsigned char* ptr =
      reinterpret_cast<const unsigned char*>(value.data());
  const unsigned char* end = ptr + value.size();
  constexpr std::size_t base = 10;
  std::size_t result_ = 0;
  for (; end > ptr; ++ptr) {
    std::size_t tmp = *ptr - '0';
    if (9 < tmp) return false;
    tmp += result_ * base;
    if (tmp < result_) return false;
    result_ = tmp;
  }
  result = result_;
  return true;
}
}  // namespace

template <class Storage, template <class, class> class Base>
inline void NetworkStreamBase<Storage, Base>::TryStoragePrepare(
    const std::string& key, const std::string& value) {
  if (IsContentLengthHeader(key)) {
    std::size_t cl;
    if (StrToSize_t(value, cl)) {
      if (!streambuf_.set_desired_size(cl) ||
          !streambuf_.set_size_increment(SIZE_INCREMENT)) {
        this->setstate(std::ios_base::badbit);
      }
    }
  }
}

/*
 * The callbacks rely on the fact that a std::shared_pointer to the stream
 * object is stored in the request context object. This guarantees that "this"
 * won't go out of scope.
 */
template <class Storage, template <class, class> class Base>
Network::HeaderCallback NetworkStreamBase<Storage, Base>::header_func() {
  return [this](const std::string& key, const std::string& value) -> void {
    TryStoragePrepare(key, value);
  };
}

template <class Storage, template <class, class> class Base>
Network::HeaderCallback NetworkStreamBase<Storage, Base>::header_func(
    const Network::HeaderCallback& chain) {
  return
      [this, chain](const std::string& key, const std::string& value) -> void {
        TryStoragePrepare(key, value);
        chain(key, value);
      };
}

}  // namespace network
}  // namespace olp
