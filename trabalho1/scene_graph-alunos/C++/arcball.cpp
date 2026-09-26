#include "arcball.h"
#include <cmath>

// Project a screen point (x,y) onto the arcball's unit sphere, returning
// its (x,y,z) coordinates. Points outside the sphere are clamped to its
// equator (z=0).
static Vec3 Map (float width, float height, float x, float y)
{
  float r = width < height ? width / 2.0f : height / 2.0f;
  float X = (x - width / 2.0f) / r;
  float Y = (y - height / 2.0f) / r;
  float l = std::sqrt(X * X + Y * Y);
  float Z;
  if (l <= 1.0f) {
    Z = std::sqrt(1.0f - l * l);
  } else {
    X /= l;
    Y /= l;
    Z = 0.0f;
  }
  return {X, Y, Z};
}

Arcball::Arcball (float distance)
: m_distance(distance), m_x0(0), m_y0(0), m_mat(Mat4Identity())
{
}

ArcballPtr Arcball::Make (float distance)
{
  return ArcballPtr(new Arcball(distance));
}

void Arcball::InitMouseMotion (float x0, float y0)
{
  m_x0 = x0;
  m_y0 = y0;
}

void Arcball::AccumulateMouseMotion (float x, float y, float width, float height)
{
  if (x == m_x0 && y == m_y0) return;
  Vec3 u = Map(width, height, m_x0, m_y0);
  Vec3 v = Map(width, height, x, y);
  m_x0 = x;
  m_y0 = y;
  Vec3 axis = Vec3Cross(u, v);
  float crossLen = Vec3Length(axis);
  if (crossLen < 1e-9f) return; // u/v are (anti-)parallel - no meaningful axis

  // atan2, not asin(crossLen): u,v are unit vectors, so crossLen is
  // sin(theta) and dot is cos(theta). The factor 2 is intentional: a drag
  // across the full sphere diameter should produce a 360-degree rotation.
  float dot = Vec3Dot(u, v);
  float theta = 2.0f * std::atan2(crossLen, dot);

  Mat4 m = Mat4Identity();
  m = Mat4Translate(m, Vec3{0, 0, -m_distance});
  m = Mat4Rotate(m, theta, axis);
  m = Mat4Translate(m, Vec3{0, 0, m_distance});
  m_mat = Mat4Multiply(m, m_mat);
}

Mat4 Arcball::GetMatrix () const { return m_mat; }

void Arcball::Translate (float dx, float dy, float dz)
{
  Mat4 m = Mat4Translate(Mat4Identity(), Vec3{dx * m_distance, dy * m_distance, dz * m_distance});
  m_mat = Mat4Multiply(m, m_mat);
}
