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

#if ((__cplusplus >= 201703L) || (defined(_MSC_VER) && _MSC_VER >= 1900))
#include <shared_mutex>
#else

// This implementation is taken from GCC 7.4 headers

#include <cassert>
#include <condition_variable>

#if (defined(__USE_UNIX98) || defined(__USE_XOPEN2K))
#define HAVE_PTHREAD_RWLOCK
#endif

#ifndef THROW_OR_ABORT
# if __cpp_exceptions
#  define THROW_OR_ABORT(exception) (throw (exception))
# else
#  include <cstdlib>
#  define THROW_OR_ABORT(exception) \
    do {                            \
      (void)exception;              \
      std::abort();                 \
    } while (0)
# endif
#endif

namespace std {
namespace detail {

// C++11 compatible std::exchange
template<class T, class U = T>
T
exchange(T& obj, U&& new_value)
{
  T old_value = std::move(obj);
  obj = std::forward<U>(new_value);
  return old_value;
}

#if defined(HAVE_PTHREAD_RWLOCK)

/// A shared mutex type implemented using pthread_rwlock_t.
class shared_mutex_pthread {
#if defined(PTHREAD_RWLOCK_INITIALIZER)
  pthread_rwlock_t _M_rwlock = PTHREAD_RWLOCK_INITIALIZER;

 public:
  shared_mutex_pthread() = default;
  ~shared_mutex_pthread() = default;
#else
  pthread_rwlock_t _M_rwlock;

 public:
  shared_mutex_pthread() {
    int __ret = pthread_rwlock_init(&_M_rwlock, NULL);
    if (__ret == ENOMEM)
      THROW_OR_ABORT(std::bad_alloc());
    else if (__ret == EAGAIN)
      THROW_OR_ABORT(std::system_error(
          std::make_error_code(errc::resource_unavailable_try_again)));
    else if (__ret == EPERM)
      THROW_OR_ABORT(std::system_error(
          std::make_error_code(errc::operation_not_permitted)));
    // Errors not handled: EBUSY, EINVAL
    assert(__ret == 0);
  }

  ~shared_mutex_pthread() {
    int __ret __attribute((__unused__)) = pthread_rwlock_destroy(&_M_rwlock);
    // Errors not handled: EBUSY, EINVAL
    assert(__ret == 0);
  }
#endif

  shared_mutex_pthread(const shared_mutex_pthread&) = delete;
  shared_mutex_pthread& operator=(const shared_mutex_pthread&) = delete;

  void lock() {
    int __ret = pthread_rwlock_wrlock(&_M_rwlock);
    if (__ret == EDEADLK)
      THROW_OR_ABORT(std::system_error(
          std::make_error_code(errc::resource_deadlock_would_occur)));
    // Errors not handled: EINVAL
    assert(__ret == 0);
  }

  bool try_lock() {
    int __ret = pthread_rwlock_trywrlock(&_M_rwlock);
    if (__ret == EBUSY)
      return false;
    // Errors not handled: EINVAL
    assert(__ret == 0);
    return true;
  }

  void unlock() {
    int __ret __attribute((__unused__)) = pthread_rwlock_unlock(&_M_rwlock);
    // Errors not handled: EPERM, EBUSY, EINVAL
    assert(__ret == 0);
  }

  // Shared ownership

  void lock_shared() {
    int __ret;
    // We retry if we exceeded the maximum number of read locks supported by
    // the POSIX implementation; this can result in busy-waiting, but this
    // is okay based on the current specification of forward progress
    // guarantees by the standard.
    do {
      __ret = pthread_rwlock_rdlock(&_M_rwlock);
    } while (__ret == EAGAIN);
    if (__ret == EDEADLK)
      THROW_OR_ABORT(std::system_error(
          std::make_error_code(errc::resource_deadlock_would_occur)));
    // Errors not handled: EINVAL
    assert(__ret == 0);
  }

  bool try_lock_shared() {
    int __ret = pthread_rwlock_tryrdlock(&_M_rwlock);
    // If the maximum number of read locks has been exceeded, we just fail
    // to acquire the lock.  Unlike for lock(), we are not allowed to throw
    // an exception.
    if (__ret == EBUSY || __ret == EAGAIN)
      return false;
    // Errors not handled: EINVAL
    assert(__ret == 0);
    return true;
  }

  void unlock_shared() { unlock(); }

  void* native_handle() { return &_M_rwlock; }
};

#else

/// A shared mutex type implemented using std::condition_variable.
class shared_mutex_cv {
  // Based on Howard Hinnant's reference implementation from N2406.

