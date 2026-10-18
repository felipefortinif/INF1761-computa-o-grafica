from __future__ import annotations

import graphics_math as gm
from luxor.interpolator import *

class LinearInterpolator (Interpolator):
  """Straight-line (lerp) interpolation between two keyframe values p0 and p1."""

  def __init__ (self, p0: gm.Vec3, p1: gm.Vec3) -> None:
    """Store start p0 and end p1 keyframe values."""
    self.p0 = p0
    self.p1 = p1

  def interpolate (self, t: float) -> gm.Vec3:
    """Linearly interpolate between p0 and p1 at t (0..1)."""
    return (1.0-t) * self.p0 + t * self.p1
