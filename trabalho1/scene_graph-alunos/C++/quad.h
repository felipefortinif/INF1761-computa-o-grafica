#include <memory>
class Quad;
using QuadPtr = std::shared_ptr<Quad>;

#ifndef QUAD_H
#define QUAD_H

#include <wgpu.h>
#include "shape.h"

// Flat grid spanning [0,1]x[0,1], subdivided into nx by ny cells via
// Grid. Coord/texcoord buffers bound at slots 0/1 - two buffers, and not
// the same one twice, because t runs opposite to y (origin at the
// image's top-left corner).
class Quad : public Shape {
  WGPUBuffer m_coordVbo, m_texcoordVbo, m_ibo;
  uint32_t m_nind;
protected:
  Quad (WGPUDevice device, int nx, int ny);
public:
  static QuadPtr Make (WGPUDevice device, int nx = 1, int ny = 1);
  virtual ~Quad ();
  virtual void Draw (StatePtr st);
};
#endif
