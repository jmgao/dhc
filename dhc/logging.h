#pragma once

/*
* Copyright (C) 2015 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "dhc/macros.h"

//
// Google-style C++ logging.
//
// This header provides a C++ stream interface to logging.
//
// To log:
//
//   LOG(INFO) << "Some text; " << some_value;
//
// Replace `INFO` with any severity from `enum LogSeverity`.
//
// To log the result of a failed function and include the string
// representation of `errno` at the end:
//
//   PLOG(ERROR) << "Write failed";
//
// The output will be something like `Write failed: I/O error`.
// Remember this as 'P' as in perror(3).
//
// To output your own types, simply implement operator<< as normal.
//
// By default, output goes to logcat on Android and stderr on the host.
// A process can use `SetLogger` to decide where all logging goes.
// Implementations are provided for logcat, stderr, and dmesg.
// This header also provides assertions:
//
//   CHECK(must_be_true);
//   CHECK_EQ(a, b) << z_is_interesting_too;
// NOTE: For Windows, you must include logging.h after windows.h to allow the
// following code to suppress the evil ERROR macro:
#if defined(_WIN32) && defined(ERROR)
// windows.h includes wingdi.h which defines an evil macro ERROR.
#undef ERROR
#endif

#include <functional>
#include <memory>
#include <ostream>

namespace dhc {
namespace logging {

enum LogSeverity {
  VERBOSE,
  DEBUG,
  INFO,
  WARNING,
  ERROR,
  FATAL_WITHOUT_ABORT,
  FATAL,
};

enum LogId {
  DEFAULT,
  MAIN,
  SYSTEM,
};

using LogFunction = std::function<void(LogId, LogSeverity, const char*,
                                       const char*, unsigned int, const char*)>;
using AbortFunction = std::function<void(const char*)>;

DHC_API void CreateLogConsole();
DHC_API void CreateLogFile(const std::string& path);

DHC_API void DefaultLogger(LogId, LogSeverity, const char*, const char*, unsigned int, const char*);
DHC_API void DefaultAborter(const char* abort_message);

DHC_API void InitLogging(char* argv[], LogFunction&& logger = &DefaultLogger,
                 AbortFunction&& aborter = &DefaultAborter);

// Replace the current logger.
DHC_API void SetLogger(LogFunction&& logger);

// Replace the current aborter.
DHC_API void SetAborter(AbortFunction&& aborter);

// A helper macro that produces an expression that accepts both a qualified name and an
// unqualified name for a LogSeverity, and returns a LogSeverity value.
// Note: DO NOT USE DIRECTLY. This is an implementation detail.
#define SEVERITY_LAMBDA(severity) ([&]() {    \
  using ::dhc::logging::VERBOSE;             \
  using ::dhc::logging::DEBUG;               \
  using ::dhc::logging::INFO;                \
  using ::dhc::logging::WARNING;             \
  using ::dhc::logging::ERROR;               \
  using ::dhc::logging::FATAL_WITHOUT_ABORT; \
  using ::dhc::logging::FATAL;               \
  return (severity); }())

#define ABORT_AFTER_LOG_FATAL
#define ABORT_AFTER_LOG_EXPR_IF(c, x) (x)
#define MUST_LOG_MESSAGE(severity) false
#define ABORT_AFTER_LOG_FATAL_EXPR(x) ABORT_AFTER_LOG_EXPR_IF(true, x)

#define LIKELY(x) (x)
#define UNLIKELY(x) (x)

// Defines whether the given severity will be logged or silently swallowed.
#define WOULD_LOG(severity)                                                            \
  (UNLIKELY((SEVERITY_LAMBDA(severity)) >= ::dhc::logging::GetMinimumLogSeverity()) || \
   MUST_LOG_MESSAGE(severity))

// Get an ostream that can be used for logging at the given severity and to the default
// destination.
//
// Notes:
// 1) This will not check whether the severity is high enough. One should use WOULD_LOG to filter
//    usage manually.
// 2) This does not save and restore errno.
#define LOG_STREAM(severity) LOG_STREAM_TO(DEFAULT, severity)

// Get an ostream that can be used for logging at the given severity and to the
// given destination. The same notes as for LOG_STREAM apply.
#define LOG_STREAM_TO(dest, severity)                                                             \
  ::dhc::logging::LogMessage(__FILE__, __LINE__, ::dhc::logging::dest, SEVERITY_LAMBDA(severity), \
                             -1)                                                                  \
      .stream()

// Logs a message to logcat on Android otherwise to stderr. If the severity is
// FATAL it also causes an abort. For example:
//
//     LOG(FATAL) << "We didn't expect to reach here";
#define LOG(severity) LOG_TO(DEFAULT, severity)

// Checks if we want to log something, and sets up appropriate RAII objects if
// so.
// Note: DO NOT USE DIRECTLY. This is an implementation detail.
#define LOGGING_PREAMBLE(severity) \
  (WOULD_LOG(severity) &&          \
   ABORT_AFTER_LOG_EXPR_IF((SEVERITY_LAMBDA(severity)) == ::dhc::logging::FATAL, true))

// Logs a message to logcat with the specified log ID on Android otherwise to
// stderr. If the severity is FATAL it also causes an abort.
// Use an expression here so we can support the << operator following the macro,
// like "LOG(DEBUG) << xxx;".
#define LOG_TO(dest, severity) LOGGING_PREAMBLE(severity) && LOG_STREAM_TO(dest, severity)

#if defined(_MSC_VER)
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif

// Marker that code is yet to be implemented.
#define UNIMPLEMENTED(level) \
  LOG(level) << __PRETTY_FUNCTION__ << " unimplemented "

// Check whether condition x holds and LOG(FATAL) if not. The value of the
// expression x is only evaluated once. Extra logging can be appended using <<
// after. For example:
//
//     CHECK(false == true) results in a log message of
//       "Check failed: false == true".
#define CHECK(x)                                                              \
  LIKELY((x)) || ABORT_AFTER_LOG_FATAL_EXPR(false) ||                         \
      ::dhc::logging::LogMessage(__FILE__, __LINE__, ::dhc::logging::DEFAULT, \
                                 ::dhc::logging::FATAL, -1)                   \
              .stream()                                                       \
          << "Check failed: " #x << " "

// Helper for CHECK_xx(x,y) macros.
#define CHECK_OP(LHS, RHS, OP)                                                                   \
  for (auto _values = ::dhc::logging::MakeEagerEvaluator(LHS, RHS);                              \
       UNLIKELY(!(_values.lhs OP _values.rhs));                                                  \
       /* empty */)                                                                              \
    ABORT_AFTER_LOG_FATAL                                                                        \
  ::dhc::logging::LogMessage(__FILE__, __LINE__, ::dhc::logging::DEFAULT, ::dhc::logging::FATAL, \
                             -1)                                                                 \
          .stream()                                                                              \
      << "Check failed: " << #LHS << " " << #OP << " " << #RHS << " (" #LHS "=" << _values.lhs   \
      << ", " #RHS "=" << _values.rhs << ") "