  // The high bit of _M_state is the write-entered flag which is set to
  // indicate a writer has taken the lock or is queuing to take the lock.
  // The remaining bits are the count of reader locks.
  //
  // To take a reader lock, block on gate1 while the write-entered flag is
  // set or the maximum number of reader locks is held, then increment the
  // reader lock count.
  // To release, decrement the count, then if the write-entered flag is set
  // and the count is zero then signal gate2 to wake a queued writer,
  // otherwise if the maximum number of reader locks was held signal gate1
  // to wake a reader.
  //
  // To take a writer lock, block on gate1 while the write-entered flag is
  // set, then set the write-entered flag to start queueing, then block on
  // gate2 while the number of reader locks is non-zero.
  // To release, unset the write-entered flag and signal gate1 to wake all
  // blocked readers and writers.
  //
  // This means that when no reader locks are held readers and writers get
  // equal priority. When one or more reader locks is held a writer gets
  // priority and no more reader locks can be taken while the writer is
  // queued.

  // Only locked when accessing _M_state or waiting on condition variables.
  mutex _M_mut;
  // Used to block while write-entered is set or reader count at maximum.
  condition_variable _M_gate1;
  // Used to block queued writers while reader count is non-zero.
  condition_variable _M_gate2;
  // The write-entered flag and reader count.
  unsigned _M_state;

  static constexpr unsigned _S_write_entered =
      1U << (sizeof(unsigned) * __CHAR_BIT__ - 1);
  static constexpr unsigned _S_max_readers = ~_S_write_entered;

  // Test whether the write-entered flag is set. _M_mut must be locked.
  bool _M_write_entered() const { return _M_state & _S_write_entered; }

  // The number of reader locks currently held. _M_mut must be locked.
  unsigned _M_readers() const { return _M_state & _S_max_readers; }

 public:
  shared_mutex_cv() : _M_state(0) {}

  ~shared_mutex_cv() { assert(_M_state == 0); }

  shared_mutex_cv(const shared_mutex_cv&) = delete;
  shared_mutex_cv& operator=(const shared_mutex_cv&) = delete;

  // Exclusive ownership

  void lock() {
    unique_lock<mutex> __lk(_M_mut);
    // Wait until we can set the write-entered flag.
    _M_gate1.wait(__lk, [=] { return !_M_write_entered(); });
    _M_state |= _S_write_entered;
    // Then wait until there are no more readers.
    _M_gate2.wait(__lk, [=] { return _M_readers() == 0; });
  }

  bool try_lock() {
    unique_lock<mutex> __lk(_M_mut, try_to_lock);
    if (__lk.owns_lock() && _M_state == 0) {
      _M_state = _S_write_entered;
      return true;
    }
    return false;
  }

  void unlock() {
    lock_guard<mutex> __lk(_M_mut);
    assert(_M_write_entered());
    _M_state = 0;
    // call notify_all() while mutex is held so that another thread can't
    // lock and unlock the mutex then destroy *this before we make the call.
    _M_gate1.notify_all();
  }

  // Shared ownership

  void lock_shared() {
    unique_lock<mutex> __lk(_M_mut);
    _M_gate1.wait(__lk, [=] { return _M_state < _S_max_readers; });
    ++_M_state;
  }

  bool try_lock_shared() {
    unique_lock<mutex> __lk(_M_mut, try_to_lock);
    if (!__lk.owns_lock())
      return false;
    if (_M_state < _S_max_readers) {
      ++_M_state;
      return true;
    }
    return false;
  }

  void unlock_shared() {
    lock_guard<mutex> __lk(_M_mut);
    assert(_M_readers() > 0);
    auto __prev = _M_state--;
    if (_M_write_entered()) {
      // Wake the queued writer if there are no more readers.
      if (_M_readers() == 0)
        _M_gate2.notify_one();
      // No need to notify gate1 because we give priority to the queued
      // writer, and that writer will eventually notify gate1 after it
      // clears the write-entered flag.
    } else {
      // Wake any thread that was blocked on reader overflow.
      if (__prev == _S_max_readers)
        _M_gate1.notify_one();
    }
  }
};
#endif
}  // namespace detail

class shared_mutex {
 public:
  shared_mutex() = default;
  ~shared_mutex() = default;

  shared_mutex(const shared_mutex&) = delete;
  shared_mutex& operator=(const shared_mutex&) = delete;

  void lock() { _M_impl.lock(); }
  bool try_lock() { return _M_impl.try_lock(); }
  void unlock() { _M_impl.unlock(); }

