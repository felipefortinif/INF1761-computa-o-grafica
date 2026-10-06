#include <memory>
class WgpuContext;
using WgpuContextPtr = std::shared_ptr<WgpuContext>;

#ifndef WGPUCONTEXT_H
#define WGPUCONTEXT_H

#include <wgpu.h>
#include <GLFW/glfw3.h>
#include <functional>

class WgpuContext : public std::enable_shared_from_this<WgpuContext>
{
  WGPUInstance m_instance;
  WGPUSurface m_surface;
  WGPUAdapter m_adapter;
  WGPUDevice m_device;
  WGPUQueue m_queue;
  WGPUTextureFormat m_surfaceFormat;
protected:
  WgpuContext ();
public:
  static WgpuContextPtr Make (GLFWwindow* window, int width, int height);
  ~WgpuContext ();
  WGPUDevice GetDevice () const;
  WGPUQueue GetQueue () const;
  WGPUSurface GetSurface () const;
  WGPUTextureFormat GetSurfaceFormat () const;
  void Configure (int width, int height);
  WGPUSurfaceTexture GetCurrentTexture () const;
  void Present () const;
  void WaitFor (const std::function<bool()>& done) const;
};

#endif
