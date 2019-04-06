#pragma once

#include <sstream>

#include "dhc/dhc.h"

// #undef overlapping macros coming from Win32.
#ifdef ERROR
#undef ERROR
#endif


#define VERBOSE LogLevel::Trace
#define DEBUG LogLevel::Debug
#define INFO LogLevel::Info
#define WARNING LogLevel::Warn
#define ERROR LogLevel::Error
#define FATAL LogLevel::Fatal

#define LOG(level) if (dhc_log_is_enabled(level)) LogMessage(level)
#define UNIMPLEMENTED(level) LOG(level) << "unimplemented function: " << __PRETTY_FUNCTION__
#define CHECK(predicate)                                                                    \
  if (!(predicate))                                                                         \
    LOG(FATAL) << "Check failed: " #predicate << "(" << __FILE__ << ": " << __LINE__ << ")"
#define CHECK_EQ(x, y) CHECK((x) == (y)) << " (" #x " == " << (x) << ", " #y << " == " << (y) << ")"
#define CHECK_GT(x, y) CHECK((x) > (y)) << " (" #x " == " << (x) << ", " #y << " == " << (y) << ")"
#define CHECK_GE(x, y) CHECK((x) >= (y)) << " (" #x " == " << (x) << ", " #y << " == " << (y) << ")"

struct LogMessage {
  explicit LogMessage(LogLevel level) : level_(level) {}

  ~LogMessage() {
    std::string message = ss_.str();
    dhc_log(level_, reinterpret_cast<const uint8_t*>(message.data()), message.size());
  }

  template<typename T>
  LogMessage& operator<<(T&& rhs) {
    ss_ << std::forward<T>(rhs);
    return *this;
  }

 private:
  LogLevel level_;
  std::stringstream ss_;
};
