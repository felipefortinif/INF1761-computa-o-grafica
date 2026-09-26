#include <memory>
class Camera2D;
using Camera2DPtr = std::shared_ptr<Camera2D>;

#ifndef CAMERA2D_H
#define CAMERA2D_H

#include "camera.h"

// Orthographic 2D camera defined by a world-space view window
// (xmin/xmax/ymin/ymax). View matrix is always identity.
class Camera2D : public Camera {
  float m_xmin, m_xmax, m_ymin, m_ymax;
protected:
  Camera2D (float xmin, float xmax, float ymin, float ymax);
public:
  static Camera2DPtr Make (float xmin = -1, float xmax = 1, float ymin = -1, float ymax = 1);
  Mat4 GetProjMatrix (std::pair<int, int> canvasSize) const override;
  Mat4 GetViewMatrix () const override;
};

#endif
