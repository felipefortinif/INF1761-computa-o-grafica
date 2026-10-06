#include <memory>
class Cube;
using CubePtr = std::shared_ptr<Cube>;

#ifndef CUBE_H
#define CUBE_H

#include <wgpu.h>
#include "shape.h"

// Axis-aligned unit cube spanning x,z in [-0.5,0.5], y in [0,1]. Vertex
// buffers: slot 0=coord, 1=normal, 2=tangent, 3=texcoord, plus a uint32
// index buffer (36 indices).
class Cube : public Shape {
  WGPUBuffer m_coordVbo, m_normalVbo, m_tangentVbo, m_texcoordVbo, m_ibo;
protected:
  Cube (WGPUDevice device);
public:
  static CubePtr Make (WGPUDevice device);
  virtual ~Cube ();
  virtual void Draw (StatePtr st);
};
#endif
