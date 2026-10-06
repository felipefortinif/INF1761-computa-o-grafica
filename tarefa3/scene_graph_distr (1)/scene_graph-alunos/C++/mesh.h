#include <memory>
class Mesh;
using MeshPtr = std::shared_ptr<Mesh>;

#ifndef MESH_H
#define MESH_H

#include <wgpu.h>
#include <string>
#include "shape.h"

// Loads an indexed triangle mesh from a text format ("V"/"N"/"T" lines for
// coords/normals/triangles; blank lines, "#" comments, and "--" section
// separators are ignored). Vertex buffers: slot 0 = coords, slot 1 =
// normals, plus a uint32 index buffer. Fails fast (Error::Fatal, with
// filename:line) on a malformed/unrecognized record, a vertex/normal
// count mismatch, or an out-of-range triangle index.
class Mesh : public Shape {
  WGPUBuffer m_coordVbo, m_normalVbo, m_ibo;
  uint32_t m_nind;
protected:
  Mesh (WGPUDevice device, const std::string& filename);
public:
  static MeshPtr Make (WGPUDevice device, const std::string& filename);
  virtual ~Mesh ();
  virtual void Draw (StatePtr st);
};
#endif
