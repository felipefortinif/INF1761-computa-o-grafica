#include <memory>
class Movement;
using MovementPtr = std::shared_ptr<Movement>;

#ifndef MOVEMENT_H
#define MOVEMENT_H

#include "interpolator.h"
#include "transform.h"
#include <vector>
#include <optional>

// A single timed keyframe segment of duration T seconds, applied to a
// set of Transforms in parallel. Advance applies the *delta* between
// the previous and new interpolated value (via Transform::Translate/
// Rotate), so it composes with whatever pose the Transform is already
// in rather than overriding it.
class Movement {
  float m_t;     // current time
  float m_T;     // duration (period)
  std::vector<TransformPtr> m_trl_trf;
  std::vector<TransformPtr> m_rot_trf;
  std::vector<InterpolatorPtr> m_trl_interp;
  std::vector<InterpolatorPtr> m_rot_interp;
protected:
  Movement (float T);
public:
  static MovementPtr Make (float T);
  virtual ~Movement ();
  void AddTranslation (TransformPtr trf, InterpolatorPtr interp);
  void AddRotation (TransformPtr trf, InterpolatorPtr interp);

  // Returns std::nullopt if the movement doesn't finish this call;
  // otherwise resets its internal clock and returns the leftover dt
  // (>= 0) beyond what was needed to finish, so a big dt (e.g. after a
  // stall) isn't silently dropped - see Animation::Advance.
  std::optional<float> Advance (float dt, bool reverse=false);
};

#endif
