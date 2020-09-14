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

namespace olp {

namespace thread {

#include <algorithm>
#include <cstdint>  // for std::uint32_t
#include <deque>
#include <functional>

/// FIFO aware priority queue with removal support
template <class T, class COMPARE = std::less<T> >
class PriorityQueueExtended {
  /// Nested helper class to make an object distinguishable
  struct DistinguishableObject {
    std::uint32_t id;
    T obj;

    template <class... Args>
    DistinguishableObject(std::uint32_t id, Args... args)
        : id(id), obj(args...) {}
    DistinguishableObject(std::uint32_t id, const T& obj) : id(id), obj(obj) {}
    DistinguishableObject(std::uint32_t id, T&& obj) : id(id), obj(obj) {}
  };

  /// Comparator for distinguishable objects
  class Compare : public std::binary_function<DistinguishableObject,
                                              DistinguishableObject, bool> {
    COMPARE compare_;

   public:
    Compare(const COMPARE& compare) : compare_(compare) {}

    bool operator()(const DistinguishableObject& lhs,
                    const DistinguishableObject& rhs) const {
      bool rc = compare_(lhs.obj, rhs.obj);  // check less
      if (!rc) {
        rc = !compare_(rhs.obj, lhs.obj);  // check greater
        if (rc)                            // both objects are considered equal
        {
          rc = lhs.id > rhs.id;
        }
      }

      return rc;
    }
  };

  std::deque<DistinguishableObject> container_;  ///< internal queue container
  Compare compare_;                              ///< prorized queue comparator
  std::uint32_t next_id_ = 0;                    ///< id counter

 public:
  /// Constructor
  explicit PriorityQueueExtended(const COMPARE& compare = COMPARE());

  /// Check for queue emptiness
  bool empty() const;

  /// Push copy of an object in priority queue
  void push(const T& value);

  /// Push a moveable object in priority queue
  void push(T&& value);

  /// Getter for front element in queue (with highest priority)
  T& front();

  /// Const getter for front element in queue (with highest priority)
  const T& front() const;

  /// Remove top element from queue
  void pop();

 private:
  /// Getter for next object id
  std::uint32_t get_next_id();
};

template <class T, class COMPARE>
PriorityQueueExtended<T, COMPARE>::PriorityQueueExtended(const COMPARE& compare)
    : compare_(compare) {}

// -------------------------------------------------------------------------------------------------

template <class T, class COMPARE>
bool PriorityQueueExtended<T, COMPARE>::empty() const {
  return container_.empty();
}

// -------------------------------------------------------------------------------------------------

template <class T, class COMPARE>
void PriorityQueueExtended<T, COMPARE>::push(const T& value) {
  container_.push_back(DistinguishableObject(get_next_id(), value));
  std::push_heap(container_.begin(), container_.end(), compare_);
}

// -------------------------------------------------------------------------------------------------

template <class T, class COMPARE>
void PriorityQueueExtended<T, COMPARE>::push(T&& value) {
  container_.push_back(DistinguishableObject(get_next_id(), value));
  std::push_heap(container_.begin(), container_.end(), compare_);
}

// -------------------------------------------------------------------------------------------------

template <class T, class COMPARE>
T& PriorityQueueExtended<T, COMPARE>::front() {
  return container_.front().obj;
}

// -------------------------------------------------------------------------------------------------

template <class T, class COMPARE>
const T& PriorityQueueExtended<T, COMPARE>::front() const {
  return container_.front().obj;
}

// -------------------------------------------------------------------------------------------------

template <class T, class COMPARE>
void PriorityQueueExtended<T, COMPARE>::pop() {
  if (!container_.empty()) {
    std::pop_heap(container_.begin(), container_.end(), compare_);
    container_.pop_back();
  }
}

// -------------------------------------------------------------------------------------------------

template <class T, class COMPARE>
std::uint32_t PriorityQueueExtended<T, COMPARE>::get_next_id() {
  if (container_.empty()) {
    next_id_ = 0;
  }

  // TODO add renumbering of ids in case of overflow (when next_id_ equal
  // UMAX_INT)
  return next_id_++;
}

}  // namespace thread
}  // namespace olp
