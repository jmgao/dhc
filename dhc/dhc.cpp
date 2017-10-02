#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "dhc.h"
#include "logging.h"

using namespace dhc::logging;

BOOL WINAPI DllMain(HMODULE module, DWORD reason, void*) {
  switch (reason) {
    case DLL_PROCESS_ATTACH:
      DisableThreadLibraryCalls(module);
      InitLogging(nullptr);
      CreateLogConsole();
      CreateLogFile("log.txt");
      SetMinimumLogSeverity(VERBOSE);
      break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
      break;
  }
  return TRUE;
}
