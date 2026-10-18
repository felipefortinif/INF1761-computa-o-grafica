#include <memory>
class Triangle;
using TrianglePtr = std::shared_ptr<Triangle>;

#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <wgpu.h>
#include "shape.h"

// Single 2D triangle with vertices (-1,0), (1,0), (0,1). One vertex
// buffer bound at both slot 0 (position) and slot 1 (texcoord).
class Triangle : public Shape {
  WGPUBuffer m_vbo;
protected:
  Triangle (WGPUDevice device);
public:
  static TrianglePtr Make (WGPUDevice device);
  virtual ~Triangle ();
  virtual void Draw (StatePtr st);
};
#endif
