from __future__ import annotations

import graphics_math as gm
from luxor.interpolator import *

class CubicInterpolator (Interpolator):
  """Cubic Hermite spline interpolation between two keyframes. p0/p1 are
  endpoint values at t=0/1; m0/m1 are their tangent vectors."""

  def __init__ (self, p0: gm.Vec3, m0: gm.Vec3, p1: gm.Vec3, m1: gm.Vec3) -> None:
    """Store endpoints p0/p1 and tangents m0/m1."""
    self.p0 = p0
    self.m0 = m0
    self.p1 = p1
    self.m1 = m1

  def interpolate (self, t: float) -> gm.Vec3:
    """Evaluate the Hermite basis functions at t (0..1)."""
    t2 = t*t
    t3 = t*t2
    return ((2*t3-3*t2+1) * self.p0 +
            (t3-2*t2+t) * self.m0 + 
            (-2*t3+3*t2) * self.p1 +
            (t3-t2) * self.m1
           )
