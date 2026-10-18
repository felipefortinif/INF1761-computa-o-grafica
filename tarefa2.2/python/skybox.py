from __future__ import annotations

from typing import TYPE_CHECKING

import numpy as np
import graphics_math as gm
import wgpu
from shape import *
from transform import Transform

if TYPE_CHECKING:
  from state import State

class SkyBox (Shape):
  """Unit cube (positions only, slot 0) whose vertex positions double as
  the cubemap sample direction. draw() re-centers it at the camera's eye
  each frame so it always appears infinitely far away."""

  # Note: the original drawing disabled glDepthMask during the draw (the
  # skybox never writes to the depth buffer, always staying behind
  # everything). In WebGPU depth_write_enabled is fixed per pipeline -
  # configure the skybox's Pipeline with depth_write_enabled=False if this
  # behavior is needed.
  def __init__ (self, device: wgpu.GPUDevice) -> None:
    """Uploads the unit cube's 36 vertex positions (6 faces x 2 triangles)
    to a single vertex buffer."""
    coords = np.array([
      -1.0,  1.0, -1.0,
      -1.0, -1.0, -1.0,
      1.0, -1.0, -1.0,
      1.0, -1.0, -1.0,
      1.0,  1.0, -1.0,
      -1.0,  1.0, -1.0,

      -1.0, -1.0,  1.0,
      -1.0, -1.0, -1.0,
      -1.0,  1.0, -1.0,
      -1.0,  1.0, -1.0,
      -1.0,  1.0,  1.0,
      -1.0, -1.0,  1.0,

      1.0, -1.0, -1.0,
      1.0, -1.0,  1.0,
      1.0,  1.0,  1.0,
      1.0,  1.0,  1.0,
      1.0,  1.0, -1.0,
      1.0, -1.0, -1.0,

      -1.0, -1.0,  1.0,
      -1.0,  1.0,  1.0,
      1.0,  1.0,  1.0,
      1.0,  1.0,  1.0,
      1.0, -1.0,  1.0,
      -1.0, -1.0,  1.0,

      -1.0,  1.0, -1.0,
      1.0,  1.0, -1.0,
      1.0,  1.0,  1.0,
      1.0,  1.0,  1.0,
      -1.0,  1.0,  1.0,
      -1.0,  1.0, -1.0,

      -1.0, -1.0, -1.0,
      -1.0, -1.0,  1.0,
      1.0, -1.0, -1.0,
      1.0, -1.0, -1.0,
      -1.0, -1.0,  1.0,
      1.0, -1.0,  1.0
    ], dtype='float32')

    self.vbo: wgpu.GPUBuffer = device.create_buffer_with_data(data=coords, usage=wgpu.BufferUsage.VERTEX)

  def draw (self, st: State) -> None:
    """Draws the cube's 36 vertices, like any other Shape. Re-centering on
    the camera's eye is SkyBoxTransform's job, not this one's - pair this
    Shape with one on its Node. Relies on the current Pipeline having
    depth_write_enabled=False so it never occludes other geometry."""
    first_instance = st.get_shader().commit_matrix(st)
    st.render_pass.set_vertex_buffer(0, self.vbo)
    st.render_pass.draw(36, 1, 0, first_instance)


class SkyBoxTransform (Transform):
  """Replaces the accumulated model matrix with a translation to the
  camera's eye, so the skybox stays centered on the viewer and therefore
  looks infinitely far away.

  Unlike a plain Transform, which composes onto its ancestors' matrix,
  this one overrides it: it uses State.push_matrix_absolute. It
  is a Transform (loaded by Node.render before load_matrices) precisely
  so that projection/vertex/normal are derived from the overridden
  matrix, with no re-entrant load_matrices from inside a draw."""

  def load (self, st: State) -> None:
    """Pushes translate(eye) onto the matrix stack, discarding whatever
    the ancestors accumulated. Pair with unload (inherited: pops it)."""
    origin = gm.vec4(0,0,0,1)
    peye = (st.get_inverse_view_matrix() @ origin)[:3]
    st.push_matrix_absolute(gm.translate(gm.mat4(1), peye))
