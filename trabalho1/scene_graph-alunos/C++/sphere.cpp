#include "sphere.h"
#include "grid.h"
#include "state.h"
#include "shader.h"
#include <cmath>

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

SpherePtr Sphere::Make (WGPUDevice device, int nstack, int nslice)
{
  return SpherePtr(new Sphere(device, nstack, nslice));
}

Sphere::Sphere (WGPUDevice device, int nstack, int nslice)
{
  Grid grid(nstack, nslice);
  m_nind = (uint32_t) grid.IndexCount();
  // a parametrizacao vem de GetCoords (v crescendo do polo sul ao norte);
  // o que vai para a GPU e GetTexCoords, com t crescendo para baixo
  const std::vector<float>& param = grid.GetCoords();
  const std::vector<float>& texcoord = grid.GetTexCoords();

  std::vector<float> coord(3 * grid.VertexCount());
  std::vector<float> tangent(3 * grid.VertexCount());
  int nc = 0;
  for (size_t i = 0; i < param.size(); i += 2) {
    float theta = param[i + 0] * 2.0f * (float) M_PI;
    float phi = param[i + 1] * (float) M_PI;
    coord[nc + 0] = std::sin(theta) * std::sin((float) M_PI - phi);
    coord[nc + 1] = std::cos((float) M_PI - phi);
    coord[nc + 2] = std::cos(theta) * std::sin((float) M_PI - phi);
    tangent[nc + 0] = std::cos(theta);
    tangent[nc + 1] = 0.0f;
    tangent[nc + 2] = -std::sin(theta);
    nc += 3;
  }

  m_coordVbo = MakeBuffer(device, WGPUBufferUsage_Vertex, coord.data(), coord.size() * sizeof(float));
  m_tangentVbo = MakeBuffer(device, WGPUBufferUsage_Vertex, tangent.data(), tangent.size() * sizeof(float));
  m_texcoordVbo = MakeBuffer(device, WGPUBufferUsage_Vertex, texcoord.data(), texcoord.size() * sizeof(float));
  m_ibo = MakeBuffer(device, WGPUBufferUsage_Index, grid.GetIndices().data(), grid.GetIndices().size() * sizeof(uint32_t));
}

Sphere::~Sphere ()
{
  wgpuBufferRelease(m_coordVbo);
  wgpuBufferRelease(m_tangentVbo);
  wgpuBufferRelease(m_texcoordVbo);
  wgpuBufferRelease(m_ibo);
}

void Sphere::Draw (StatePtr st)
{
  uint32_t firstInstance = (uint32_t) st->GetShader()->CommitMatrix(st);
  WGPURenderPassEncoder pass = st->GetRenderPass();
  wgpuRenderPassEncoderSetVertexBuffer(pass, 0, m_coordVbo, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetVertexBuffer(pass, 1, m_coordVbo, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetVertexBuffer(pass, 2, m_tangentVbo, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetVertexBuffer(pass, 3, m_texcoordVbo, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetIndexBuffer(pass, m_ibo, WGPUIndexFormat_Uint32, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderDrawIndexed(pass, m_nind, 1, 0, 0, firstInstance);
}
