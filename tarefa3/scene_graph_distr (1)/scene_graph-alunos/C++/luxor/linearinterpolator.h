#include <memory>
class LinearInterpolator;
using LinearInterpolatorPtr = std::shared_ptr<LinearInterpolator>;

#ifndef LINEAR_INTERPOLATOR_H
#define LINEAR_INTERPOLATOR_H

#include "interpolator.h"

class LinearInterpolator : public Interpolator {
  Vec3 m_p0;
  Vec3 m_p1;
protected:
  LinearInterpolator (const Vec3& p0, const Vec3& p1);
public:
  static LinearInterpolatorPtr Make (const Vec3& p0, const Vec3& p1);
  virtual ~LinearInterpolator ();
  virtual Vec3 Interpolate (float t);
};

#endif
