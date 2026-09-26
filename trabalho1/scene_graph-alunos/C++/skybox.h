#include <memory>
class SkyBox;
using SkyBoxPtr = std::shared_ptr<SkyBox>;
class SkyBoxTransform;
using SkyBoxTransformPtr = std::shared_ptr<SkyBoxTransform>;

#ifndef SKYBOX_H
#define SKYBOX_H

#include <wgpu.h>
#include "shape.h"
#include "transform.h"

// Unit cube (positions only, slot 0) whose vertex positions double as
// the cubemap sample direction. Pair it with a SkyBoxTransform on its
// Node - re-centering on the eye is that Transform's job, not Draw()'s.
//
// Note: the original drawing disabled depth writes during the draw (the
// skybox never writes to the depth buffer, always staying behind
// everything). In WebGPU depth_write_enabled is fixed per pipeline -
// configure the skybox's Pipeline with depthWriteEnabled=false if this
// behavior is needed.
class SkyBox : public Shape {
  WGPUBuffer m_vbo;
protected:
  SkyBox (WGPUDevice device);
public:
  static SkyBoxPtr Make (WGPUDevice device);
  virtual ~SkyBox ();
  virtual void Draw (StatePtr st);
};

// Replaces the accumulated model matrix with a translation to the
// camera's eye, so the skybox stays centered on the viewer and therefore
// looks infinitely far away.
//
// Unlike a plain Transform, which composes onto its ancestors' matrix,
// this one overrides it: it pushes with State::Push, not PushMatrix. It
// is a Transform (loaded by Node::Render before LoadMatrices) precisely
// so that projection/vertex/normal are derived from the overridden
// matrix, with no re-entrant LoadMatrices from inside a draw.
class SkyBoxTransform : public Transform {
protected:
  SkyBoxTransform () = default;
public:
  static SkyBoxTransformPtr Make ();
  virtual void Load (StatePtr st) const;
};

#endif
