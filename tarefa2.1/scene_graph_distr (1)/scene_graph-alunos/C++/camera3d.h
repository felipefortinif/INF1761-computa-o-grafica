#include <memory>
class Camera3D;
using Camera3DPtr = std::shared_ptr<Camera3D>;

#ifndef CAMERA3D_H
#define CAMERA3D_H

#include "camera.h"
#include "arcball.h"

class Node;
using NodePtr = std::shared_ptr<Node>;

// Perspective or orthographic 3D camera positioned by an eye/center/up
// triad. Optional Arcball adds interactive orbit rotation; optional
// reference Node's inverse model matrix composes into the view so the
// camera follows it.
class Camera3D : public Camera {
  bool m_ortho;
  float m_fovy;
  float m_znear, m_zfar;
  Vec3 m_center;
  Vec3 m_eye;
  Vec3 m_up;
  ArcballPtr m_arcball;
  NodePtr m_reference;

protected:
  Camera3D (float x, float y, float z);

public:
  static Camera3DPtr Make (float x, float y, float z);

  void SetAngle (float fovy);
  float GetAngle () const;
  void SetZPlanes (float znear, float zfar);
  void SetCenter (float x, float y, float z);
  Vec3 GetCenter () const;
  void SetEye (float x, float y, float z);
  Vec3 GetEye () const;
  void SetUpDir (float x, float y, float z);
  void SetOrtho (bool flag);
  ArcballPtr CreateArcball ();
  ArcballPtr GetArcball () const;
  void SetReference (NodePtr reference);

  Mat4 GetProjMatrix (std::pair<int, int> canvasSize) const override;
  Mat4 GetViewMatrix () const override;
};

#endif