  void lock_shared() { _M_impl.lock_shared(); }
  bool try_lock_shared() { return _M_impl.try_lock_shared(); }
  void unlock_shared() { _M_impl.unlock_shared(); }

#if defined(HAVE_PTHREAD_RWLOCK)
  typedef void* native_handle_type;
  native_handle_type native_handle() { return _M_impl.native_handle(); }

 private:
  detail::shared_mutex_pthread _M_impl;
#else

 private:
  detail::shared_mutex_cv _M_impl;
#endif
};

template <typename Mutex>
class shared_lock {
 public:
  typedef Mutex mutex_type;

  shared_lock() noexcept : _M_pm(nullptr), _M_owns(false) {}

  explicit shared_lock(mutex_type& __m)
      : _M_pm(std::__addressof(__m)), _M_owns(true) {
    __m.lock_shared();
  }

  shared_lock(mutex_type& __m, defer_lock_t) noexcept
      : _M_pm(std::__addressof(__m)), _M_owns(false) {}

  shared_lock(mutex_type& __m, try_to_lock_t)
      : _M_pm(std::__addressof(__m)), _M_owns(__m.try_lock_shared()) {}

  shared_lock(mutex_type& __m, adopt_lock_t)
      : _M_pm(std::__addressof(__m)), _M_owns(true) {}

  template <typename _Clock, typename _Duration>
  shared_lock(mutex_type& __m,
              const chrono::time_point<_Clock, _Duration>& __abs_time)
      : _M_pm(std::__addressof(__m)),
        _M_owns(__m.try_lock_shared_until(__abs_time)) {}

  template <typename _Rep, typename _Period>
  shared_lock(mutex_type& __m,
              const chrono::duration<_Rep, _Period>& __rel_time)
      : _M_pm(std::__addressof(__m)),
        _M_owns(__m.try_lock_shared_for(__rel_time)) {}

  ~shared_lock() {
    if (_M_owns)
      _M_pm->unlock_shared();
  }

  shared_lock(shared_lock const&) = delete;
  shared_lock& operator=(shared_lock const&) = delete;

  shared_lock(shared_lock&& __sl) noexcept : shared_lock() { swap(__sl); }

  shared_lock& operator=(shared_lock&& __sl) noexcept {
    shared_lock(std::move(__sl)).swap(*this);
    return *this;
  }

  void lock() {
    _M_lockable();
    _M_pm->lock_shared();
    _M_owns = true;
  }

  bool try_lock() {
    _M_lockable();
    return _M_owns = _M_pm->try_lock_shared();
  }

  template <typename _Rep, typename _Period>
  bool try_lock_for(const chrono::duration<_Rep, _Period>& __rel_time) {
    _M_lockable();
    return _M_owns = _M_pm->try_lock_shared_for(__rel_time);
  }

  template <typename _Clock, typename _Duration>
  bool try_lock_until(const chrono::time_point<_Clock, _Duration>& __abs_time) {
    _M_lockable();
    return _M_owns = _M_pm->try_lock_shared_until(__abs_time);
  }

  void unlock() {
    if (!_M_owns)
      THROW_OR_ABORT(std::system_error(
          std::make_error_code(errc::resource_deadlock_would_occur)));
    _M_pm->unlock_shared();
    _M_owns = false;
  }

  void swap(shared_lock& __u) noexcept {
    std::swap(_M_pm, __u._M_pm);
    std::swap(_M_owns, __u._M_owns);
  }

  mutex_type* release() noexcept {
    _M_owns = false;
    return detail::exchange(_M_pm, nullptr);
  }

  bool owns_lock() const noexcept { return _M_owns; }

  explicit operator bool() const noexcept { return _M_owns; }

  mutex_type* mutex() const noexcept { return _M_pm; }

 private:
  void _M_lockable() const {
    if (_M_pm == nullptr)
      THROW_OR_ABORT(std::system_error(
          std::make_error_code(errc::operation_not_permitted)));
    if (_M_owns)
      THROW_OR_ABORT(std::system_error(
          std::make_error_code(errc::resource_deadlock_would_occur)));
  }

  mutex_type* _M_pm;
  bool _M_owns;
};

template <typename _Mutex>
void swap(shared_lock<_Mutex>& __x, shared_lock<_Mutex>& __y) noexcept {
  __x.swap(__y);
}

}  // namespace std

#if defined(HAVE_PTHREAD_RWLOCK)
#undef HAVE_PTHREAD_RWLOCK
#endif

#endif
