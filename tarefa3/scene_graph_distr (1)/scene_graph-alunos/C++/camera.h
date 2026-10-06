#include <memory>
class Camera;
using CameraPtr = std::shared_ptr<Camera>;

#ifndef CAMERA_H
#define CAMERA_H

#include <utility>
#include "graphicsmath.h"

class State;
using StatePtr = std::shared_ptr<State>;

// Base interface for cameras: projection matrix, view matrix, and
// per-frame uniform upload. Defaults are identity/no-op; Camera2D/
// Camera3D override them.
class Camera {
public:
  virtual ~Camera () {}

  // Projection matrix mapping camera space to clip space, for the given
  // canvas size (width, height in pixels). Recomputed every frame.
  virtual Mat4 GetProjMatrix (std::pair<int, int> canvasSize) const { (void) canvasSize; return Mat4Identity(); }

  // View matrix mapping world space into camera (eye) space.
  virtual Mat4 GetViewMatrix () const { return Mat4Identity(); }

  // Pushes this camera's per-frame uniforms (if any) onto State; called
  // via Pipeline::Load whenever a Node's own Pipeline is loaded, so a
  // real Shader is always active by the time this runs. Pair any
  // override with a matching Unload.
  virtual void Load (StatePtr st) const { (void) st; }
  virtual void Unload (StatePtr st) const { (void) st; }
};

#endif
