#include <dinput.h>
#include <windows.h>

#include <algorithm>
#include <chrono>

#include "dhc/backend/dinput.h"
#include "dhc/logging.h"
#include "dhc/utils.h"

using namespace std::chrono_literals;

namespace dhc {

static WINAPI DWORD ScannerThreadTrampoline(void* self) {
  static_cast<DinputProvider*>(self)->ScannerThread();
  return 0;
}

DinputProvider::DinputProvider() {
  di_ = GetRealDirectInput8A();
  scanner_thread_ = CreateThread(nullptr, 0, ScannerThreadTrampoline, this, 0, nullptr);
  CHECK_NE(scanner_thread_, INVALID_HANDLE_VALUE);
}

DinputProvider::~DinputProvider() {}

void DinputProvider::Assign(observer_ptr<Device> device) {
  device->provider_ = observer_ptr<InputProvider>(this);

  lock_guard<mutex> lock(mutex_);
  available_devices_.push_back(device);
}

void DinputProvider::Revoke(observer_ptr<Device> device) { UNIMPLEMENTED(FATAL); }

void DinputProvider::Refresh(observer_ptr<Device> device) {
  mutex_.lock();
  auto assignment = FindAssignmentLocked(device);
  if (!assignment) {
    // Check to make sure we're actually waiting for the device.
    auto it = std::find(available_devices_.begin(), available_devices_.end(), device);
    CHECK(it != available_devices_.end());

    mutex_.unlock();
    return;
  }
  mutex_.unlock();

  Refresh(assignment);
}

observer_ptr<DeviceAssignment> DinputProvider::FindAssignment(GUID real_device_guid) {
  lock_guard<mutex> lock(mutex_);
  return FindAssignmentLocked(real_device_guid);
}

observer_ptr<DeviceAssignment> DinputProvider::FindAssignmentLocked(GUID real_device_guid) {
  for (auto& assignment : assignments_) {
    if (assignment->real_device_guid_ == real_device_guid) {
      return observer_ptr<DeviceAssignment>(assignment.get());
    }
  }
  return nullptr;
}

observer_ptr<DeviceAssignment> DinputProvider::FindAssignment(observer_ptr<Device> virtual_device) {
  lock_guard<mutex> lock(mutex_);
  return FindAssignmentLocked(virtual_device);
}

observer_ptr<DeviceAssignment> DinputProvider::FindAssignmentLocked(
    observer_ptr<Device> virtual_device) {
  for (auto& assignment : assignments_) {
    if (assignment->virtual_device_ == virtual_device) {
      return observer_ptr<DeviceAssignment>(assignment.get());
    }
  }
  return nullptr;
}

static WINAPI BOOL EnumerateDeviceTrampoline(const DIDEVICEINSTANCEA* device, void* arg) {
  auto self = reinterpret_cast<DinputProvider*>(arg);
  auto dev = observer_ptr<const DIDEVICEINSTANCEA>(device);
  return self->EnumerateDevice(dev) ? DIENUM_CONTINUE : DIENUM_STOP;
}

void DinputProvider::ScannerThread() {
  LOG(INFO) << "DinputProvider::ScannerThread started";
  while (true) {
    auto begin = std::chrono::high_resolution_clock::now();
    di_->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumerateDeviceTrampoline, this, DIEDFL_ATTACHEDONLY);
    auto end = std::chrono::high_resolution_clock::now();
    LOG(DEBUG) << "finished scanning in " << (end - begin) / 1.0ms << "ms";

    // TODO: Should this use RegisterDeviceNotificationA instead?
    Sleep(250);
  }
  LOG(INFO) << "DinputProvider::ScannerThread exiting";
}

struct DeviceObjectCounts {
  size_t axes = 0;
  size_t buttons = 0;
  size_t pov_hats = 0;
};

