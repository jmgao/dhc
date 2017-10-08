#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX 1
#include <windows.h>

#include <memory>
#include <string>
#include <vector>

#ifdef DHC_EXPORTS
#define DHC_API __declspec(dllexport)
#else
#define DHC_API __declspec(dllimport)
#endif

namespace dhc {

static constexpr GUID GUID_DHC_P1 = {
  0xdead571c,
  0x4efc,
  0x9fa7,
  {
    0x9a, 0x7e, 0x8d, 0x10,
    0x00, 0x00, 0x00, 0x01
  }
};

static constexpr GUID GUID_DHC_P2 = {
  0xdead571c,
  0x4efc,
  0x9fa7,
  {
    0x9a, 0x7e, 0x8d, 0x10,
    0x00, 0x00, 0x00, 0x02
  }
};

}