// Check whether a condition holds between x and y, LOG(FATAL) if not. The value
// of the expressions x and y is evaluated once. Extra logging can be appended
// using << after. For example:
//
//     CHECK_NE(0 == 1, false) results in
//       "Check failed: false != false (0==1=false, false=false) ".
#define CHECK_EQ(x, y) CHECK_OP(x, y, == )
#define CHECK_NE(x, y) CHECK_OP(x, y, != )
#define CHECK_LE(x, y) CHECK_OP(x, y, <= )
#define CHECK_LT(x, y) CHECK_OP(x, y, < )
#define CHECK_GE(x, y) CHECK_OP(x, y, >= )
#define CHECK_GT(x, y) CHECK_OP(x, y, > )
// Helper for CHECK_STRxx(s1,s2) macros.
#define CHECK_STROP(s1, s2, sense)                                             \
  while (UNLIKELY((strcmp(s1, s2) == 0) != (sense)))                           \
    ABORT_AFTER_LOG_FATAL                                                      \
    ::dhc::logging::LogMessage(__FILE__, __LINE__, ::dhc::logging::DEFAULT,  \
                                ::dhc::logging::FATAL, -1).stream()           \
        << "Check failed: " << "\"" << (s1) << "\""                            \
        << ((sense) ? " == " : " != ") << "\"" << (s2) << "\""
// Check for string (const char*) equality between s1 and s2, LOG(FATAL) if not.
#define CHECK_STREQ(s1, s2) CHECK_STROP(s1, s2, true)
#define CHECK_STRNE(s1, s2) CHECK_STROP(s1, s2, false)

