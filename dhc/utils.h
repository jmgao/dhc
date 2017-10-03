#pragma once

#include <string.h>
#include <wchar.h>

#include "dhc.h"

namespace dhc {

DHC_API HMODULE LoadSystemLibrary(const std::wstring& name);

// Unicode helpers.
DHC_API std::string to_string(const std::string& str);
DHC_API std::string to_string(const std::wstring& wstr);

DHC_API std::wstring to_wstring(const std::string& str);
DHC_API std::wstring to_wstring(const std::wstring& wstr);

template<typename CharType>
static CharType* tstrncpy(CharType* dst, char* src, size_t len);

template<>
wchar_t* tstrncpy(wchar_t* dst, char* src, size_t len) {
  std::wstring wstr = to_wstring(src);
  return wcsncpy(dst, wstr.data(), len);
}

template<>
char* tstrncpy(char* dst, char* src, size_t len) {
  return strncpy(dst, src, len);
}

// Windows junk.
// See https://blogs.msdn.microsoft.com/oldnewthing/20041025-00/?p=37483
extern "C" IMAGE_DOS_HEADER __ImageBase;
#define HINST_SELF ((HINSTANCE)&__ImageBase)

// COM junk.
#define COM_OBJECT_BASE()                                                          \
  virtual ULONG STDMETHODCALLTYPE AddRef() override final { return ++ref_count_; } \
  virtual ULONG STDMETHODCALLTYPE Release() override final {                       \
    ULONG rc = --ref_count_;                                                       \
    if (rc == 0) {                                                                 \
      delete this;                                                                 \
    }                                                                              \
    return rc;                                                                     \
  }                                                                                \
  std::atomic<ULONG> ref_count_ = 1

template <typename T>
struct unique_com_ptr {
  unique_com_ptr() {}
  explicit unique_com_ptr(T* ptr) {
    ptr_ = ptr;
    ptr_->AddRef();
  }

  ~unique_com_ptr() {
    reset();
  }

  unique_com_ptr(const unique_com_ptr& copy) = delete;
  unique_com_ptr(unique_com_ptr&& move) {
    reset(move.release());
  }

  unique_com_ptr& operator=(const unique_com_ptr& copy) = delete;
  unique_com_ptr& operator=(unique_com_ptr&& move) {
    reset(move.release());
    return *this;
  }

  T* release() {
    T* temp = ptr_;
    ptr_ = nullptr;
    return temp;
  }

  void reset(T* new_ptr = nullptr) {
    if (ptr_) {
      ptr_->Release();
    }
    ptr_ = new_ptr;
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


}  // namespace dhc
