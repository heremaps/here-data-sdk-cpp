/*
 * Copyright (C) 2019-2021 HERE Europe B.V.
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

#include <condition_variable>
#include <deque>
#include <mutex>
#include <queue>

#include <olp/core/CoreApi.h>

namespace olp {
namespace thread {

/**
 * @brief A thread-safe container adapter.
 *
 * @tparam T The queue item that should be stored inside the buffer.
 * @tparam Container The type of the underlying container that should mimic
 * the API of `std::queue` that provides the following functions:
 * - `empty()`
 * - `push(T&&)`
 * - `push(const T&)`
 * - `front()`
 * - `pop()`
 */
template <typename T, typename Container>
class CORE_API SyncQueue final {
 public:
  SyncQueue() noexcept;
  ~SyncQueue();

  /// Non-copyable, non-movable
  SyncQueue(const SyncQueue&) = delete;
  /// Non-copyable, non-movable
  SyncQueue& operator=(const SyncQueue&) = delete;
  /// Non-copyable, non-movable
  SyncQueue(SyncQueue&&) = delete;
  /// Non-copyable, non-movable
  SyncQueue& operator=(SyncQueue&&) = delete;

  /**
   * @brief Checks whether this `SyncQueue` instance is empty.
   *
   * @return True if this `SyncQueue` instance is empty; false otherwise.
   */
  bool Empty() const;

  /**
   * @brief Closes this `SyncQueue` instance, deletes all the queued elements,
   * and blocks you from pulling any elements from the queue.
   */
  void Close();

  /**
   * @brief Pulls one element from the `SyncQueue` instance.
   *
   * @param element The element pulled from the queue.
   *
   * @return True if the pull was successful; false if the `SyncQueue` instance
   * was closed. Once closed, the `SyncQueue` instance does not open again.
   */
  bool Pull(T& element);

  /**
   * @brief Forwards the passed element into the `SyncQueue` instance.
   *
   * @param element The rvalue reference to the element.
   */
  void Push(T&& element);

  /**
   * @brief Copies the passed element into the `SyncQueue` instance.
   *
   * @param element The const lvalue reference to the element.
   */
  void Push(const T& element);

 private:
  /// Queue that stores the elements in user defined order.
  Container queue_;
  /// Mutex to protect against race conditions.
  mutable std::mutex mutex_;
  /// Condition to notify sleeping threads waiting for elements.
  std::condition_variable ready_;
  /// Marks this SyncQueue as closed. Threads should not wait anymore
  /// on this SyncQueue as it will not open again once closed.
  bool closed_;
};

/// Alias for First-In, First-Out behaviour.
template <typename T>
using SyncQueueFifo = SyncQueue<T, std::queue<T, std::deque<T>>>;

}  // namespace thread
}  // namespace olp

#include "SyncQueue.inl"
