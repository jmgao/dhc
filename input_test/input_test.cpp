#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <string>

#include <dhc/dhc.h>
#include <dhc/logging.h>
#include <dhc/utils.h>

using namespace std::chrono_literals;

static void PrintAxis(dhc::observer_ptr<dhc::Device> device, const char* name, dhc::AxisType type,
                      double min = -1.0, double max = 1.0) {
  auto& axis = device->Axes()[type];
  printf("%-10s %f\n", name, dhc::lerp(axis.value, min, max));
}

static void ClearScreen() {
  const char* ansi_clear = "\033[2J\033[H";
  fwrite(ansi_clear, strlen(ansi_clear), 1, stdout);
  fflush(stdout);
}

int main() {
  dhc::logging::SetMinimumLogSeverity(dhc::logging::INFO);

  auto ctx = dhc::Context::GetInstance();
  auto device = ctx->GetDevice(0);
  CHECK(device != nullptr);
  for (unsigned long i = 0;; ++i) {
    printf("Tick %lu\n", i);
    device->Update();
    PrintAxis(device, "LS-X", dhc::AxisType::LStickX);
    PrintAxis(device, "LS-Y", dhc::AxisType::LStickY);
    PrintAxis(device, "RS-X", dhc::AxisType::RStickX);
    PrintAxis(device, "RS-Y", dhc::AxisType::RStickY);
    PrintAxis(device, "LT", dhc::AxisType::LTrigger, 0.0, 1.0);
    PrintAxis(device, "RT", dhc::AxisType::RTrigger, 0.0, 1.0);
    fflush(stdout);

    usleep(16'666);
    ClearScreen();
  }
}
