#pragma once

#ifdef DHC_EXPORTS
#define DHC_API __declspec(dllexport)
#else
#define DHC_API __declspec(dllimport)
#endif

#if defined(_MSC_VER)
#define MSVC_SUPPRESS(warnings) __pragma(warning(suppress: warnings))
#else
#define MSVC_SUPPRESS(warnings)
#endif
