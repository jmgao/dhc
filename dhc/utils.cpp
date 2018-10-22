#include <windows.h>

#include "dhc/utils.h"

#include <codecvt>
#include <locale>
#include <string>

#include "dhc/dhc.h"
#include "dhc/logging.h"

namespace dhc {

static std::wstring GetSystemDirectory() {
  std::wstring system_directory(MAX_PATH, L'\0');

  while (true) {
    size_t rc = ::GetSystemDirectoryW(&system_directory[0],
                                      static_cast<unsigned int>(system_directory.size()));
    if (rc == 0) {
      LOG(FATAL) << "failed to get system directory";
    }

    size_t orig_size = system_directory.size();
    system_directory.resize(rc);
    if (rc <= orig_size) {
      break;
    }
  }

  return system_directory;
}

std::string to_string(const std::string& str) {
  return str;
}

std::string to_string(const std::wstring& wstr) {
  using convert_type = std::codecvt_utf8<wchar_t>;
  std::wstring_convert<convert_type, wchar_t> converter;
  return converter.to_bytes(wstr);
}

std::wstring to_wstring(const std::string& str) {
  using convert_type = std::codecvt_utf8<wchar_t>;
  std::wstring_convert<convert_type, wchar_t> converter;
  return converter.from_bytes(str);
}

std::wstring to_wstring(const std::wstring& wstr) {
  return wstr;
}

std::string_view dierr_to_string(HRESULT result) {
  switch (result) {
    case DI_OK:
      return "DI_OK";
    case DIERR_INPUTLOST:
      return "DIERR_INPUTLOST";
    case DIERR_INVALIDPARAM:
      return "DIERR_INVALIDPARAM";
    case DIERR_NOTACQUIRED:
      return "DIERR_NOTACQUIRED";
    case DIERR_NOTINITIALIZED:
      return "DIERR_NOTINITIALIZED";
    case E_PENDING:
      return "E_PENDING";
    default:
      return "<unknown>";
  }
}

HMODULE LoadSystemLibrary(const std::wstring& name) {
  std::wstring loaded_module_path;
  std::wstring path = GetSystemDirectory() + L"\\" + name;

  HMODULE result = LoadLibraryW(path.c_str());
  if (result) {
    LOG(INFO) << "loaded system library " << to_string(path);
  } else {
    LOG(ERROR) << "failed to load system library " << to_string(path);
  }
  return result;
}

FARPROC WINAPI GetDirectInput8Proc(const char* proc_name) {
  static HMODULE real = LoadSystemLibrary(L"dinput8.dll");
  return GetProcAddress(real, proc_name);
}

static HRESULT RealDirectInput8Create(HINSTANCE hinst, DWORD version, REFIID desired_interface,
                                      void** out_interface, IUnknown* unknown) {
  static auto real =
      reinterpret_cast<decltype(&DirectInput8Create)>(GetDirectInput8Proc("DirectInput8Create"));
  return real(hinst, version, desired_interface, out_interface, unknown);
}

com_ptr<IDirectInput8W> GetRealDirectInput8W() {
  void* iface;
  HRESULT rc = RealDirectInput8Create(HINST_SELF, 0x0800, IID_IDirectInput8W, &iface, nullptr);
  CHECK_EQ(DI_OK, rc);
  return com_ptr<IDirectInput8W>(static_cast<IDirectInput8W*>(iface));
}

com_ptr<IDirectInput8A> GetRealDirectInput8A() {
  void* iface;
  HRESULT rc = RealDirectInput8Create(HINST_SELF, 0x0800, IID_IDirectInput8A, &iface, nullptr);
  CHECK_EQ(DI_OK, rc);
  return com_ptr<IDirectInput8A>(static_cast<IDirectInput8A*>(iface));
}

}  // namespace dhc