static WINAPI BOOL EnumerateObjectCallback(const DIDEVICEOBJECTINSTANCE* object, void* arg) {
  auto* counts = static_cast<DeviceObjectCounts*>(arg);
  if ((object->dwType & DIDFT_AXIS)) {
    ++counts->axes;
  } else if ((object->dwType & DIDFT_BUTTON)) {
    ++counts->buttons;
  } else if ((object->dwType & DIDFT_POV)) {
    ++counts->pov_hats;
  }

  LOG(INFO) << "Object " << object->tszName;
  LOG(INFO) << "  GUID: " << to_string(object->guidType);
  LOG(INFO) << "  Type: " << didft_to_string(object->dwType);
  LOG(INFO) << "  Flags: " << didoi_to_string(object->dwFlags);
  return true;
}

bool DinputProvider::EnumerateDevice(observer_ptr<const DIDEVICEINSTANCEA> device) {
  lock_guard<mutex> scan_lock(scan_mutex_);

  if (std::find(opened_device_guids_.begin(), opened_device_guids_.end(), device->guidInstance) !=
      opened_device_guids_.end()) {
    LOG(VERBOSE) << "skipping already-assigned device " << device->tszInstanceName;
    return true;
  }

  // TODO: Check to make sure this is actually a suitable device?
  com_ptr<IDirectInputDevice8A> real_device;
  HRESULT rc = di_->CreateDevice(device->guidInstance, real_device.receive(), nullptr);
  if (rc != DI_OK) {
    LOG(ERROR) << "failed to CreateDevice(" << device->tszInstanceName
               << "): " << dierr_to_string(rc);
    return true;
  }

  opened_device_guids_.push_back(device->guidInstance);
  DeviceObjectCounts counts;
  rc = real_device->EnumObjects(EnumerateObjectCallback, &counts, DIDFT_ALL);
  if (rc != DI_OK) {
    LOG(ERROR) << "failed to EnumObjects on device " << device->tszInstanceName << ": "
               << dierr_to_string(rc);
    return true;
  }

  if (counts.axes == 6 && counts.buttons == 14 && counts.pov_hats == 1) {
    LOG(INFO) << "Enumerated " << device->tszInstanceName << " objects: PS4 controller";
  } else if (counts.axes == 4 && counts.buttons == 13 && counts.pov_hats == 1) {
    LOG(INFO) << "Enumerated " << device->tszInstanceName << " objects: PS3 controller";
  } else {
    LOG(INFO) << "Enumerated " << device->tszInstanceName << " objects: unknown";
    LOG(INFO) << "  Axes: " << counts.axes;
    LOG(INFO) << "  Buttons: " << counts.buttons;
    LOG(INFO) << "  POV Hats: " << counts.pov_hats;
    if (counts.axes == 0 && counts.buttons == 0 && counts.pov_hats == 0) {
      LOG(INFO) << "Skipping empty device...";
      return true;
    }
  }

  // TODO: Should we create our own DIDATAFORMAT?
  rc = real_device->SetDataFormat(&c_dfDIJoystick);
  if (rc != DI_OK) {
    LOG(ERROR) << "failed to SetDataFormat on device " << device->tszInstanceName << ":"
               << dierr_to_string(rc);
    return true;
  }

  rc = real_device->Acquire();
  if (rc != DI_OK) {
    LOG(ERROR) << "failed to Acquire device " << device->tszInstanceName << ": "
               << dierr_to_string(rc);
    return true;
  }

  lock_guard<mutex> lock(mutex_);

  // TODO: We probably shouldn't be blindly assuming that this is a PS4 controller.
  auto assignment = std::make_unique<DeviceAssignment>();
  assignment->real_device_ = std::move(real_device);
  assignment->real_device_guid_ = device->guidInstance;
  assignment->virtual_device_ = available_devices_.front();
  assignment->provider_ = observer_ptr<DinputProvider>(this);
  available_devices_.pop_front();

  LOG(INFO) << "assigning device " << assignment->virtual_device_->id_ << " to "
            << device->tszInstanceName;
  assignments_.push_back(std::move(assignment));
  return true;
}

