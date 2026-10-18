#include "square.h"
#include "state.h"
#include "shader.h"

static WGPUBuffer MakeBuffer (WGPUDevice device, WGPUBufferUsage usage, const void* data, size_t size)
{
  WGPUBufferDescriptor desc = {};
  desc.size = size;
  desc.usage = usage | WGPUBufferUsage_CopyDst;
  WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &desc);
  WGPUQueue queue = wgpuDeviceGetQueue(device);
  wgpuQueueWriteBuffer(queue, buffer, 0, data, size);
  wgpuQueueRelease(queue);
  return buffer;
}

SquarePtr Square::Make (WGPUDevice device)
{
  return SquarePtr(new Square(device));
}

Square::Square (WGPUDevice device)
{
  static const float coord[] = {-1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f};
  // t cresce para baixo: o vertice de baixo recebe t = 1
  static const float texcoord[] = {0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f};
  static const uint32_t index[] = {0, 1, 2, 0, 2, 3};

  m_coordVbo = MakeBuffer(device, WGPUBufferUsage_Vertex, coord, sizeof(coord));
  m_texcoordVbo = MakeBuffer(device, WGPUBufferUsage_Vertex, texcoord, sizeof(texcoord));
  m_ibo = MakeBuffer(device, WGPUBufferUsage_Index, index, sizeof(index));
}

Square::~Square ()
{
  wgpuBufferRelease(m_coordVbo);
  wgpuBufferRelease(m_texcoordVbo);
  wgpuBufferRelease(m_ibo);
}

void Square::Draw (StatePtr st)
{
  uint32_t firstInstance = (uint32_t) st->GetShader()->CommitMatrix(st);
  WGPURenderPassEncoder pass = st->GetRenderPass();
  wgpuRenderPassEncoderSetVertexBuffer(pass, 0, m_coordVbo, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetVertexBuffer(pass, 1, m_texcoordVbo, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetIndexBuffer(pass, m_ibo, WGPUIndexFormat_Uint32, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderDrawIndexed(pass, 6, 1, 0, 0, firstInstance);
}
