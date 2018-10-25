#pragma once

#include <chrono>

#include "dhc/logging.h"

namespace dhc {

struct Timer {
  explicit Timer(const char* function)
      : function_(function), begin_(std::chrono::high_resolution_clock::now()) {}

  ~Timer() {
    using namespace std::chrono_literals;
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = end - begin_;
    auto error_level = logging::DEBUG;
    if (duration > 1ms) {
      error_level = logging::ERROR;
    }
    LOG(error_level) << function_ << " completed in " << duration / 1.0ms << "ms";
  }

  const char* function_;
  std::chrono::high_resolution_clock::time_point begin_;
};

#define TIMER() Timer __timer(__func__)

}  // namespace dhc
