from __future__ import annotations

import math
from typing import TYPE_CHECKING

import graphics_math as gm
from camera import *

if TYPE_CHECKING:
  from arcball import Arcball
  from node import Node

class Camera3D (Camera):
  """Perspective or orthographic 3D camera positioned by an eye/center/up
  triad. Optional Arcball adds interactive orbit rotation; optional
  reference Node's inverse model matrix composes into the view so the
  camera follows it."""

  def __init__(self, x: float, y: float, z: float) -> None:
    """Place the eye at (x,y,z), looking at the world origin with +Y up,
    perspective by default (fovy=45, znear=0.1, zfar=1000); arcball and
    reference node start unset."""
    self.ortho: bool = False
    self.fovy: float = 45
    self.znear: float = 0.1
    self.zfar: float = 1000
    self.center: gm.Vec3 = gm.vec3(0,0,0)
    self.eye: gm.Vec3 = gm.vec3(x,y,z)
    self.up: gm.Vec3 = gm.vec3(0,1,0)
    self.arcball: Arcball | None = None
    self.reference: Node | None = None

  def set_angle (self, fovy: float) -> None:
    """Set the vertical field of view, in degrees."""
    self.fovy = fovy

  def get_angle (self) -> float:
    """Return the vertical field of view, in degrees."""
    return self.fovy

  def set_z_planes (self, znear: float, zfar: float) -> None:
    """Set the near/far clip plane distances used by get_proj_matrix, for
    both the perspective and orthographic cases."""
    self.znear = znear
    self.zfar = zfar

  def set_center (self, x: float, y: float, z: float) -> None:
    """Set the look-at target point, in world space, that the camera aims
    at and (if attached) the arcball pivots around."""
    self.center = gm.vec3(x,y,z)

  def get_center (self) -> gm.Vec3:
    """Return the current look-at target point, in world space."""
    return self.center

  def set_eye (self, x: float, y: float, z: float) -> None:
    """Set the camera's eye (viewpoint) position, in world space."""
    self.eye = gm.vec3(x,y,z)

  def get_eye (self) -> gm.Vec3:
    """Return the camera's current eye (viewpoint) position, in world space."""
    return self.eye

  def set_up_dir (self, x: float, y: float, z: float) -> None:
    """Set the up direction for the look-at view matrix; needn't be
    normalized or orthogonal to the view direction."""
    self.up = gm.vec3(x,y,z)

  def set_ortho (self, flag: bool) -> None:
    """Toggle get_proj_matrix between perspective (False, default) and
    orthographic (True) projection; the orthographic frustum is still
    derived from fovy and the eye-center distance."""
    self.ortho = flag

  def create_arcball (self) -> Arcball:
    """Create and attach an Arcball sized to the camera's current
    eye-to-center distance, so its rotation pivots around the center."""
    from arcball import Arcball
    d = gm.length(self.eye-self.center)
    self.arcball = Arcball(d)
    return self.arcball

  def get_arcball (self) -> Arcball | None:
    """Return the attached Arcball, or None if create_arcball was never
    called."""
    return self.arcball

  def set_reference (self, ref: Node) -> None:
    """Attach the camera to a scene node; the node's model matrix is
    inverted and composed into the view matrix so the camera follows it."""
    self.reference = ref

  def get_proj_matrix (self, canvas_size: tuple[int, int]) -> gm.Mat4:
    """Perspective or orthographic projection matrix (per set_ortho) for the
    given canvas size. Recomputed every frame - depends on canvas_size and,
    when orthographic, the eye-center distance."""
    # WebGPU's NDC z range is [0,1] (near->0, far->1),
    # not OpenGL's [-1,1] depth range - using
    # the GL convention here would get near-plane-clipped for any znear/zfar
    # pair where the true depth sits in the half of the range that maps to
    # negative NDC z (invisible for the usual znear<<zfar case, but not for
    # e.g. main_shadow.py's znear=10/zfar=300 shadow-camera ortho projection).
    #
    # When orthographic, the half-height/half-width of the frustum are
    # derived from fovy and the eye-center distance, so the visible extent
    # at the center matches what the perspective projection would show.
    w, h = canvas_size
    if w <= 0 or h <= 0:
      raise ValueError(f"get_proj_matrix needs a canvas_size with both dimensions > 0, got {canvas_size}")
    ratio = w/h
    if not self.ortho:
      return gm.perspective(gm.radians(self.fovy),ratio,self.znear,self.zfar)
    else:
      dist = gm.distance(self.eye,self.center)
      height = dist * math.tan(gm.radians(self.fovy)/2)
      width = height / h * w
      return gm.ortho(-width,width,-height,height,self.znear,self.zfar)

  def get_view_matrix (self) -> gm.Mat4:
    """View matrix transforming world space into camera (eye) space:
    composes the arcball rotation (if attached), the eye/center/up look-at,
    then the reference node's inverse transform (if attached), in that
    order. Recomputed every frame."""
    # Composition order: arcball rotation (about center), then the
    # eye/center/up look-at, then the reference node's inverse transform.
    view = gm.mat4(1.0)
    if self.arcball:
      view = view @ self.arcball.get_matrix()
    view = view @ gm.look_at(self.eye,self.center,self.up)
    if (self.reference):
        view = view @ gm.inverse(self.reference.get_model_matrix())
    return view
