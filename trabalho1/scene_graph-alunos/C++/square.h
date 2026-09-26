#include <memory>
class Square;
using SquarePtr = std::shared_ptr<Square>;

#ifndef SQUARE_H
#define SQUARE_H

#include <wgpu.h>
#include "shape.h"

// Flat quad spanning [-1,1] in x and y (z=0), with separate coord and
// texcoord buffers bound at slots 0/1.
class Square : public Shape {
  WGPUBuffer m_coordVbo, m_texcoordVbo, m_ibo;
protected:
  Square (WGPUDevice device);
public:
  static SquarePtr Make (WGPUDevice device);
  virtual ~Square ();
  virtual void Draw (StatePtr st);
};
#endif
