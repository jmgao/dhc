#pragma once

#include <dinput.h>

#include <string.h>
#include <wchar.h>

#include <atomic>
#include <string>
#include <string_view>

#include "dhc/dhc.h"

namespace dhc {

DHC_API HMODULE LoadSystemLibrary(const std::wstring& name);
DHC_API FARPROC WINAPI GetDirectInput8Proc(const char* proc_name);

// String helpers.
template <typename Iterable>
std::string Join(Iterable&& iterable, const std::string& glue = ", ") {
  std::string result;
  for (const auto& item : iterable) {
    result += item;
    result += glue;
  }
  if (!result.empty()) {
    result.resize(result.size() - glue.size());
  }
  return result;
}

// Unicode helpers.
DHC_API std::string to_string(const std::string& str);
DHC_API std::string to_string(const std::wstring& wstr);

DHC_API std::wstring to_wstring(const std::string& str);
DHC_API std::wstring to_wstring(const std::wstring& wstr);

inline wchar_t* tstrncpy(wchar_t* dst, const char* src, size_t len) {
  std::wstring wstr = to_wstring(src);
  return wcsncpy(dst, wstr.data(), len);
}

inline char* tstrncpy(char* dst, const char* src, size_t len) {
  return strncpy(dst, src, len);
}

// Windows junk.
// See https://blogs.msdn.microsoft.com/oldnewthing/20041025-00/?p=37483
extern "C" IMAGE_DOS_HEADER __ImageBase;
#define HINST_SELF ((HINSTANCE)&__ImageBase)

DHC_API std::string_view dierr_to_string(HRESULT result);

// COM junk.
template <typename T>
struct com_base : public T {
  com_base() : ref_count_(1) {}
  virtual ~com_base() = default;

  virtual ULONG STDMETHODCALLTYPE AddRef() override final { return ++ref_count_; }
  virtual ULONG STDMETHODCALLTYPE Release() override final {
    ULONG rc = --ref_count_;
    if (rc == 0) {
      delete this;
    }
    return rc;
  }

 private:
  std::atomic<ULONG> ref_count_;
};

template <typename T>
struct com_ptr {
  com_ptr() {}
  explicit com_ptr(T* ptr) {
    ptr_ = ptr;
  }

  ~com_ptr() {
    reset();
  }

  com_ptr(const com_ptr& copy) = delete;
  com_ptr(com_ptr&& move) {
    reset(move.release());
  }

  com_ptr& operator=(const com_ptr& copy) = delete;
  com_ptr& operator=(com_ptr&& move) {
    reset(move.release());
    return *this;
  }

  com_ptr clone() {
    ptr_->AddRef();
    return com_ptr(ptr_);
  }

  T* release() {
    T* temp = ptr_;
    ptr_ = nullptr;
    return temp;
  }

  T** receive() {
    reset();
    return &ptr_;
  }

  void reset(T* new_ptr = nullptr) {
    if (ptr_) {
      ptr_->Release();
    }
    ptr_ = new_ptr;
  }

  T* get() {
    return ptr_;
  }

  const T* get() const {
    return ptr_;
  }

  T* operator->() {
    return ptr_;
  }

  const T* operator->() const {
    return ptr_;
  }

  T& operator*() {
    return *ptr_;
  }

  const T& operator*() const {
    return *ptr_;
  }

  operator bool() {
    return ptr_;
  }

 private:
  T* ptr_ = nullptr;
};

DHC_API com_ptr<IDirectInput8A> GetRealDirectInput8A();
DHC_API com_ptr<IDirectInput8W> GetRealDirectInput8W();

}  // namespace dhc
