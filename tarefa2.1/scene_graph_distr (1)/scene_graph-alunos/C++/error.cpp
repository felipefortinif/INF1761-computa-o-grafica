#include "error.h"
#include <cstdio>
#include <cstdlib>

void Error::Fatal (const std::string& msg)
{
  fprintf(stderr, "error: %s\n", msg.c_str());
  exit(1);
}

static void ErrorCallback (WGPUDevice const* device, WGPUErrorType type,
                            WGPUStringView msg, void* ud1, void* ud2)
{
  (void) device; (void) ud1; (void) ud2;
  Error::Fatal("wgpu uncaptured error (type=" + std::to_string((int) type) + "): " +
               std::string(msg.data, msg.length));
}

static void DeviceLostCallback (WGPUDevice const* device, WGPUDeviceLostReason reason,
                                 WGPUStringView msg, void* ud1, void* ud2)
{
  (void) device; (void) ud1; (void) ud2;
  Error::Fatal("wgpu device lost (reason=" + std::to_string((int) reason) + "): " +
               std::string(msg.data, msg.length));
}

WGPUUncapturedErrorCallbackInfo Error::MakeErrorCallbackInfo ()
{
  WGPUUncapturedErrorCallbackInfo info = {};
  info.callback = ErrorCallback;
  return info;
}

WGPUDeviceLostCallbackInfo Error::MakeDeviceLostCallbackInfo ()
{
  WGPUDeviceLostCallbackInfo info = {};
  info.mode = WGPUCallbackMode_AllowSpontaneous;
  info.callback = DeviceLostCallback;
  return info;
}
