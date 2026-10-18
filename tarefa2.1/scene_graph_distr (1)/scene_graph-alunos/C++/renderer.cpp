#include "renderer.h"
#include "state.h"
#include "shader.h"

Renderer::Renderer (WGPUDevice device, bool depthTest, std::array<double, 4> clearValue, WGPUTextureFormat depthFormat)
  : m_device(device), m_depthTest(depthTest), m_clearValue(clearValue), m_depthFormat(depthFormat),
    m_depthTexture(nullptr), m_depthView(nullptr)
{
}

RendererPtr Renderer::Make (WGPUDevice device, bool depthTest, std::array<double, 4> clearValue,
                             WGPUTextureFormat depthFormat)
{
  return RendererPtr(new Renderer(device, depthTest, clearValue, depthFormat));
}

Renderer::~Renderer ()
{
  if (m_depthView) wgpuTextureViewRelease(m_depthView);
  if (m_depthTexture) wgpuTextureRelease(m_depthTexture);
}

void Renderer::EnsureDepthTexture (uint32_t width, uint32_t height)
{
  if (!m_depthTest) return;
  if (m_depthTexture && wgpuTextureGetWidth(m_depthTexture) == width && wgpuTextureGetHeight(m_depthTexture) == height)
    return;
  if (m_depthTexture) {
    wgpuTextureViewRelease(m_depthView);
    wgpuTextureDestroy(m_depthTexture);
    wgpuTextureRelease(m_depthTexture);
  }
  WGPUTextureDescriptor desc = {};
  desc.usage = WGPUTextureUsage_RenderAttachment;
  desc.dimension = WGPUTextureDimension_2D;
  desc.size = {width, height, 1};
  desc.format = m_depthFormat;
  desc.mipLevelCount = 1;
  desc.sampleCount = 1;
  m_depthTexture = wgpuDeviceCreateTexture(m_device, &desc);
  m_depthView = wgpuTextureCreateView(m_depthTexture, nullptr);
}

void Renderer::Render (WGPUTexture targetTexture, ScenePtr scene, CameraPtr camera)
{
  uint32_t width = wgpuTextureGetWidth(targetTexture);
  uint32_t height = wgpuTextureGetHeight(targetTexture);
  EnsureDepthTexture(width, height);

  WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_device, nullptr);

  WGPUTextureView targetView = wgpuTextureCreateView(targetTexture, nullptr);
  WGPURenderPassColorAttachment colorAtt = {};
  colorAtt.view = targetView;
  colorAtt.loadOp = WGPULoadOp_Clear;
  colorAtt.storeOp = WGPUStoreOp_Store;
  colorAtt.clearValue = {m_clearValue[0], m_clearValue[1], m_clearValue[2], m_clearValue[3]};
  colorAtt.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

  WGPURenderPassDepthStencilAttachment depthAtt = {};
  if (m_depthTest) {
    depthAtt.view = m_depthView;
    depthAtt.depthClearValue = 1.0f;
    depthAtt.depthLoadOp = WGPULoadOp_Clear;
    depthAtt.depthStoreOp = WGPUStoreOp_Store;
  }

  WGPURenderPassDescriptor passDesc = {};
  passDesc.colorAttachmentCount = 1;
  passDesc.colorAttachments = &colorAtt;
  passDesc.depthStencilAttachment = m_depthTest ? &depthAtt : nullptr;

  WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &passDesc);

  StatePtr st = State::Make(camera, m_device, pass, {(int) width, (int) height});
  scene->Render(st);
  // the traversal only packed rows on the CPU; one write per shader here,
  // before submit (write_buffer is on the queue, draws are in the encoder)
  for (const ShaderPtr& shd : st->GetMatrixShaders())
    shd->FlushMatrices();

  wgpuRenderPassEncoderEnd(pass);
  wgpuRenderPassEncoderRelease(pass);
  wgpuTextureViewRelease(targetView);

  WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(encoder, nullptr);
  wgpuCommandEncoderRelease(encoder);
  WGPUQueue queue = wgpuDeviceGetQueue(m_device);
  wgpuQueueSubmit(queue, 1, &cmd);
  wgpuQueueRelease(queue);
  wgpuCommandBufferRelease(cmd);
}
