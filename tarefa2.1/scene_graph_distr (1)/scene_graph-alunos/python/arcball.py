from __future__ import annotations

import math
from typing import Any

import graphics_math as gm

class Arcball:
  """Virtual trackball: maps mouse drags to a rotation matrix around a
  pivot `distance` in front of the camera. Accumulates rotations across
  drags; get_matrix returns the result to compose into the camera's view."""

  def __init__ (self, distance: float) -> None:
    """Store the pivot distance and initialize the accumulated rotation to
    identity."""
    self.distance = distance
    self.x0: float = 0
    self.y0: float = 0
    self.mat: gm.Mat4 = gm.mat4(1)
    self._dragging: bool = False

  def attach (self, canvas: Any) -> None:
    """Wire up pointer_down/up/move handlers on a wgpu canvas so dragging
    with the mouse drives the arcball rotation."""
    def on_down (event: Any) -> None:
      """Starts a drag: records the pointer position as the drag origin,
      flipping y to measure from the bottom."""
      self._dragging = True
      # Pointer-event coordinates are expressed in logical pixels.  Using the
      # physical framebuffer size here breaks the sphere mapping on HiDPI
      # displays (the visual center no longer maps to the sphere's center).
      w, h = canvas.get_logical_size()
      self.init_mouse_motion(event["x"], h - event["y"])
    def on_up (event: Any) -> None:
      """Ends the drag; subsequent pointer_move events are ignored until
      the next pointer_down."""
      self._dragging = False
    def on_move (event: Any) -> None:
      """While dragging, feeds the new pointer position into
      accumulate_mouse_motion to extend the rotation."""
      if self._dragging:
        w, h = canvas.get_logical_size()
        self.accumulate_mouse_motion(event["x"], h - event["y"], w, h)
    canvas.add_event_handler(on_down, "pointer_down")
    canvas.add_event_handler(on_up, "pointer_up")
    canvas.add_event_handler(on_move, "pointer_move")

  def init_mouse_motion (self, x0: float, y0: float) -> None:
    """Record the drag start position, in canvas pixel coordinates with
    y measured from the bottom."""
    self.x0 = x0
    self.y0 = y0

  def accumulate_mouse_motion (self, x: float, y: float, width: float, height: float) -> None:
    """Projects the previous and current mouse positions onto the unit
    sphere and composes the rotation between them into the accumulated
    matrix. width/height give the canvas size used for that mapping."""
    if x==self.x0 and y==self.y0:
      return
    ux, uy, uz = Map(width,height,self.x0,self.y0)
    vx, vy, vz = Map(width,height,x,y)
    self.x0 = x
    self.y0 = y
    ax = uy*vz - uz*vy
    ay = uz*vx - ux*vz
    az = ux*vy - uy*vx
    cross_len = math.sqrt(ax*ax+ay*ay+az*az)
    if cross_len < 1e-9:
      return  # u/v are (anti-)parallel - no meaningful axis, nothing to rotate
    # atan2, not asin(cross_len): u,v are unit vectors, so cross_len is
    # sin(theta) and dot is cos(theta).  The factor 2 is intentional: a
    # drag across the full sphere diameter should produce a 360-degree
    # rotation rather than merely 180 degrees.
    dot = ux*vx + uy*vy + uz*vz
    theta = 2 * math.atan2(cross_len, dot)
    # self.mat = T * R * -T
    m = gm.mat4(1)
    m = gm.translate(m,gm.vec3(0,0,-self.distance))
    m = gm.rotate(m,theta,gm.vec3(ax,ay,az))
    m = gm.translate(m,gm.vec3(0,0,self.distance))
    self.mat = m @ self.mat

  def get_matrix (self) -> gm.Mat4:
    """Return the accumulated rotation/pan matrix, meant to be composed
    into the camera's view matrix."""
    return self.mat

  def translate (self, dx: float, dy: float, dz: float) -> None:
    """Pan the arcball's accumulated matrix by an offset scaled by
    `distance`, so pan speed stays proportional to the current zoom."""
    m = gm.mat4(1)
    m = gm.translate(m,gm.vec3(dx*self.distance,dy*self.distance,dz*self.distance))
    self.mat = m @ self.mat

def Map (width: float, height: float, x: float, y: float) -> tuple[float, float, float]:
  """Project a screen point (x,y) onto the arcball's unit sphere, returning
  its (px,py,pz) coordinates. Points outside the sphere are clamped to its
  equator (pz=0)."""
  if width < height:
    r = width/2
  else:
    r = height/2
  X = (x - width/2) / r
  Y = (y - height/2) / r
  l = math.sqrt(X*X + Y*Y)
  if l <= 1:
    Z = math.sqrt(1 - l*l)
  else:
    X /= l
    Y /= l
    Z = 0
  return (X,Y,Z)
