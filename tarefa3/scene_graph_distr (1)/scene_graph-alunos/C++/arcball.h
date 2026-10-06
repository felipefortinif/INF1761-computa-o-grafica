#include <memory>
class Arcball;
using ArcballPtr = std::shared_ptr<Arcball>;

#ifndef ARCBALL_H
#define ARCBALL_H

#include "graphicsmath.h"

// Virtual trackball: maps mouse drags to a rotation matrix around a
// pivot `distance` in front of the camera. Accumulates rotations across
// drags; GetMatrix returns the result to compose into the camera's view.
class Arcball {
  float m_distance;
  float m_x0, m_y0;
  Mat4 m_mat;
protected:
  Arcball (float distance);
public:
  static ArcballPtr Make (float distance);
  void InitMouseMotion (float x0, float y0);
  void AccumulateMouseMotion (float x, float y, float width, float height);
  Mat4 GetMatrix () const;
  void Translate (float dx, float dy, float dz);
};

#endif
