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

#include "ThreadSafeQueue.h"
#include <algorithm>
#include <chrono>
#include <string>

namespace olp {
namespace dataservice {
namespace write {
template <typename T>
ThreadSafeQueue<T>::ThreadSafeQueue() = default;

template <typename T>
bool ThreadSafeQueue<T>::empty() const {
  std::lock_guard<std::mutex> lg{m_mutex};
  return m_vec.empty();
}

template <typename T>
typename ThreadSafeQueue<T>::size_type ThreadSafeQueue<T>::size() const {
  std::lock_guard<std::mutex> lg{m_mutex};
  return m_vec.size();
}

template <typename T>
typename ThreadSafeQueue<T>::value_type& ThreadSafeQueue<T>::front() {
  std::lock_guard<std::mutex> lg{m_mutex};
  return m_vec.front();
}

template <typename T>
const typename ThreadSafeQueue<T>::value_type& ThreadSafeQueue<T>::front()
    const {
  std::lock_guard<std::mutex> lg{m_mutex};
  return m_vec.front();
}

template <typename T>
typename ThreadSafeQueue<T>::value_type& ThreadSafeQueue<T>::back() {
  std::lock_guard<std::mutex> lg{m_mutex};
  return m_vec.back();
}

template <typename T>
const typename ThreadSafeQueue<T>::value_type& ThreadSafeQueue<T>::back()
    const {
  std::lock_guard<std::mutex> lg{m_mutex};
  return m_vec.back();
}

template <typename T>
void ThreadSafeQueue<T>::push(const value_type& val) {
  std::lock_guard<std::mutex> lg{m_mutex};
  m_vec.push_back(val);
}

template <typename T>
void ThreadSafeQueue<T>::push(const value_type& val, size_type maxSize,
                              bool bOverwrite) {
  std::lock_guard<std::mutex> lg{m_mutex};
  if (maxSize == 0) {
    m_vec.clear();
    return;
  }

  if (m_vec.size() >= maxSize) {
    if (bOverwrite) {
      m_vec.erase(std::rotate(std::begin(m_vec),
                              std::begin(m_vec) + m_vec.size() - maxSize + 1,
                              std::end(m_vec)),
                  std::end(m_vec));
    } else {
      return;
    }
  }
  m_vec.push_back(val);
}

template <typename T>
void ThreadSafeQueue<T>::push(value_type&& val) {
  std::lock_guard<std::mutex> lg{m_mutex};
  m_vec.push_back(std::move(val));
}

template <typename T>
void ThreadSafeQueue<T>::push(value_type&& val, size_type maxSize,
                              bool bOverwrite) {
  std::lock_guard<std::mutex> lg{m_mutex};
  if (maxSize == 0) {
    if (bOverwrite) {
      m_vec.clear();
    }
    return;
  }

  if (m_vec.size() >= maxSize) {
    if (bOverwrite) {
      m_vec.erase(std::rotate(std::begin(m_vec),
                              std::begin(m_vec) + m_vec.size() - maxSize + 1,
                              std::end(m_vec)),
                  std::end(m_vec));
    } else {
      return;
    }
  }
  m_vec.emplace_back(std::move(val));
}

template <typename T>
void ThreadSafeQueue<T>::pop() {
  std::lock_guard<std::mutex> lg{m_mutex};
  if (!m_vec.empty()) {
    m_vec.erase(
        std::rotate(std::begin(m_vec), std::begin(m_vec) + 1, std::end(m_vec)),
        std::end(m_vec));
  }
}

template <typename T>
bool ThreadSafeQueue<T>::pop(size_type num) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (num != 0 && m_vec.size() >= num) {
    std::vector<typename std::vector<T>::value_type>(std::begin(m_vec) + num,
                                                     std::end(m_vec))
        .swap(m_vec);
    return true;
  }
  return false;
}

template <typename T>
void ThreadSafeQueue<T>::swap(ThreadSafeQueue& x) noexcept {
  std::lock(m_mutex, x.m_mutex);
  std::lock_guard<std::mutex> lg1(m_mutex, std::adopt_lock);
  std::lock_guard<std::mutex> lg2(x.m_mutex, std::adopt_lock);
  std::swap(m_vec, x.m_vec);
}

template <typename T>
typename ThreadSafeQueue<T>::value_type ThreadSafeQueue<T>::top(
    typename std::vector<T>::size_type offset) const {
  std::lock_guard<std::mutex> lg{m_mutex};
  if (m_vec.size() <= offset) {
    return {};
  }
  return m_vec[offset];
}

template <typename T>
bool ThreadSafeQueue<T>::try_pop(value_type& val) {
  std::lock_guard<std::mutex> lg{m_mutex};
  if (!m_vec.empty()) {
    val = m_vec.front();
    m_vec.erase(
        std::rotate(std::begin(m_vec), std::begin(m_vec) + 1, std::end(m_vec)),
        std::end(m_vec));
    return true;
  }
  return false;
}

template class ThreadSafeQueue<int>;
template class ThreadSafeQueue<std::chrono::system_clock::time_point>;
template class ThreadSafeQueue<std::string>;

}  // namespace write
}  // namespace dataservice
}  // namespace olp