static PovState PovStateFromDword(DWORD pov) {
  auto state = static_cast<PovState>(pov);
  switch (state) {
    case PovState::Neutral:
    case PovState::North:
    case PovState::NorthEast:
    case PovState::East:
    case PovState::SouthEast:
    case PovState::South:
    case PovState::SouthWest:
    case PovState::West:
    case PovState::NorthWest:
      return state;

    default:
      LOG(FATAL) << "can't translate POV-hat orientation " << pov;
  }

  __builtin_unreachable();
}

void DinputProvider::Refresh(observer_ptr<DeviceAssignment> assignment) {
  HRESULT rc = assignment->real_device_->Poll();
  if (rc != DI_OK && rc != DI_NOEFFECT) {
    LOG(ERROR) << "failed to Poll on device " << assignment->virtual_device_->Id() << ": "
               << dierr_to_string(rc);
    Release(assignment);
    return;
  }

  struct DIJOYSTATE device_state;
  rc = assignment->real_device_->GetDeviceState(sizeof(device_state), &device_state);
  if (rc != DI_OK) {
    LOG(ERROR) << "failed to GetDeviceState on device " << assignment->virtual_device_->Id() << ": "
               << dierr_to_string(rc);
    Release(assignment);
    return;
  }

  // TODO: We probably shouldn't be blindly assuming that this is a PS4 controller...
  auto& axes = assignment->virtual_device_->axes_;
  axes[AxisType::LStickX].value = device_state.lX / 65535.0;
  axes[AxisType::LStickY].value = device_state.lY / 65535.0;
  axes[AxisType::RStickX].value = device_state.lZ / 65535.0;
  axes[AxisType::RStickY].value = device_state.lRz / 65535.0;
  axes[AxisType::LTrigger].value = device_state.lRx / 65535.0;
  axes[AxisType::RTrigger].value = device_state.lRy / 65535.0;

  auto& buttons = assignment->virtual_device_->buttons_;
  buttons[ButtonType::Square].pressed = device_state.rgbButtons[0];
  buttons[ButtonType::Cross].pressed = device_state.rgbButtons[1];
  buttons[ButtonType::Circle].pressed = device_state.rgbButtons[2];
  buttons[ButtonType::Triangle].pressed = device_state.rgbButtons[3];
  buttons[ButtonType::L1].pressed = device_state.rgbButtons[4];
  buttons[ButtonType::R1].pressed = device_state.rgbButtons[5];
  buttons[ButtonType::L2].pressed = device_state.rgbButtons[6];
  buttons[ButtonType::R2].pressed = device_state.rgbButtons[7];
  buttons[ButtonType::Share].pressed = device_state.rgbButtons[8];
  buttons[ButtonType::Options].pressed = device_state.rgbButtons[9];
  buttons[ButtonType::L3].pressed = device_state.rgbButtons[10];
  buttons[ButtonType::R3].pressed = device_state.rgbButtons[11];
  buttons[ButtonType::PlayStation].pressed = device_state.rgbButtons[12];
  buttons[ButtonType::Trackpad].pressed = device_state.rgbButtons[13];

  auto& dpad = assignment->virtual_device_->povs_[PovType::DPad];
  dpad.state = PovStateFromDword(device_state.rgdwPOV[0]);
}

void DinputProvider::Release(observer_ptr<DeviceAssignment> assignment) {
  lock_guard<mutex> scan_lock(scan_mutex_);
  lock_guard<mutex> lock(mutex_);

  assignment->real_device_->Unacquire();

  auto vdev = assignment->virtual_device_;
  available_devices_.push_front(vdev);
  std::sort(available_devices_.begin(), available_devices_.end(),
            [](observer_ptr<Device> lhs, observer_ptr<Device> rhs) { return lhs->id_ < rhs->id_; });

  opened_device_guids_.erase(std::remove(opened_device_guids_.begin(), opened_device_guids_.end(),
                                         assignment->real_device_guid_),
                             opened_device_guids_.end());

  for (auto it = assignments_.begin(); it != assignments_.end(); ++it) {
    if (it->get() == assignment.get()) {
      assignments_.erase(it);
      break;
    }
  }

  vdev->Reset();
}

}  // namespace dhc
