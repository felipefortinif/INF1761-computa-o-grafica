#include "quad.h"
#include "grid.h"
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

QuadPtr Quad::Make (WGPUDevice device, int nx, int ny)
{
  return QuadPtr(new Quad(device, nx, ny));
}

Quad::Quad (WGPUDevice device, int nx, int ny)
{
  Grid grid(nx, ny);
  m_nind = (uint32_t) grid.IndexCount();
  m_coordVbo = MakeBuffer(device, WGPUBufferUsage_Vertex, grid.GetCoords().data(), grid.GetCoords().size() * sizeof(float));
  m_texcoordVbo = MakeBuffer(device, WGPUBufferUsage_Vertex, grid.GetTexCoords().data(), grid.GetTexCoords().size() * sizeof(float));
  m_ibo = MakeBuffer(device, WGPUBufferUsage_Index, grid.GetIndices().data(), grid.GetIndices().size() * sizeof(uint32_t));
}

Quad::~Quad ()
{
  wgpuBufferRelease(m_coordVbo);
  wgpuBufferRelease(m_texcoordVbo);
  wgpuBufferRelease(m_ibo);
}

void Quad::Draw (StatePtr st)
{
  uint32_t firstInstance = (uint32_t) st->GetShader()->CommitMatrix(st);
  WGPURenderPassEncoder pass = st->GetRenderPass();
  wgpuRenderPassEncoderSetVertexBuffer(pass, 0, m_coordVbo, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetVertexBuffer(pass, 1, m_texcoordVbo, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetIndexBuffer(pass, m_ibo, WGPUIndexFormat_Uint32, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderDrawIndexed(pass, m_nind, 1, 0, 0, firstInstance);
}
