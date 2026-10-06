#include <memory>
class Interpolator;
using InterpolatorPtr = std::shared_ptr<Interpolator>;

#ifndef INTERPOLATOR_H
#define INTERPOLATOR_H

#include "graphicsmath.h"

class Interpolator {
public:
  virtual ~Interpolator () {}
  virtual Vec3 Interpolate (float t) = 0;
};

#endif
