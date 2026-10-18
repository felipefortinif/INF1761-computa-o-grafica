from __future__ import annotations

from typing import TYPE_CHECKING

from transform import *
import graphics_math as gm

if TYPE_CHECKING:
  from luxor.interpolator import Interpolator

class Movement:
  """A timed keyframe transition, lasting T seconds, applied to one or more
  Transforms in parallel. Each advance call nudges every registered
  Transform by the delta between the previous and current interpolated
  value, so it composes with existing transform state; rotations apply as
  Euler-degree deltas in X/Y/Z order."""

  def __init__ (self, T: float) -> None:
    """Set up an empty Movement lasting T seconds with its clock at 0.
    Raises if T <= 0 - t0/t1 divide by T, and a zero-length movement
    can never actually finish advancing."""
    if T <= 0:
      raise ValueError(f"Movement T must be > 0, got {T}")
    self.t: float = 0
    self.T = T
    self.trl_trf: list[Transform] = []
    self.rot_trf: list[Transform] = []
    self.trl_interp: list[Interpolator] = []
    self.rot_interp: list[Interpolator] = []

  def add_translation (self, trf: Transform, interp: Interpolator) -> None:
    """Register a translation channel: interp's output is applied to trf
    as position deltas on each advance."""
    self.trl_trf.append(trf)
    self.trl_interp.append(interp)

  def add_rotation (self, trf: Transform, interp: Interpolator) -> None:
    """Register a rotation channel: interp's output is treated as
    Euler-degree deltas applied to trf in X/Y/Z order on each advance."""
    self.rot_trf.append(trf)
    self.rot_interp.append(interp)

  def advance (self, dt: float, reverse: bool) -> float | None:
    """Step by dt seconds (backward if reverse), applying deltas to all
    registered Transforms. Returns None if the movement doesn't finish
    this step; otherwise resets the clock to 0 and returns however much
    of dt was left over beyond what this movement needed to finish (>=
    0), for the caller (Animation) to feed into the next Movement
    instead of silently dropping it."""
    t = self.t + dt
    leftover = None
    if (t >= self.T):
      leftover = t - self.T
      t = self.T
    if reverse:
      t0 = (self.T-self.t)/self.T
      t1 = (self.T-t)/self.T
    else:
      t0 = self.t / self.T
      t1 = t / self.T
    # perform translations
    for i in range(0,len(self.trl_trf)):
      v0 = self.trl_interp[i].interpolate(t0)
      v1 = self.trl_interp[i].interpolate(t1)
      self.trl_trf[i].translate(v1[0]-v0[0],v1[1]-v0[1],v1[2]-v0[2])
    # perform rotations 
    for i in range(0,len(self.rot_trf)):
      v0 = self.rot_interp[i].interpolate(t0)
      v1 = self.rot_interp[i].interpolate(t1)
      self.rot_trf[i].rotate(v1[0]-v0[0],1.0,0.0,0.0)
      self.rot_trf[i].rotate(v1[1]-v0[1],0.0,1.0,0.0)
      self.rot_trf[i].rotate(v1[2]-v0[2],0.0,0.0,1.0)
    if leftover is not None:
      self.t = 0.0   # reset internal clock for reuse
      return leftover
    else:
      self.t = t
      return None   # signalize the movement continues
