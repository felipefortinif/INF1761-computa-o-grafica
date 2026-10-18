#include "algorithm.h"
#include "state.h"
#include "shader.h"

Algorithm::Algorithm (WGPUDevice device) : m_device(device) {}

AlgorithmPtr Algorithm::Make (WGPUDevice device)
{
  return AlgorithmPtr(new Algorithm(device));
}

void Algorithm::RenderPass (const Framebuffer& framebuffer, ScenePtr scene, CameraPtr camera, std::pair<int, int> size)
{
  WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_device, nullptr);

  WGPURenderPassDescriptor passDesc = {};
  passDesc.colorAttachmentCount = framebuffer.colorAttachments.size();
  passDesc.colorAttachments = framebuffer.colorAttachments.empty() ? nullptr : framebuffer.colorAttachments.data();
  passDesc.depthStencilAttachment = framebuffer.depthStencilAttachment ? &*framebuffer.depthStencilAttachment : nullptr;

  WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &passDesc);

  StatePtr st = State::Make(camera, m_device, pass, size);
  scene->Render(st);
  // the traversal only packed rows on the CPU; one write per shader here,
  // before submit (write_buffer is on the queue, draws are in the encoder)
  for (const ShaderPtr& shd : st->GetMatrixShaders())
    shd->FlushMatrices();

  wgpuRenderPassEncoderEnd(pass);
  wgpuRenderPassEncoderRelease(pass);

  WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(encoder, nullptr);
  wgpuCommandEncoderRelease(encoder);
  WGPUQueue queue = wgpuDeviceGetQueue(m_device);
  wgpuQueueSubmit(queue, 1, &cmd);
  wgpuQueueRelease(queue);
  wgpuCommandBufferRelease(cmd);
}
