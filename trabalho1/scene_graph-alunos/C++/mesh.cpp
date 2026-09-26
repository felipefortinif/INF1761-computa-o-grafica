#include "mesh.h"
#include "state.h"
#include "shader.h"
#include "error.h"
#include <fstream>
#include <sstream>
#include <vector>

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

MeshPtr Mesh::Make (WGPUDevice device, const std::string& filename)
{
  return MeshPtr(new Mesh(device, filename));
}

Mesh::Mesh (WGPUDevice device, const std::string& filename)
{
  std::ifstream file(filename);
  if (!file) Error::Fatal("could not open mesh file: " + filename);

  std::vector<float> coords, normals;
  std::vector<uint32_t> indices;
  std::string line;
  int lineno = 0;
  while (std::getline(file, line)) {
    lineno++;
    std::istringstream ss(line);
    std::string tag;
    if (!(ss >> tag)) continue;
    if (tag[0] == '#' || tag.rfind("--", 0) == 0) continue;

    if (tag == "V") {
      float x, y, z;
      if (!(ss >> x >> y >> z)) Error::Fatal(filename + ":" + std::to_string(lineno) + ": 'V' record needs 3 values");
      coords.push_back(x); coords.push_back(y); coords.push_back(z);
    } else if (tag == "N") {
      float x, y, z;
      if (!(ss >> x >> y >> z)) Error::Fatal(filename + ":" + std::to_string(lineno) + ": 'N' record needs 3 values");
      normals.push_back(x); normals.push_back(y); normals.push_back(z);
    } else if (tag == "T") {
      int i0, i1, i2;
      if (!(ss >> i0 >> i1 >> i2)) Error::Fatal(filename + ":" + std::to_string(lineno) + ": 'T' record needs 3 values");
      indices.push_back((uint32_t) i0); indices.push_back((uint32_t) i1); indices.push_back((uint32_t) i2);
    } else {
      Error::Fatal(filename + ":" + std::to_string(lineno) + ": unrecognized record type '" + tag + "'");
    }
  }

  size_t nverts = coords.size() / 3;
  if (normals.size() / 3 != nverts)
    Error::Fatal(filename + ": " + std::to_string(nverts) + " vertex coords but " +
                 std::to_string(normals.size() / 3) + " normals - a normal is required for every vertex");
  if (indices.empty()) Error::Fatal(filename + ": no triangles ('T' records) found");
  for (uint32_t i : indices)
    if (i >= nverts) Error::Fatal(filename + ": triangle index " + std::to_string(i) + " out of range for " +
                                   std::to_string(nverts) + " vertices");

  m_nind = (uint32_t) indices.size();
  m_coordVbo = MakeBuffer(device, WGPUBufferUsage_Vertex, coords.data(), coords.size() * sizeof(float));
  m_normalVbo = MakeBuffer(device, WGPUBufferUsage_Vertex, normals.data(), normals.size() * sizeof(float));
  m_ibo = MakeBuffer(device, WGPUBufferUsage_Index, indices.data(), indices.size() * sizeof(uint32_t));
}

Mesh::~Mesh ()
{
  wgpuBufferRelease(m_coordVbo);
  wgpuBufferRelease(m_normalVbo);
  wgpuBufferRelease(m_ibo);
}

void Mesh::Draw (StatePtr st)
{
  uint32_t firstInstance = (uint32_t) st->GetShader()->CommitMatrix(st);
  WGPURenderPassEncoder pass = st->GetRenderPass();
  wgpuRenderPassEncoderSetVertexBuffer(pass, 0, m_coordVbo, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetVertexBuffer(pass, 1, m_normalVbo, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetIndexBuffer(pass, m_ibo, WGPUIndexFormat_Uint32, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderDrawIndexed(pass, m_nind, 1, 0, 0, firstInstance);
}
