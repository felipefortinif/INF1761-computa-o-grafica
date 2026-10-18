#include <memory>
class CubicInterpolator;
using CubicInterpolatorPtr = std::shared_ptr<CubicInterpolator>;

#ifndef CUBIC_INTERPOLATOR_H
#define CUBIC_INTERPOLATOR_H

#include "interpolator.h"

class CubicInterpolator : public Interpolator {
  Vec3 m_p0;
  Vec3 m_m0;
  Vec3 m_p1;
  Vec3 m_m1;
protected:
  CubicInterpolator (const Vec3& p0, const Vec3& m0, const Vec3& p1, const Vec3& m1);
public:
  static CubicInterpolatorPtr Make (const Vec3& p0, const Vec3& m0, const Vec3& p1, const Vec3& m1);
  virtual ~CubicInterpolator ();
  virtual Vec3 Interpolate (float t);
};

#endif
