#include "camera3d.h"
#include "node.h"
#include "error.h"
#include <cmath>

Camera3D::Camera3D (float x, float y, float z)
: m_ortho(false), m_fovy(45.0f), m_znear(0.1f), m_zfar(1000.0f),
  m_center{0, 0, 0}, m_eye{x, y, z}, m_up{0, 1, 0}
{
}

Camera3DPtr Camera3D::Make (float x, float y, float z)
{
  return Camera3DPtr(new Camera3D(x, y, z));
}

void Camera3D::SetAngle (float fovy) { m_fovy = fovy; }
float Camera3D::GetAngle () const { return m_fovy; }
void Camera3D::SetZPlanes (float znear, float zfar) { m_znear = znear; m_zfar = zfar; }
void Camera3D::SetCenter (float x, float y, float z) { m_center = {x, y, z}; }
Vec3 Camera3D::GetCenter () const { return m_center; }
void Camera3D::SetEye (float x, float y, float z) { m_eye = {x, y, z}; }
Vec3 Camera3D::GetEye () const { return m_eye; }
void Camera3D::SetUpDir (float x, float y, float z) { m_up = {x, y, z}; }
void Camera3D::SetOrtho (bool flag) { m_ortho = flag; }

ArcballPtr Camera3D::CreateArcball ()
{
  float d = Vec3Length(m_eye - m_center);
  m_arcball = Arcball::Make(d);
  return m_arcball;
}

ArcballPtr Camera3D::GetArcball () const { return m_arcball; }

void Camera3D::SetReference (NodePtr reference) { m_reference = reference; }

Mat4 Camera3D::GetProjMatrix (std::pair<int, int> canvasSize) const
{
  int w = canvasSize.first, h = canvasSize.second;
  if (w <= 0 || h <= 0) Error::Fatal("GetProjMatrix needs a canvas size with both dimensions > 0");
  float ratio = (float) w / h;
  if (!m_ortho) return Mat4Perspective(Radians(m_fovy), ratio, m_znear, m_zfar);
  float dist = Vec3Length(m_eye - m_center);
  float height = dist * std::tan(Radians(m_fovy) / 2.0f);
  float width = height / h * w;
  return Mat4Ortho(-width, width, -height, height, m_znear, m_zfar);
}

Mat4 Camera3D::GetViewMatrix () const
{
  Mat4 view = Mat4Identity();
  if (m_arcball) view = Mat4Multiply(view, m_arcball->GetMatrix());
  view = Mat4Multiply(view, Mat4LookAt(m_eye, m_center, m_up));
  if (m_reference) view = Mat4Multiply(view, Mat4Inverse(m_reference->GetModelMatrix()));
  return view;
}
