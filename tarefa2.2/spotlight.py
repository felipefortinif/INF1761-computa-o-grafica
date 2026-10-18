from __future__ import annotations

import math
from typing import Any

import graphics_math as gm

from light import Light


class SpotLight(Light):
    """Spot light: a point light whose emission is limited to a cone.

    Defined by its position (s_pos), central direction (s_dir), cutoff
    angle theta (in degrees) and concentration exponent (s_exp), besides
    the usual light colors. The shader receives:
      light_position: vec4  - s_pos, mapped like a point (w = 1)
      spot_direction: vec3  - s_dir, mapped like a vector and normalized
      spot_cutoff:    f32   - cos(theta), so the cone test is one compare
      spot_exponent:  f32   - s_exp
    and computes, per fragment, c = -l . s_dir and
    k_spot = max(c,0)^s_exp if c >= cos(theta), 0 otherwise."""

    def __init__ (self, x: float, y: float, z: float,
                  dx: float, dy: float, dz: float,
                  cutoff: float = 30.0, exponent: float = 0.0,
                  space: str = "world", **values: Any) -> None:
      Light.__init__(self, space, **values)
      self.set_position(x, y, z)
      self.set_direction(dx, dy, dz)
      self.set_cutoff(cutoff)
      self.set_exponent(exponent)

    def set_position (self, x: float, y: float, z: float) -> None:
      self.set("light_position", gm.vec4(x,y,z,1.0))

    def set_direction (self, x: float, y: float, z: float) -> None:
      self.set("spot_direction", gm.vec3(x,y,z))

    def set_target (self, x: float, y: float, z: float) -> None:
      """Points the spot at (x,y,z), given in the light's own space."""
      p = self.get("light_position")
      self.set_direction(x-p[0], y-p[1], z-p[2])

    def set_cutoff (self, angle: float) -> None:
      """Sets the cutoff angle theta, in degrees, measured from the
      central direction to the border of the cone."""
      if not 0.0 < angle <= 180.0:
        raise ValueError("Spot cutoff angle must be in (0, 180] degrees")
      self.cutoff = angle
      self.set("spot_cutoff", math.cos(math.radians(angle)))

    def get_cutoff (self) -> float:
      return self.cutoff

    def set_exponent (self, exponent: float) -> None:
      if exponent < 0.0:
        raise ValueError("Spot exponent must be non-negative")
      self.set("spot_exponent", float(exponent))

    def get_exponent (self) -> float:
      return self.get("spot_exponent")

    def map (self, matrix: gm.Mat4) -> dict[str, Any]:
      position = self.get("light_position")
      d = self.get("spot_direction")
      direction = (matrix @ gm.vec4(d[0], d[1], d[2], 0.0))[:3]
      return {
        "light_position": matrix @ position,
        "spot_direction": gm.normalize(direction),
      }
