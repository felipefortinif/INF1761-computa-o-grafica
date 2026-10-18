#include "camera2d.h"
#include "error.h"

Camera2D::Camera2D (float xmin, float xmax, float ymin, float ymax)
  : m_xmin(xmin), m_xmax(xmax), m_ymin(ymin), m_ymax(ymax)
{
}

Camera2DPtr Camera2D::Make (float xmin, float xmax, float ymin, float ymax)
{
  return Camera2DPtr(new Camera2D(xmin, xmax, ymin, ymax));
}

Mat4 Camera2D::GetProjMatrix (std::pair<int, int> canvasSize) const
{
  int w = canvasSize.first, h = canvasSize.second;
  if (w <= 0 || h <= 0) Error::Fatal("GetProjMatrix needs a canvas size with both dimensions > 0");

  float dx = m_xmax - m_xmin;
  float dy = m_ymax - m_ymin;
  float xmin = m_xmin, xmax = m_xmax, ymin = m_ymin, ymax = m_ymax;
  if ((float) w / h > dx / dy) {
    float xc = (m_xmin + m_xmax) / 2.0f;
    xmin = xc - dx / 2.0f * w / h;
    xmax = xc + dx / 2.0f * w / h;
  } else {
    float yc = (m_ymin + m_ymax) / 2.0f;
    ymin = yc - dy / 2.0f * h / w;
    ymax = yc + dy / 2.0f * h / w;
  }
  // WebGPU's NDC z range is [0,1], not OpenGL's [-1,1]; see Mat4Ortho.
  return Mat4Ortho(xmin, xmax, ymin, ymax, -1.0f, 1.0f);
}

Mat4 Camera2D::GetViewMatrix () const
{
  return Mat4Identity();
}
