from __future__ import annotations

from typing import TYPE_CHECKING

import graphics_math as gm

if TYPE_CHECKING:
  from shader import Shader

class ClipPlane:
  """Emulates GLSL's gl_ClipDistance (no WGSL equivalent) via a
  per-fragment discard in the shader. Sets "clip_plane" (ax+by+cz+d=0)
  and "clip_plane_color" (cut cross-section color) directly on `shader`
  - a "global" group field, raising if
  `shader`'s "global" struct doesn't declare them (see
  Shader.set_value)."""

  def __init__ (self, shader: Shader, a: float, b: float, c: float, d: float) -> None:
    self.shader = shader
    self.set_plane(a, b, c, d)
    self.set_color(0.5, 0.5, 0.5)

  def set_plane (self, a: float, b: float, c: float, d: float) -> None:
    """Sets the plane equation ax+by+cz+d=0, stored as gm.vec4(a,b,c,d)."""
    self.shader.set_value("clip_plane", gm.vec4(a, b, c, d))

  def set_color (self, r: float, g: float, b: float) -> None:
    self.shader.set_value("clip_plane_color", gm.vec4(r, g, b, 1.0))
