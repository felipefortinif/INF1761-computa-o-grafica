from __future__ import annotations

from typing import Any

import graphics_math as gm

from light import Light


class DirectionalLight(Light):
    """Directional light whose direction ignores translation and is normalized."""

    def __init__ (self, x: float, y: float, z: float,
                  space: str = "world", **values: Any) -> None:
      Light.__init__(self, space, **values)
      self.set_direction(x, y, z)

    def set_direction (self, x: float, y: float, z: float) -> None:
      self.set("light_direction", gm.vec3(x,y,z))

    def map (self, matrix: gm.Mat4) -> dict[str, Any]:
      d = self.get("light_direction")
      direction = (matrix @ gm.vec4(d[0], d[1], d[2], 0.0))[:3]
      return {"light_direction": gm.normalize(direction)}
