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

}  // namespace dhc
