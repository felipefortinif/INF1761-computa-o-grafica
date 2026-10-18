#include "cube.h"
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

CubePtr Cube::Make (WGPUDevice device)
{
  return CubePtr(new Cube(device));
}

Cube::Cube (WGPUDevice device)
{
  static const float coords[] = {
    // back face: counter clockwise
    -0.5f, 0.0f, -0.5f,  -0.5f, 1.0f, -0.5f,  0.5f, 1.0f, -0.5f,  0.5f, 0.0f, -0.5f,
    // front face: counter clockwise
    -0.5f, 0.0f, 0.5f,  0.5f, 0.0f, 0.5f,  0.5f, 1.0f, 0.5f,  -0.5f, 1.0f, 0.5f,
    // left face: counter clockwise
    -0.5f, 0.0f, -0.5f,  -0.5f, 0.0f, 0.5f,  -0.5f, 1.0f, 0.5f,  -0.5f, 1.0f, -0.5f,
    // right face: counter clockwise
    0.5f, 0.0f, -0.5f,  0.5f, 1.0f, -0.5f,  0.5f, 1.0f, 0.5f,  0.5f, 0.0f, 0.5f,
    // bottom face: counter clockwise
    -0.5f, 0.0f, -0.5f,  0.5f, 0.0f, -0.5f,  0.5f, 0.0f, 0.5f,  -0.5f, 0.0f, 0.5f,
    // top face: counter clockwise
    -0.5f, 1.0f, -0.5f,  -0.5f, 1.0f, 0.5f,  0.5f, 1.0f, 0.5f,  0.5f, 1.0f, -0.5f,
  };
  static const float normals[] = {
    0.0f, 0.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f, -1.0f,
    0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f,
    -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
    0.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f,
  };
  // t cresce para baixo: o par de vertices de baixo de cada face recebe t = 1
  static const float texcoords[] = {
    0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
  };
  static const float tangents[] = {
    -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f,
    0.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,  0.0f, -1.0f, 0.0f,
    -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
  };
  static const uint32_t index[] = {
    0,1,2,0,2,3, 4,5,6,4,6,7, 8,9,10,8,10,11, 12,13,14,12,14,15, 16,17,18,16,18,19, 20,21,22,20,22,23
  };

  m_coordVbo = MakeBuffer(device, WGPUBufferUsage_Vertex, coords, sizeof(coords));
  m_normalVbo = MakeBuffer(device, WGPUBufferUsage_Vertex, normals, sizeof(normals));
  m_tangentVbo = MakeBuffer(device, WGPUBufferUsage_Vertex, tangents, sizeof(tangents));
  m_texcoordVbo = MakeBuffer(device, WGPUBufferUsage_Vertex, texcoords, sizeof(texcoords));
  m_ibo = MakeBuffer(device, WGPUBufferUsage_Index, index, sizeof(index));
}

Cube::~Cube ()
{
  wgpuBufferRelease(m_coordVbo);
  wgpuBufferRelease(m_normalVbo);
  wgpuBufferRelease(m_tangentVbo);
  wgpuBufferRelease(m_texcoordVbo);
  wgpuBufferRelease(m_ibo);
}

void Cube::Draw (StatePtr st)
{
  uint32_t firstInstance = (uint32_t) st->GetShader()->CommitMatrix(st);
  WGPURenderPassEncoder pass = st->GetRenderPass();
  wgpuRenderPassEncoderSetVertexBuffer(pass, 0, m_coordVbo, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetVertexBuffer(pass, 1, m_normalVbo, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetVertexBuffer(pass, 2, m_tangentVbo, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetVertexBuffer(pass, 3, m_texcoordVbo, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetIndexBuffer(pass, m_ibo, WGPUIndexFormat_Uint32, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderDrawIndexed(pass, 36, 1, 0, 0, firstInstance);
}
