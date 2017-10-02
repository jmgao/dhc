#include <codecvt>
#include <locale>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "dhc.h"
#include "logging.h"
#include "utils.h"

namespace dhc {

static std::wstring GetSystemDirectory() {
  std::wstring system_directory(MAX_PATH, L'\0');

  while (true) {
    size_t rc = ::GetSystemDirectoryW(&system_directory[0], system_directory.size());
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

}  // namespace dhc