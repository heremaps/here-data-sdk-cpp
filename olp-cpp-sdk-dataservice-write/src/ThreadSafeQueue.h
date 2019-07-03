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

#include <mutex>
#include <vector>

namespace olp {
namespace dataservice {
namespace write {
template <typename T>
class ThreadSafeQueue {
 public:
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using iterator = typename std::vector<T>::iterator;
  using const_iterator = typename std::vector<T>::const_iterator;
  using size_type = typename std::vector<T>::size_type;

  explicit ThreadSafeQueue();

  bool empty() const;

  size_type size() const;

  value_type& front();

  const value_type& front() const;

  value_type& back();

  const value_type& back() const;

  void push(const value_type& val);

  void push(const value_type& val, size_type maxSize, bool bOverwrite = false);

  void push(value_type&& val);

  void push(value_type&& val, size_type maxSize, bool bOverwrite = false);

  template <class... Args>
  void emplace(Args&&... args) {
    std::lock_guard<std::mutex> lg{m_mutex};
    m_vec.emplace_back(std::forward<Args>(args)...);
  }

  template <class... Args>
  void emplace_count(size_type num, Args&&... args) {
    std::lock_guard<std::mutex> lg{m_mutex};
    for (auto i = size_type{}; i < num; ++i)
      m_vec.emplace_back(std::forward<Args>(args)...);
  }

  void pop();

  bool pop(size_type num);

  // can't use std::swap as mutex isn't moveable, only the vectors are swapped.
  void swap(ThreadSafeQueue& x) noexcept;

  value_type top(typename std::vector<T>::size_type offset =
                     typename std::vector<T>::size_type{}) const;

  bool try_pop(value_type& val);

 private:
  mutable std::mutex m_mutex;
  std::vector<T> m_vec;
};
}  // namespace write
}  // namespace dataservice
}  // namespace olp
