#include <memory>
class Renderer;
using RendererPtr = std::shared_ptr<Renderer>;

#ifndef RENDERER_H
#define RENDERER_H

#include <wgpu.h>
#include <array>
#include "scene.h"
#include "camera.h"

// Owns the per-frame wgpu mechanics for a single render pass: the
// encoder/render-pass/submit dance and the depth-texture lifecycle. The
// depth texture (when depthTest is set) is created lazily and resized to
// match the target texture, so no separate Resize() call is needed.
class Renderer {
  WGPUDevice m_device;
  bool m_depthTest;
  std::array<double, 4> m_clearValue;
  WGPUTextureFormat m_depthFormat;
  WGPUTexture m_depthTexture;
  WGPUTextureView m_depthView;

  void EnsureDepthTexture (uint32_t width, uint32_t height);

protected:
  Renderer (WGPUDevice device, bool depthTest, std::array<double, 4> clearValue, WGPUTextureFormat depthFormat);

public:
  static RendererPtr Make (WGPUDevice device, bool depthTest = false,
                            std::array<double, 4> clearValue = {0, 0, 0, 1},
                            WGPUTextureFormat depthFormat = WGPUTextureFormat_Depth24Plus);
  ~Renderer ();

  void Render (WGPUTexture targetTexture, ScenePtr scene, CameraPtr camera);
};

#endif
