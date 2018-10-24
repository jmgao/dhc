#include <dinput.h>
#include <windows.h>

#include <stdarg.h>
#include <stdio.h>

#include <iostream>
#include <vector>

extern "C" IMAGE_DOS_HEADER __ImageBase;
#define HINST_SELF ((HINSTANCE)&__ImageBase)

[[noreturn]] void __attribute__((__format__(printf, 2, 3))) errx(int rc, const char* fmt, ...) {
  va_list va;
  va_start(va, fmt);
  vfprintf(stderr, fmt, va);
  va_end(va);
  fprintf(stderr, "\n");
  fflush(stderr);
  exit(rc);
}

static IDirectInput8A* di;
static WINAPI int EnumerateDevices(const DIDEVICEINSTANCEA* device, void* out_device) {
  printf("Found device: %s\n", device->tszInstanceName);
  HRESULT rc = di->CreateDevice(device->guidInstance,
                                static_cast<IDirectInputDevice8A**>(out_device), nullptr);
  if (rc != DI_OK) {
    errx(1, "IDirectInput8A::CreateDevice failed");
  }
  printf("Device created...\n");
  return DIENUM_STOP;
}

#define FIELDS           \
  FIELD(dummy)           \
  FIELD(X)               \
  FIELD(Y)               \
  FIELD(Z)               \
  FIELD(Rx)              \
  FIELD(Ry)              \
  FIELD(Rz)              \
  FIELD(POV1)            \
  FIELD(POV2)            \
  FIELD(X_Velocity)      \
  FIELD(X_Accel)         \
  FIELD(X_Force)         \
  FIELD(Y_Velocity)      \
  FIELD(Y_Accel)         \
  FIELD(Y_Force)         \
  FIELD(Z_Velocity)      \
  FIELD(Z_Accel)         \
  FIELD(Z_Force)         \
  FIELD(Rx_Velocity)     \
  FIELD(Rx_Accel)        \
  FIELD(Rx_Force)        \
  FIELD(Ry_Velocity)     \
  FIELD(Ry_Accel)        \
  FIELD(Ry_Force)        \
  FIELD(Rz_Velocity)     \
  FIELD(Rz_Accel)        \
  FIELD(Rz_Force)        \
  FIELD(Slider)          \
  FIELD(Slider_Velocity) \
  FIELD(Slider_Accel)    \
  FIELD(Slider_Force)    \
  FIELD(extra)

#define FIELD(x) DWORD x;
struct Data {
  char buttons[32];
  FIELDS
};
#undef FIELD

std::string dierr_to_string(HRESULT result) {
  switch (result) {
    case DI_OK:
      return "DI_OK";
    case DIERR_INPUTLOST:
      return "DIERR_INPUTLOST";
    case DIERR_INVALIDPARAM:
      return "DIERR_INVALIDPARAM";
    case DIERR_NOTACQUIRED:
      return "DIERR_NOTACQUIRED";
    case DIERR_NOTINITIALIZED:
      return "DIERR_NOTINITIALIZED";
    case E_PENDING:
      return "E_PENDING";
    default:
      return "<unknown>";
  }
}

int main() {
  void* iface = nullptr;
  HRESULT rc = DirectInput8Create(HINST_SELF, 0x0800, IID_IDirectInput8A, &iface, nullptr);
  if (rc != DI_OK) {
    errx(1, "DirectInput8Create failed");
  }

  di = static_cast<IDirectInput8A*>(iface);
  IDirectInputDevice8A* device = nullptr;
  rc = di->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumerateDevices, &device, DIEDFL_ATTACHEDONLY);
  if (!device) {
    errx(1, "failed to create device");
  }

  std::vector<DIOBJECTDATAFORMAT> object_data_formats;
  object_data_formats.push_back(
      {&GUID_POV, offsetof(Data, POV1), DIDFT_POV | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, 0});

  object_data_formats.push_back(
      {&GUID_POV, offsetof(Data, POV2), DIDFT_POV | DIDFT_ANYINSTANCE | DIDFT_OPTIONAL, 0});

#define AXES(guid, prefix)                                                        \
  object_data_formats.push_back({&guid, offsetof(Data, prefix),                   \
                                 DIDFT_AXIS | DIDFT_OPTIONAL | DIDFT_ANYINSTANCE, \
                                 DIDOI_ASPECTPOSITION});                          \
  object_data_formats.push_back({&guid, offsetof(Data, prefix##_Velocity),        \
                                 DIDFT_AXIS | DIDFT_OPTIONAL | DIDFT_ANYINSTANCE, \
                                 DIDOI_ASPECTVELOCITY});                          \
  object_data_formats.push_back({&guid, offsetof(Data, prefix##_Accel),           \
                                 DIDFT_AXIS | DIDFT_OPTIONAL | DIDFT_ANYINSTANCE, \
                                 DIDOI_ASPECTACCEL});                             \
  object_data_formats.push_back({&guid, offsetof(Data, prefix##_Force),           \
                                 DIDFT_AXIS | DIDFT_OPTIONAL | DIDFT_ANYINSTANCE, \
                                 DIDOI_ASPECTFORCE})

  AXES(GUID_XAxis, X);
  AXES(GUID_YAxis, Y);
  AXES(GUID_ZAxis, Z);
  AXES(GUID_RxAxis, Rx);
  AXES(GUID_RyAxis, Ry);
  AXES(GUID_RzAxis, Rz);
  AXES(GUID_Slider, Slider);

  for (int i = 0; i < 32; ++i) {
    object_data_formats.push_back({&GUID_Button, offsetof(Data, buttons) + i,
                                   DIDFT_AXIS | DIDFT_OPTIONAL | DIDFT_MAKEINSTANCE(i), 0});
  }

  DIDATAFORMAT data_format;
  data_format.dwSize = sizeof(data_format);
  data_format.dwObjSize = sizeof(DIOBJECTDATAFORMAT);
  data_format.dwFlags = DIDF_ABSAXIS;
  data_format.dwDataSize = sizeof(Data);
  data_format.dwNumObjs = object_data_formats.size();
  data_format.rgodf = object_data_formats.data();

  rc = device->SetDataFormat(&data_format);
  if (rc != DI_OK) {
    errx(1, "failed to SetDataFormat");
  }

  printf("Data format set\n");
  DIPROPRANGE range;
  range.diph.dwHeaderSize = sizeof(range.diph);
  range.diph.dwSize = sizeof(range);
  range.diph.dwHow = DIPH_BYID;
  range.diph.dwObj = 32770;
  range.lMin = -1000;
  range.lMax = 1000;
  rc = device->SetProperty(DIPROP_RANGE, &range.diph);
  if (rc != DI_OK) {
    errx(1, "failed to SetProperty(DIPROP_RANGE)");
  }

  device->Acquire();

  char fill = 0;
  while (true) {
    Data data;
    memset(&data, fill, sizeof(data));
    fill = ~fill;
    printf("fill = %d\n", fill);

    rc = device->Poll();
    if (rc != DI_OK) {
      printf("Poll failed: %s\n", dierr_to_string(rc).c_str());
    }

    rc = device->GetDeviceState(sizeof(data), &data);
    if (rc != DI_OK) {
      errx(1, "GetDeviceState failed: %s", dierr_to_string(rc).c_str());
    }

#define FIELD(x) printf("%s = %ld\n", #x, data.x);
    FIELDS

    std::string input;
    std::getline(std::cin, input);
  };
}
