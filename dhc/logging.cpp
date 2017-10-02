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

#if defined(_WIN32)
#include <windows.h>
#endif

#include "logging.h"

#include <fcntl.h>
#include <time.h>

#include <iostream>
#include <limits>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <sys/types.h>

namespace dhc {
namespace logging {

static std::mutex& LoggingLock() {
  static auto& logging_lock = *new std::mutex();
  return logging_lock;
}

static LogFunction& Logger() {
  static auto& logger = *new LogFunction(DefaultLogger);
  return logger;
}

static AbortFunction& Aborter() {
  static auto& aborter = *new AbortFunction(DefaultAborter);
  return aborter;
}

static std::string& ProgramInvocationName() {
  static auto& programInvocationName = *new std::string("unknown");
  return programInvocationName;
}

static bool gInitialized = false;
static LogSeverity gMinimumLogSeverity = INFO;

static HANDLE console_out;
static FILE* file_out;

void CreateLogConsole() {
  if (console_out) {
    LOG(ERROR) << "log console already created";
    return;
  }

  AllocConsole();
  console_out = GetStdHandle(STD_OUTPUT_HANDLE);
}

void CreateLogFile(const std::string& path) {
  if (file_out) {
    LOG(ERROR) << "output file already created";
    return;
  }

  file_out = fopen(path.c_str(), "w+");
  return;
}

void DefaultLogger(LogId, LogSeverity severity, const char*, const char* file, unsigned int line,
                   const char* message) {
  if (!console_out && !file_out) {
    return;
  }

  struct tm now;
  time_t t = time(nullptr);
  localtime_s(&now, &t);

  char timestamp[32];
  strftime(timestamp, sizeof(timestamp), "%m-%d %H:%M:%S", &now);

  static const char log_characters[] = "VDIWEFF";
  static_assert(sizeof(log_characters) - 1 == FATAL + 1,
                "Mismatch in size of log_characters and values in LogSeverity");

  char severity_char = log_characters[severity];
  char buf[2048];
  size_t rc = snprintf(buf, sizeof(buf), "%c %s %5d %s:%u] %s\n", severity_char, timestamp,
                       GetCurrentThreadId(), file, line, message);
  if (console_out) {
    WriteFile(console_out, buf, rc, nullptr, nullptr);
  }
  if (file_out) {
    fwrite(buf, rc, 1, file_out);
    fflush(file_out);
  }
}

void DefaultAborter(const char* abort_message) {
  (void)abort_message;
  abort();
}

void InitLogging(char* argv[], LogFunction&& logger, AbortFunction&& aborter) {
  SetLogger(std::forward<LogFunction>(logger));
  SetAborter(std::forward<AbortFunction>(aborter));

  if (gInitialized) {
    return;
  }

  gInitialized = true;
}

void SetLogger(LogFunction&& logger) {
  std::lock_guard<std::mutex> lock(LoggingLock());
  Logger() = std::move(logger);
}

void SetAborter(AbortFunction&& aborter) {
  std::lock_guard<std::mutex> lock(LoggingLock());
  Aborter() = std::move(aborter);
}

static const char* GetFileBasename(const char* file) {
  // We can't use basename(3) even on Unix because the Mac doesn't
  // have a non-modifying basename.
  const char* last_slash = strrchr(file, '/');
  if (last_slash != nullptr) {
    return last_slash + 1;
  }
  #if defined(_WIN32)
  const char* last_backslash = strrchr(file, '\\');
  if (last_backslash != nullptr) {
    return last_backslash + 1;
  }
  #endif
  return file;
}

// This indirection greatly reduces the stack impact of having lots of
// checks/logging in a function.
class LogMessageData {
public:
  LogMessageData(const char* file, unsigned int line, LogId id,
                 LogSeverity severity, int error)
    : file_(GetFileBasename(file)),
    line_number_(line),
    id_(id),
    severity_(severity),
    error_(error) {
  }

  const char* GetFile() const {
    return file_;
  }

  unsigned int GetLineNumber() const {
    return line_number_;
  }

  LogSeverity GetSeverity() const {
    return severity_;
  }

  LogId GetId() const {
    return id_;
  }

  int GetError() const {
    return error_;
  }

  std::ostream& GetBuffer() {
    return buffer_;
  }

  std::string ToString() const {
    return buffer_.str();
  }

private:
  std::ostringstream buffer_;
  const char* const file_;
  const unsigned int line_number_;
  const LogId id_;
  const LogSeverity severity_;
  const int error_;

  LogMessageData(const LogMessageData& copy) = delete;
  LogMessageData(LogMessageData&& move) = delete;
};

LogMessage::LogMessage(const char* file, unsigned int line, LogId id,
                       LogSeverity severity, int error)
  : data_(new LogMessageData(file, line, id, severity, error)) {
}

LogMessage::~LogMessage() {
  // Check severity again. This is duplicate work wrt/ LOG macros, but not LOG_STREAM.
  if (!WOULD_LOG(data_->GetSeverity())) {
    return;
  }

  // Finish constructing the message.
  if (data_->GetError() != -1) {
    data_->GetBuffer() << ": " << strerror(data_->GetError());
  }
  std::string msg(data_->ToString());

  {
    // Do the actual logging with the lock held.
    std::lock_guard<std::mutex> lock(LoggingLock());
    if (msg.find('\n') == std::string::npos) {
      LogLine(data_->GetFile(), data_->GetLineNumber(), data_->GetId(),
              data_->GetSeverity(), msg.c_str());
    } else {
      msg += '\n';
      size_t i = 0;
      while (i < msg.size()) {
        size_t nl = msg.find('\n', i);
        msg[nl] = '\0';
        LogLine(data_->GetFile(), data_->GetLineNumber(), data_->GetId(),
                data_->GetSeverity(), &msg[i]);
        // Undo the zero-termination so we can give the complete message to the aborter.
        msg[nl] = '\n';
        i = nl + 1;
      }
    }
  }

  // Abort if necessary.
  if (data_->GetSeverity() == FATAL) {
    Aborter()(msg.c_str());
  }
}

std::ostream& LogMessage::stream() {
  return data_->GetBuffer();
}

void LogMessage::LogLine(const char* file, unsigned int line, LogId id,
                         LogSeverity severity, const char* message) {
  const char* tag = ProgramInvocationName().c_str();
  Logger()(id, severity, tag, file, line, message);
}

LogSeverity GetMinimumLogSeverity() {
  return gMinimumLogSeverity;
}

LogSeverity SetMinimumLogSeverity(LogSeverity new_severity) {
  LogSeverity old_severity = gMinimumLogSeverity;
  gMinimumLogSeverity = new_severity;
  return old_severity;
}

ScopedLogSeverity::ScopedLogSeverity(LogSeverity new_severity) {
  old_ = SetMinimumLogSeverity(new_severity);
}

ScopedLogSeverity::~ScopedLogSeverity() {
  SetMinimumLogSeverity(old_);
}

}  // namespace logging
}  // namespace dhc