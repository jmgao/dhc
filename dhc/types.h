#pragma once

#include <windows.h>

#include <experimental/memory>

#include "dhc/logging.h"
#include "dhc/macros.h"
#include "dhc/thread_annotations.h"

namespace dhc {

using std::experimental::observer_ptr;

template<typename KeyType, typename ValueType, KeyType Count>
struct EnumMap {
  EnumMap() {
    for (size_t i = 0; i < data_.size(); ++i) {
      data_[i].type = static_cast<KeyType>(i);
    }
  }

  ValueType& operator[](KeyType idx) {
    return data_.at(static_cast<size_t>(idx));
  }

  const ValueType& operator[](KeyType idx) const {
    return data_.at(static_cast<size_t>(idx));
  }

  struct Iterator {
    const std::array<ValueType, static_cast<size_t>(Count)>& base;
    size_t offset;

    bool operator==(const Iterator& rhs) const {
      return &base == &rhs.base && offset == rhs.offset;
    }

    bool operator!=(const Iterator& rhs) const {
      return !(*this == rhs);
    }

    std::pair<KeyType, ValueType&> operator*() const {
      return std::make_pair<KeyType, ValueType&>(static_cast<KeyType>(offset), base[offset]);
    }

    Iterator& operator++() {
      ++offset;
      return *this;
    }
  };

  Iterator begin() const {
    return Iterator { data_, 0 };
  }


  Iterator end() const {
    return Iterator { data_, data_.size() };
  }

 private:
  std::array<ValueType, static_cast<size_t>(Count)> data_;
};

template<typename T>
struct SCOPED_CAPABILITY lock_guard {
  lock_guard(T& mutex) ACQUIRE(mutex) : mutex_(mutex) {
    mutex_.lock();
  }

  ~lock_guard() RELEASE() {
    mutex_.unlock();
  }

 private:
  T& mutex_;
};

template<bool Recursive>
struct CAPABILITY("mutex") mutex_base {
  mutex_base() {
    InitializeCriticalSection(&critical_section_);
  }

  ~mutex_base() {
    DeleteCriticalSection(&critical_section_);
  }

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

  CRITICAL_SECTION critical_section_;
  bool locked = false;
};

struct mutex : public mutex_base<false> {};

class SCOPED_CAPABILITY ScopedAssumeLocked {
  public:
    ScopedAssumeLocked(mutex& mutex) ACQUIRE(mutex) {}
    ~ScopedAssumeLocked() RELEASE() {}
};

}  // namespace dhc
