#include <windows.h>

#include "dhc/backend/dinput.h"
#include "dhc/backend/input_provider.h"
#include "dhc/dhc.h"
#include "dhc/logging.h"

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

namespace dhc {

REFGUID AxisToGUID(AxisType axis_type) {
  switch (axis_type) {
    case AxisType::LStickX:
      return GUID_XAxis;
    case AxisType::LStickY:
      return GUID_YAxis;
    case AxisType::RStickX:
      return GUID_ZAxis;
    case AxisType::RStickY:
      return GUID_RzAxis;
    case AxisType::LTrigger:
      return GUID_RxAxis;
    case AxisType::RTrigger:
      return GUID_RyAxis;
    default:
      LOG(FATAL) << "unknown AxisType: " << static_cast<long>(axis_type);
  }
  __builtin_unreachable();
}


Context::Context(std::unique_ptr<InputProvider> provider) : provider_(std::move(provider)) {
  devices_[0] = std::make_unique<Device>(observer_ptr<Context>(this), 0);
  devices_[1] = std::make_unique<Device>(observer_ptr<Context>(this), 1);
  provider_->Assign(observer_ptr<Device>(devices_[0].get()));
  provider_->Assign(observer_ptr<Device>(devices_[1].get()));
}

observer_ptr<Context> Context::GetInstance() {
  static Context ctx(std::make_unique<DinputProvider>());
  return observer_ptr<Context>(&ctx);
}

observer_ptr<Device> Context::GetDevice(size_t id) {
  if (id >= 2) {
    return nullptr;
  }
  return observer_ptr<Device>(devices_[id].get());
}

void Device::Reset() {
  for (auto &stick : {AxisType::LStickX, AxisType::LStickY, AxisType::RStickX, AxisType::RStickY}) {
    axes_[stick].value = 0.5;
  }

  for (auto &trigger : {AxisType::LTrigger, AxisType::RTrigger}) {
    axes_[trigger].value = 0.0;
  }

  povs_[DPad].state = PovState::Neutral;
}

void Device::Update() {
  LOG(VERBOSE) << "Device(" << id_ << ")::Update()";
  provider_->Refresh(observer_ptr<Device>(this));
}

}  // namespace dhc
