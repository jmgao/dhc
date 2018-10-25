#pragma once

#include "dhc/dhc.h"
#include "dhc/types.h"

namespace dhc {

struct InputProvider {
  InputProvider() = default;
  InputProvider(const InputProvider& copy) = delete;
  InputProvider(InputProvider&& move) = delete;

  virtual ~InputProvider() = default;

  virtual void Assign(observer_ptr<Device> device) = 0;
  virtual void Revoke(observer_ptr<Device> device) = 0;
  virtual void Refresh(observer_ptr<Device> device) = 0;
};

}  // namespace dhc
