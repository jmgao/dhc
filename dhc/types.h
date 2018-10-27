#pragma once

#include <windows.h>
#include <synchapi.h>

#include <experimental/memory>

#include "dhc/logging.h"
#include "dhc/macros.h"
#include "dhc/thread_annotations.h"

namespace dhc {

using std::experimental::observer_ptr;

template <typename KeyType, typename ValueType, KeyType Count>
struct EnumMap {
  EnumMap() {
    for (size_t i = 0; i < data_.size(); ++i) {
      data_[i].type = static_cast<KeyType>(i);
    }
  }

  ValueType& operator[](KeyType idx) { return data_.at(static_cast<size_t>(idx)); }

  const ValueType& operator[](KeyType idx) const { return data_.at(static_cast<size_t>(idx)); }

  struct Iterator {
    const std::array<ValueType, static_cast<size_t>(Count)>& base;
    size_t offset;

    bool operator==(const Iterator& rhs) const {
      return &base == &rhs.base && offset == rhs.offset;
    }

    bool operator!=(const Iterator& rhs) const { return !(*this == rhs); }

    std::pair<KeyType, ValueType&> operator*() const {
      return std::make_pair<KeyType, ValueType&>(static_cast<KeyType>(offset), base[offset]);
    }

    Iterator& operator++() {
      ++offset;
      return *this;
    }
  };

  Iterator begin() const { return Iterator{data_, 0}; }

  Iterator end() const { return Iterator{data_, data_.size()}; }

 private:
  std::array<ValueType, static_cast<size_t>(Count)> data_;
};


template <bool Recursive>
struct CAPABILITY("mutex") mutex_base {
  mutex_base() { InitializeCriticalSection(&critical_section_); }

  ~mutex_base() { DeleteCriticalSection(&critical_section_); }

  mutex_base(const mutex_base& copy) = delete;
  mutex_base(mutex_base&& move) = delete;

  void lock() ACQUIRE() {
    EnterCriticalSection(&critical_section_);
    if constexpr (!Recursive) {
      if (locked) {
        LOG(FATAL) << "attempted to recursively try_lock an already-locked mutex";
      }
      locked = true;
    }
  }

  bool try_lock() TRY_ACQUIRE(true) {
    if (TryEnterCriticalSection(&critical_section_)) {
      if constexpr (!Recursive) {
        if (locked) {
          LOG(FATAL) << "attempted to recursively try_lock an already-locked mutex";
        }
        locked = true;
      }
      return true;
    }

    return false;
  }

  void unlock() RELEASE() {
    if constexpr (!Recursive) {
      CHECK(locked);
      locked = false;
    }

    LeaveCriticalSection(&critical_section_);
  }

  // Provide an overload for operator! to support negative thread-safety annotations.
  const mutex_base& operator!() const;

  CRITICAL_SECTION critical_section_;
  bool locked = false;
};

struct mutex : public mutex_base<false> {};
struct recursive_mutex : public mutex_base<true> {};

struct CAPABILITY("mutex") shared_mutex {
  shared_mutex() = default;

  void lock() ACQUIRE() { AcquireSRWLockExclusive(&srw_lock_); }
  bool try_lock() TRY_ACQUIRE(true) { return TryAcquireSRWLockExclusive(&srw_lock_); }
  void unlock() RELEASE() { return ReleaseSRWLockExclusive(&srw_lock_); }

  void lock_shared() ACQUIRE_SHARED() { AcquireSRWLockExclusive(&srw_lock_); }
  bool try_lock_shared() TRY_ACQUIRE_SHARED(true) { return TryAcquireSRWLockExclusive(&srw_lock_); }
  void unlock_shared() RELEASE_SHARED() { return ReleaseSRWLockShared(&srw_lock_); }

  // Provide an overload for operator! to support negative thread-safety annotations.

  const shared_mutex& operator!() const;
 private:
  SRWLOCK srw_lock_ = SRWLOCK_INIT;
};

// This is available in libstdc++ even without winpthreads, but it doesn't have thread safety
// annotations, so implement it again ourselves.
template <typename T>
struct SCOPED_CAPABILITY lock_guard {
  lock_guard(T& mutex) ACQUIRE(mutex) : mutex_(mutex) { mutex_.lock(); }
  ~lock_guard() RELEASE() { mutex_.unlock(); }

 private:
  T& mutex_;
};

template <typename T>
struct SCOPED_CAPABILITY shared_lock_guard {
  shared_lock_guard(T& mutex) ACQUIRE_SHARED(mutex) : mutex_(mutex) { mutex_.lock_shared(); }
  ~shared_lock_guard() RELEASE_SHARED() { mutex_.unlock_shared(); }

 private:
  T& mutex_;
};

template<typename T>
class SCOPED_CAPABILITY ScopedAssumeLocked {
 public:
  ScopedAssumeLocked(T& mutex) ACQUIRE(mutex) {}
  ~ScopedAssumeLocked() RELEASE() {}
};

}  // namespace dhc