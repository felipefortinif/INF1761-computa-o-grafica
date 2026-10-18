#include "transform.h"

TransformPtr Transform::Make ()
{
  return TransformPtr(new Transform());
}

Transform::Transform ()
: m_mat(Mat4Identity())
{
}
Transform::~Transform ()
{
}
void Transform::LoadIdentity ()
{
  m_mat = Mat4Identity();
}
void Transform::MultMatrix (const Mat4& mat)
{
  m_mat = Mat4Multiply(m_mat, mat);
}
void Transform::Translate (float x, float y, float z)
{
  m_mat = Mat4Translate(m_mat, Vec3{x, y, z});
}
void Transform::Scale (float x, float y, float z)
{
  m_mat = Mat4Scale(m_mat, Vec3{x, y, z});
}
void Transform::Rotate (float angle, float x, float y, float z)
{
  m_mat = Mat4Rotate(m_mat, Radians(angle), Vec3{x, y, z});
}
const Mat4& Transform::GetMatrix () const
{
  return m_mat;
}

void Transform::Load (StatePtr st) const
{
  st->PushMatrix(m_mat);
}

void Transform::Unload (StatePtr st) const
{
  st->PopMatrix();
}
