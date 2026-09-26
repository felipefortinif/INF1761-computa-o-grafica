#include <memory>
class Transform;
using TransformPtr = std::shared_ptr<Transform>;

#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "graphicsmath.h"
#include "state.h"

// Accumulates an affine transform as a single 4x4 matrix;
// Translate/Scale/Rotate/MultMatrix each right-multiply the current
// matrix. On Load, composes this matrix onto the State's "matrix" stack;
// Unload pops it back off.
class Transform {
  Mat4 m_mat;
protected:
  Transform ();
public:
  static TransformPtr Make ();
  virtual ~Transform ();
  void LoadIdentity ();
  void MultMatrix (const Mat4& mat);
  void Translate (float x, float y, float z);
  void Scale (float x, float y, float z);
  void Rotate (float angle, float x, float y, float z);
  const Mat4& GetMatrix () const;
  // virtual so a subclass can override rather than compose - see
  // SkyBoxTransform in skybox.h
  virtual void Load (StatePtr st) const;
  virtual void Unload (StatePtr st) const;
};

#endif
