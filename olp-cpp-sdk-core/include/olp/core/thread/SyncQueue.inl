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

namespace olp {
namespace thread {

template <typename T, typename Container>
inline SyncQueue<T, Container>::SyncQueue() noexcept : closed_(false) {}

template <typename T, typename Container>
inline SyncQueue<T, Container>::~SyncQueue() {
  Close();
}

template <typename T, typename Container>
inline bool SyncQueue<T, Container>::Empty() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return queue_.empty();
}

template <typename T, typename Container>
inline void SyncQueue<T, Container>::Close() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    closed_ = true;
    queue_ = Container();
  }
  ready_.notify_all();
}

template <typename T, typename Container>
inline bool SyncQueue<T, Container>::Pull(T& element) {
  std::unique_lock<std::mutex> lock(mutex_);
  ready_.wait(lock, [this]() { return !queue_.empty() || closed_; });

  if (closed_) {
    return false;
  }

  element = std::move(queue_.front());
  queue_.pop();
  return true;
}

template <typename T, typename Container>
inline void SyncQueue<T, Container>::Push(T&& element) {
  {
    std::lock_guard<std::mutex> lock(mutex_);

    // Do not push on a closed queue
    if (closed_) {
      return;
    }

    queue_.push(std::forward<T>(element));
  }
  ready_.notify_one();
}

template <typename T, typename Container>
inline void SyncQueue<T, Container>::Push(const T& element) {
  {
    std::lock_guard<std::mutex> lock(mutex_);

    // Do not push on a closed queue
    if (closed_) {
      return;
    }

    queue_.push(element);
  }
  ready_.notify_one();
}

}  // namespace thread
}  // namespace olp
