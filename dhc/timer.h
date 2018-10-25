#pragma once

#include <chrono>
#include <experimental/source_location>

#include "dhc/logging.h"

namespace dhc {

struct Timer {
  explicit Timer(
      std::experimental::source_location location = std::experimental::source_location::current())
      : sloc_(location), begin_(std::chrono::high_resolution_clock::now()) {}

  ~Timer() {
    using namespace std::chrono_literals;
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = end - begin_;
    auto error_level = logging::DEBUG;
    if (duration > 1ms) {
      error_level = logging::ERROR;
    }
    LOG(error_level) << sloc_.function_name() << " completed in " << duration / 1.0ms << "ms";
  }

  std::experimental::source_location sloc_;
  std::chrono::high_resolution_clock::time_point begin_;
};

}  // namespace dhc