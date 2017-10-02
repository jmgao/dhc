#pragma once

#include "dhc.h"

namespace dhc {

DHC_API HMODULE LoadSystemLibrary(const std::wstring& name);

DHC_API std::string to_string(const std::string& str);
DHC_API std::string to_string(const std::wstring& wstr);

}  // namespace dhc