// CHECK that can be used in a constexpr function. For example:
//
//    constexpr int half(int n) {
//      return
//          DCHECK_CONSTEXPR(n >= 0, , 0)
//          CHECK_CONSTEXPR((n & 1) == 0),
//              << "Extra debugging output: n = " << n, 0)
//          n / 2;
//    }
#define CHECK_CONSTEXPR(x, out, dummy)                                     \
  (UNLIKELY(!(x)))                                                         \
      ? (LOG(FATAL) << "Check failed: " << #x out, dummy) \
      :

// DCHECKs are debug variants of CHECKs only enabled in debug builds. Generally
// CHECK should be used unless profiling identifies a CHECK as being in
// performance critical code.
#if defined(NDEBUG) && !defined(__clang_analyzer__)
static constexpr bool kEnableDChecks = false;
#else
static constexpr bool kEnableDChecks = true;
#endif
#define DCHECK(x) \
  if (::dhc::logging::kEnableDChecks) CHECK(x)
#define DCHECK_EQ(x, y) \
  if (::dhc::logging::kEnableDChecks) CHECK_EQ(x, y)
#define DCHECK_NE(x, y) \
  if (::dhc::logging::kEnableDChecks) CHECK_NE(x, y)
#define DCHECK_LE(x, y) \
  if (::dhc::logging::kEnableDChecks) CHECK_LE(x, y)
#define DCHECK_LT(x, y) \
  if (::dhc::logging::kEnableDChecks) CHECK_LT(x, y)
#define DCHECK_GE(x, y) \
  if (::dhc::logging::kEnableDChecks) CHECK_GE(x, y)
#define DCHECK_GT(x, y) \
  if (::dhc::logging::kEnableDChecks) CHECK_GT(x, y)
#define DCHECK_STREQ(s1, s2) \
  if (::dhc::logging::kEnableDChecks) CHECK_STREQ(s1, s2)
#define DCHECK_STRNE(s1, s2) \
  if (::dhc::logging::kEnableDChecks) CHECK_STRNE(s1, s2)

#if defined(NDEBUG) && !defined(__clang_analyzer__)
#define DCHECK_CONSTEXPR(x, out, dummy)
#else
#define DCHECK_CONSTEXPR(x, out, dummy) CHECK_CONSTEXPR(x, out, dummy)
#endif

// Temporary class created to evaluate the LHS and RHS, used with
// MakeEagerEvaluator to infer the types of LHS and RHS.
template <typename LHS, typename RHS>
struct EagerEvaluator {
  constexpr EagerEvaluator(LHS l, RHS r) : lhs(l), rhs(r) {}
  LHS lhs;
  RHS rhs;
};

// Helper function for CHECK_xx.
template <typename LHS, typename RHS>
constexpr EagerEvaluator<LHS, RHS> MakeEagerEvaluator(LHS lhs, RHS rhs) {
  return EagerEvaluator<LHS, RHS>(lhs, rhs);
}

// Explicitly instantiate EagerEvalue for pointers so that char*s aren't treated
// as strings. To compare strings use CHECK_STREQ and CHECK_STRNE. We rely on
// signed/unsigned warnings to protect you against combinations not explicitly
// listed below.
#define EAGER_PTR_EVALUATOR(T1, T2)               \
  template <>                                     \
  struct EagerEvaluator<T1, T2> {                 \
    EagerEvaluator(T1 l, T2 r)                    \
        : lhs(reinterpret_cast<const void*>(l)),  \
          rhs(reinterpret_cast<const void*>(r)) { \
    }                                             \
    const void* lhs;                              \
    const void* rhs;                              \
  }
EAGER_PTR_EVALUATOR(const char*, const char*);
EAGER_PTR_EVALUATOR(const char*, char*);
EAGER_PTR_EVALUATOR(char*, const char*);
EAGER_PTR_EVALUATOR(char*, char*);
EAGER_PTR_EVALUATOR(const unsigned char*, const unsigned char*);
EAGER_PTR_EVALUATOR(const unsigned char*, unsigned char*);
EAGER_PTR_EVALUATOR(unsigned char*, const unsigned char*);
EAGER_PTR_EVALUATOR(unsigned char*, unsigned char*);
EAGER_PTR_EVALUATOR(const signed char*, const signed char*);
EAGER_PTR_EVALUATOR(const signed char*, signed char*);
EAGER_PTR_EVALUATOR(signed char*, const signed char*);
EAGER_PTR_EVALUATOR(signed char*, signed char*);

// Data for the log message, not stored in LogMessage to avoid increasing the
// stack size.
class LogMessageData;

// A LogMessage is a temporarily scoped object used by LOG and the unlikely part
// of a CHECK. The destructor will abort if the severity is FATAL.
class LogMessage {
 public:
  DHC_API LogMessage(const char* file, unsigned int line, LogId id, LogSeverity severity,
                     int error);
  DHC_API ~LogMessage();
  // Returns the stream associated with the message, the LogMessage performs
  // output when it goes out of scope.
  DHC_API std::ostream& stream();
  // The routine that performs the actual logging.
  DHC_API static void LogLine(const char* file, unsigned int line, LogId id, LogSeverity severity,
                      const char* msg);

 private:
  const std::unique_ptr<LogMessageData> data_;
  LogMessage(const LogMessage& copy) = delete;
  LogMessage(LogMessage&& move) = delete;
};

// Get the minimum severity level for logging.
DHC_API LogSeverity GetMinimumLogSeverity();

// Set the minimum severity level for logging, returning the old severity.
DHC_API LogSeverity SetMinimumLogSeverity(LogSeverity new_severity);

// Allows to temporarily change the minimum severity level for logging.
class ScopedLogSeverity {
 public:
  DHC_API explicit ScopedLogSeverity(LogSeverity level);
  DHC_API ~ScopedLogSeverity();

 private:
  LogSeverity old_;
};

}  // namespace logging
}  // namespace dhc
