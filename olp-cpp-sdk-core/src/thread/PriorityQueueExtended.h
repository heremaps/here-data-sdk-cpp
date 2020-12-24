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
#include <cstdint>
#include <deque>

#include <olp/core/porting/make_unique.h>

namespace olp {
namespace thread {

/// FIFO aware priority queue with removal support
template <class T, class COMPARE = std::less<T> >
class PriorityQueueExtended {
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
  class PriorityQueueExtendedImpl;
  std::unique_ptr<PriorityQueueExtendedImpl> impl_;
};

template <class T, class COMPARE>
class PriorityQueueExtended<T, COMPARE>::PriorityQueueExtendedImpl {
 public:
  /// Constructor
  explicit PriorityQueueExtendedImpl(const COMPARE& compare = COMPARE());

  /// Check for queue emptiness
  bool Empty() const;

  /// Push copy of an object in priority queue
  void Push(const T& value);

  /// Push a moveable object in priority queue
  void Push(T&& value);

  /// Getter for front element in queue (with highest priority)
  T& Front();

  /// Const getter for front element in queue (with highest priority)
  const T& Front() const;

  /// Remove top element from queue
  void Pop();

 private:
  /// Nested helper class to make an object distinguishable
  struct DistinguishableObject {
    template <class... Args>
    DistinguishableObject(std::uint32_t id, Args... args)
        : id(id), obj(args...) {}
    DistinguishableObject(std::uint32_t id, const T& obj) : id(id), obj(obj) {}
    DistinguishableObject(std::uint32_t id, T&& obj)
        : id(id), obj(std::move(obj)) {}

    std::uint32_t id;
    T obj;
  };

  /// Comparator for distinguishable objects
  class Compare {
   public:
    Compare(const COMPARE& compare) : compare_(compare) {}

    bool operator()(const DistinguishableObject& lhs,
                    const DistinguishableObject& rhs) const {
      return (compare_(lhs.obj, rhs.obj) ||
              (!compare_(rhs.obj, lhs.obj) && lhs.id > rhs.id));
    }

   private:
    COMPARE compare_;
  };

  /// Getter for next object id. Id is used to keep equal objects FIFO order.
  std::uint32_t GetNextId();

  /// internal queue container
  std::deque<DistinguishableObject> container_;
  /// prorized queue comparator
  Compare compare_;
  /// id counter
  std::uint32_t next_id_ = 0;
};

template <class T, class COMPARE>
PriorityQueueExtended<T, COMPARE>::PriorityQueueExtended(const COMPARE& compare)
    : impl_(std::make_unique<PriorityQueueExtendedImpl>(compare)) {}

template <class T, class COMPARE>
bool PriorityQueueExtended<T, COMPARE>::empty() const {
  return impl_->Empty();
}

template <class T, class COMPARE>
void PriorityQueueExtended<T, COMPARE>::push(const T& value) {
  impl_->Push(value);
}

template <class T, class COMPARE>
void PriorityQueueExtended<T, COMPARE>::push(T&& value) {
  impl_->Push(std::move(value));
}

template <class T, class COMPARE>
T& PriorityQueueExtended<T, COMPARE>::front() {
  return impl_->Front();
}

template <class T, class COMPARE>
const T& PriorityQueueExtended<T, COMPARE>::front() const {
  return impl_->Front();
}

template <class T, class COMPARE>
void PriorityQueueExtended<T, COMPARE>::pop() {
  impl_->Pop();
}

template <class T, class COMPARE>
PriorityQueueExtended<T, COMPARE>::PriorityQueueExtendedImpl::
    PriorityQueueExtendedImpl(const COMPARE& compare)
    : compare_(compare) {}

template <class T, class COMPARE>
bool PriorityQueueExtended<T, COMPARE>::PriorityQueueExtendedImpl::Empty()
    const {
  return container_.empty();
}

template <class T, class COMPARE>
void PriorityQueueExtended<T, COMPARE>::PriorityQueueExtendedImpl::Push(
    const T& value) {
  container_.push_back(DistinguishableObject(GetNextId(), value));
  std::push_heap(container_.begin(), container_.end(), compare_);
}

template <class T, class COMPARE>
void PriorityQueueExtended<T, COMPARE>::PriorityQueueExtendedImpl::Push(
    T&& value) {
  container_.push_back(DistinguishableObject(GetNextId(), std::move(value)));
  std::push_heap(container_.begin(), container_.end(), compare_);
}

template <class T, class COMPARE>
T& PriorityQueueExtended<T, COMPARE>::PriorityQueueExtendedImpl::Front() {
  return container_.front().obj;
}

template <class T, class COMPARE>
const T& PriorityQueueExtended<T, COMPARE>::PriorityQueueExtendedImpl::Front()
    const {
  return container_.front().obj;
}

template <class T, class COMPARE>
void PriorityQueueExtended<T, COMPARE>::PriorityQueueExtendedImpl::Pop() {
  if (!container_.empty()) {
    std::pop_heap(container_.begin(), container_.end(), compare_);
    container_.pop_back();
  }
}

template <class T, class COMPARE>
std::uint32_t
PriorityQueueExtended<T, COMPARE>::PriorityQueueExtendedImpl::GetNextId() {
  if (container_.empty()) {
    next_id_ = 0;
  }

  // TODO add renumbering of ids in case of overflow (when next_id_ equal
  // UMAX_INT)
  return next_id_++;
}

}  // namespace thread
}  // namespace olp
