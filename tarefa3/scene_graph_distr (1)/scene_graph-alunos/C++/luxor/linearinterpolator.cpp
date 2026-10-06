#include "linearinterpolator.h"
LinearInterpolator::LinearInterpolator (const Vec3& p0, const Vec3& p1)
: m_p0(p0), m_p1(p1)
{
}
LinearInterpolatorPtr LinearInterpolator::Make (const Vec3& p0, const Vec3& p1)
{
  return LinearInterpolatorPtr(new LinearInterpolator(p0,p1));
}

LinearInterpolator::~LinearInterpolator ()
{
}

Vec3 LinearInterpolator::Interpolate (float t)
{
  return (1.0f-t) * m_p0 + t * m_p1;
}
