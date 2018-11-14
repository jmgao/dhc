#pragma once

#include <deque>
#include <vector>

#include "dhc/backend/input_provider.h"
#include "dhc/types.h"
#include "dhc/utils.h"

namespace dhc {

struct DinputProvider;

struct DeviceAssignment {
  observer_ptr<Device> virtual_device_;
  com_ptr<IDirectInputDevice8A> real_device_;
  GUID real_device_guid_;
  std::string real_device_name_;
  observer_ptr<DinputProvider> provider_;
};

struct DinputProvider : public InputProvider {
  DinputProvider();
  virtual ~DinputProvider();

  virtual void Assign(observer_ptr<Device> device) override final REQUIRES(!mutex_)
      REQUIRES(!scan_mutex_);
  virtual void Revoke(observer_ptr<Device> device) override final REQUIRES(!mutex_)
      REQUIRES(!scan_mutex_);
  virtual void Refresh(observer_ptr<Device> device) override final REQUIRES(!mutex_)
      REQUIRES(!scan_mutex_);

  void ScannerThread() REQUIRES(!mutex_);
  bool EnumerateDevice(observer_ptr<const DIDEVICEINSTANCEA> device) REQUIRES(!mutex_)
      REQUIRES(!scan_mutex_);

  void AssignDevices() REQUIRES(mutex_);

 private:
  observer_ptr<DeviceAssignment> FindAssignment(GUID real_device_guid) REQUIRES(!mutex_);
  observer_ptr<DeviceAssignment> FindAssignmentLocked(GUID real_device_guid) REQUIRES(mutex_);

  observer_ptr<DeviceAssignment> FindAssignment(observer_ptr<Device> virtual_device)
      REQUIRES(!mutex_);
  observer_ptr<DeviceAssignment> FindAssignmentLocked(observer_ptr<Device> virtual_device)
      REQUIRES(mutex_);

  void Refresh(observer_ptr<DeviceAssignment> assignment) REQUIRES(!mutex_) REQUIRES(!scan_mutex_);
  void Release(observer_ptr<DeviceAssignment> assignment) REQUIRES(!mutex_) REQUIRES(!scan_mutex_);

  com_ptr<IDirectInput8A> di_;

 public:
  mutex mutex_;
  mutex scan_mutex_ ACQUIRED_BEFORE(mutex_);

 private:
  HANDLE scanner_thread_ = INVALID_HANDLE_VALUE;
  std::vector<GUID> opened_device_guids_ GUARDED_BY(scan_mutex_);

  std::deque<observer_ptr<Device>> available_virtual_devices_ GUARDED_BY(mutex_);
  std::deque<std::unique_ptr<DeviceAssignment>> available_physical_devices_ GUARDED_BY(mutex_);
  std::vector<std::unique_ptr<DeviceAssignment>> assignments_ GUARDED_BY(mutex_);
};

}  // namespace dhc
