#include <windows.h>
#include <dinput.h>

#include "dhc/utils.h"

#include <codecvt>
#include <deque>
#include <locale>
#include <string>

#include "dhc/dhc.h"
#include "dhc/logging.h"

using namespace std::string_literals;

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
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
  return converter.to_bytes(wstr);
}

std::wstring to_wstring(const std::string& str) {
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
  return converter.from_bytes(str.data());
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

std::string to_string(REFGUID guid) {
  if (guid == GUID_SysKeyboard) {
    return "GUID_SysKeyboard";
  } else if (guid == GUID_SysMouse) {
    return "GUID_SysMouse";
  } else if (guid == GUID_DHC_P1) {
    return "GUID_DHC_P1";
  } else if (guid == GUID_DHC_P2) {
    return "GUID_DHC_P2";
  } else if (guid == GUID_XAxis) {
    return "GUID_XAxis";
  } else if (guid == GUID_YAxis) {
    return "GUID_YAxis";
  } else if (guid == GUID_ZAxis) {
    return "GUID_ZAxis";
  } else if (guid == GUID_RxAxis) {
    return "GUID_RxAxis";
  } else if (guid == GUID_RyAxis) {
    return "GUID_RyAxis";
  } else if (guid == GUID_RzAxis) {
    return "GUID_RzAxis";
  } else if (guid == GUID_Slider) {
    return "GUID_Slider";
  } else if (guid == GUID_Button) {
    return "GUID_Button";
  } else if (guid == GUID_Key) {
    return "GUID_Key";
  } else if (guid == GUID_POV) {
    return "GUID_POV";
  } else if (guid == GUID_Unknown) {
    return "GUID_Unknown";
  } else {
    char guid_string[37];  // 32 hex chars + 4 hyphens + null terminator
    snprintf(guid_string, sizeof(guid_string) / sizeof(guid_string[0]),
             "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", guid.Data1, guid.Data2,
             guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4],
             guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return guid_string;
  }
}

std::string didft_to_string(DWORD type) {
  std::deque<std::string> result;
  if ((type & DIDFT_AXIS) == DIDFT_AXIS) {
    result.push_back("DIDFT_AXIS");
  } else if ((type & DIDFT_AXIS) == DIDFT_ABSAXIS) {
    result.push_back("DIDFT_ABSAXIS");
  } else if ((type & DIDFT_AXIS) == DIDFT_RELAXIS) {
    result.push_back("DIDFT_RELAXIS");
  }
  type &= ~DIDFT_AXIS;

  if ((type & DIDFT_BUTTON) == DIDFT_BUTTON) {
    result.push_back("DIDFT_BUTTON");
  } else if ((type & DIDFT_BUTTON) == DIDFT_PSHBUTTON) {
    result.push_back("DIDFT_PSHBUTTON");
  } else if ((type & DIDFT_BUTTON) == DIDFT_TGLBUTTON) {
    result.push_back("DIDFT_TGLBUTTON");
  }
  type &= ~DIDFT_BUTTON;

  if ((type & DIDFT_POV)) {
    result.push_back("DIDFT_POV");
  }
  type &= ~DIDFT_POV;

  if ((type & DIDFT_VENDORDEFINED)) {
    result.push_back("DIDFT_VENDORDEFINED");
  }
  type &= ~DIDFT_VENDORDEFINED;

  if ((type & DIDFT_COLLECTION)) {
    result.push_back("DIDFT_COLLECTION");
  }
  type &= ~DIDFT_COLLECTION;

  if ((type & DIDFT_NODATA)) {
    result.push_back("DIDFT_NODATA");
  }
  type &= ~DIDFT_NODATA;

  if ((type & DIDFT_OPTIONAL)) {
    result.push_back("DIDFT_OPTIONAL");
  }
  type &= ~DIDFT_OPTIONAL;

  if ((type & DIDFT_ANYINSTANCE) == DIDFT_ANYINSTANCE) {
    result.push_back("DIDFT_ANYINSTANCE");
  } else if ((type & DIDFT_ANYINSTANCE)) {
    int instance = DIDFT_GETINSTANCE(type);
    result.push_back("DIDFT_MAKE_INSTANCE("s + std::to_string(instance) + ")");
  }
  type &= ~DIDFT_ANYINSTANCE;

  if (type) {
    result.push_back("<unknown value "s + std::to_string(static_cast<uint32_t>(type)) + ">");
  }

  if (result.empty()) {
    return "0";
  }
  return Join(result, " | ");
}

std::string didoi_to_string(DWORD flags) {
  std::deque<std::string> result;
  if ((flags & DIDOI_ASPECTACCEL) == DIDOI_ASPECTACCEL) {
    result.push_back("DIDOI_ASPECTACCEL");
  } else if ((flags & DIDOI_ASPECTPOSITION)) {
    result.push_back("DIDOI_ASPECTPOSITION");
  } else if ((flags & DIDOI_ASPECTVELOCITY)) {
    result.push_back("DIDOI_ASPECTVELOCITY");
  }
  flags &= ~DIDOI_ASPECTACCEL;

  if ((flags & DIDOI_ASPECTFORCE)) {
    result.push_back("DIDOI_ASPECTFORCE");
  }
  flags &= ~DIDOI_ASPECTFORCE;

  if ((flags & DIDOI_POLLED)) {
    result.push_back("DIDOI_POLLED");
  }
  flags &= ~DIDOI_POLLED;

  if ((flags & DIDOI_FFACTUATOR)) {
    result.push_back("DIDOI_FFACTUATOR");
  }
  flags &= ~DIDOI_FFACTUATOR;

  if ((flags & DIDOI_FFEFFECTTRIGGER)) {
    result.push_back("DIDOI_FFEFFECTTRIGGER");
  }
  flags &= ~DIDOI_FFEFFECTTRIGGER;

  if (flags) {
    result.push_back("<unknown value "s + std::to_string(static_cast<uint32_t>(flags)) + ">");
  }

  if (result.empty()) {
    return "0";
  }
  return Join(result, " | ");
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
