from __future__ import annotations

import graphics_math as gm

class Interpolator:
  """Base class for an animation curve between two keyframe values.
  Subclasses store their keyframe data and implement interpolate."""

  def interpolate (self, t: float) -> gm.Vec3:
    """Map t in [0,1] to the interpolated vec3 value. Subclasses override."""
    raise NotImplementedError()
