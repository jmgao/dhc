#pragma once

#include <memory>
#include <string>
#include <vector>

#ifdef DHC_EXPORTS
#define DHC_API __declspec(dllexport)
#else
#define DHC_API __declspec(dllimport)
#endif

DHC_API HMODULE LoadSystemLibrary(const std::wstring& name);