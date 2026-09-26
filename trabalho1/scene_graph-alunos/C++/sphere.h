#include <memory>
class Sphere;
using SpherePtr = std::shared_ptr<Sphere>;

#ifndef SPHERE_H
#define SPHERE_H

#include <wgpu.h>
#include "shape.h"

// Unit sphere centered at the origin, parameterized over a Grid of
// nstack latitude bands by nslice longitude slices. Vertex buffers: slot
// 0=coord, 1=normal (reuses coord_vbo, since position == normal on a
// unit sphere), 2=tangent, 3=texcoord.
class Sphere : public Shape {
  WGPUBuffer m_coordVbo, m_tangentVbo, m_texcoordVbo, m_ibo;
  uint32_t m_nind;
protected:
  Sphere (WGPUDevice device, int nstack, int nslice);
public:
  static SpherePtr Make (WGPUDevice device, int nstack = 64, int nslice = 64);
  virtual ~Sphere ();
  virtual void Draw (StatePtr st);
};
#endif
