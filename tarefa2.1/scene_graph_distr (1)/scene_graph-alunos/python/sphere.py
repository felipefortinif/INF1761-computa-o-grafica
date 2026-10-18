from __future__ import annotations

from typing import TYPE_CHECKING

import numpy as np
import wgpu
import math
from shape import Shape
from grid import Grid

if TYPE_CHECKING:
  from state import State

class Sphere(Shape):
  """Unit sphere centered at the origin, parameterized over a Grid of
  nstack latitude bands by nslice longitude slices. Vertex buffers: slot
  0=coord, 1=normal (reuses coord_vbo, since position == normal on a unit
  sphere), 2=tangent, 3=texcoord."""

  def __init__(self, device: wgpu.GPUDevice, nstack: int = 64, nslice: int = 64) -> None:
    grid = Grid(nstack,nslice)
    self.nind: int = grid.index_count()
    coord = np.empty(3*grid.vertex_count(), dtype = 'float32')
    tangent = np.empty(3*grid.vertex_count(), dtype = 'float32')
    # a parametrização vem de get_coords (v crescendo do polo sul ao norte);
    # o que vai para a GPU é get_texcoords, com t crescendo para baixo
    param = grid.get_coords()
    texcoord = grid.get_texcoords()
    nc = 0
    for i in range(0,2*grid.vertex_count(),2):
      theta = param[i+0]*2*math.pi
      phi = param[i+1]*math.pi
      coord[nc+0] = math.sin(theta) * math.sin(math.pi-phi)
      coord[nc+1] = math.cos(math.pi-phi)
      coord[nc+2] = math.cos(theta) * math.sin(math.pi-phi)
      tangent[nc+0] = math.cos(theta)
      tangent[nc+1] = 0
      tangent[nc+2] = -math.sin(theta)
      nc += 3

    self.coord_vbo: wgpu.GPUBuffer = device.create_buffer_with_data(data=coord, usage=wgpu.BufferUsage.VERTEX)
    # a mesma malha (coord) serve de normal, já que é uma esfera unitária centrada na origem
    self.tangent_vbo: wgpu.GPUBuffer = device.create_buffer_with_data(data=tangent, usage=wgpu.BufferUsage.VERTEX)
    self.texcoord_vbo: wgpu.GPUBuffer = device.create_buffer_with_data(data=texcoord, usage=wgpu.BufferUsage.VERTEX)
    self.ibo: wgpu.GPUBuffer = device.create_buffer_with_data(data=grid.get_indices(), usage=wgpu.BufferUsage.INDEX)

  def draw (self, st: State) -> None:
    """Reuses coord_vbo for the normal slot before the draw_indexed call."""
    first_instance = st.get_shader().commit_matrix(st)
    st.render_pass.set_vertex_buffer(0, self.coord_vbo)
    st.render_pass.set_vertex_buffer(1, self.coord_vbo)  # normal == coord (esfera unitária)
    st.render_pass.set_vertex_buffer(2, self.tangent_vbo)
    st.render_pass.set_vertex_buffer(3, self.texcoord_vbo)
    st.render_pass.set_index_buffer(self.ibo, wgpu.IndexFormat.uint32)
    st.render_pass.draw_indexed(self.nind, 1, 0, 0, first_instance)
