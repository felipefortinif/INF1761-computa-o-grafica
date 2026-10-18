#ifndef ERROR_H
#define ERROR_H

#include <string>
#include <wgpu.h>

class Error {
public:
  static void Fatal (const std::string& msg);
  static WGPUUncapturedErrorCallbackInfo MakeErrorCallbackInfo ();
  static WGPUDeviceLostCallbackInfo MakeDeviceLostCallbackInfo ();
};
#endif
