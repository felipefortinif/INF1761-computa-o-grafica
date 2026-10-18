#include "triangle.h"
#include "state.h"
#include "shader.h"

TrianglePtr Triangle::Make (WGPUDevice device)
{
  return TrianglePtr(new Triangle(device));
}

Triangle::Triangle (WGPUDevice device)
{
  float coord[] = {-1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f};
  WGPUBufferDescriptor desc = {};
  desc.size = sizeof(coord);
  desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
  m_vbo = wgpuDeviceCreateBuffer(device, &desc);
  WGPUQueue queue = wgpuDeviceGetQueue(device);
  wgpuQueueWriteBuffer(queue, m_vbo, 0, coord, sizeof(coord));
  wgpuQueueRelease(queue);
}

Triangle::~Triangle ()
{
  wgpuBufferRelease(m_vbo);
}

void Triangle::Draw (StatePtr st)
{
  uint32_t firstInstance = (uint32_t) st->GetShader()->CommitMatrix(st);
  wgpuRenderPassEncoderSetVertexBuffer(st->GetRenderPass(), 0, m_vbo, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetVertexBuffer(st->GetRenderPass(), 1, m_vbo, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderDraw(st->GetRenderPass(), 3, 1, 0, firstInstance);
}
