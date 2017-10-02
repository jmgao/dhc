#pragma once

#include <dinput.h>

#include "dhc.h"

namespace dhc {

DHC_API IDirectInput8W* GetEmulatedDirectInput8W();
DHC_API IDirectInput8A* GetEmulatedDirectInput8A();

}  // namespace dhc