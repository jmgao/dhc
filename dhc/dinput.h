#pragma once

#include <dinput.h>

#include "dhc.h"

namespace dhc {

DHC_API FARPROC WINAPI GetDirectInput8Proc(const char* proc_name);

DHC_API IDirectInput8W* GetEmulatedDirectInput8W();
DHC_API IDirectInput8A* GetEmulatedDirectInput8A();

}  // namespace dhc
