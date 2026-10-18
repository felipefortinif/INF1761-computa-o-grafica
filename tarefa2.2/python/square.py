from __future__ import annotations

from typing import TYPE_CHECKING

import numpy as np
import wgpu
from shape import Shape

if TYPE_CHECKING:
  from state import State

class Square (Shape):
  """Flat quad spanning [-1,1] in x and y (z=0), with separate coord and
  texcoord buffers bound at slots 0/1."""

  # WebGPU não tem topologia "triangle-fan" (só triangle-list/-strip); os 4
  # vértices do quad são triangulados via índice, reproduzindo o mesmo
  # resultado do GL_TRIANGLE_FAN original.
  def __init__ (self, device: wgpu.GPUDevice) -> None:
    coord = [[-1.0,-1.0],[1.0,-1.0],[1.0,1.0],[-1.0,1.0]]
    # t cresce para baixo: o vértice de baixo recebe t = 1
    texcoord = [[0.0,1.0],[1.0,1.0],[1.0,0.0],[0.0,0.0]]
    bcoord = np.array(coord,dtype='float32')
    btexcoord = np.array(texcoord,dtype='float32')
    index = np.array([0,1,2, 0,2,3], dtype='uint32')
    self.coord_vbo: wgpu.GPUBuffer = device.create_buffer_with_data(data=bcoord, usage=wgpu.BufferUsage.VERTEX)
    self.texcoord_vbo: wgpu.GPUBuffer = device.create_buffer_with_data(data=btexcoord, usage=wgpu.BufferUsage.VERTEX)
    self.ibo: wgpu.GPUBuffer = device.create_buffer_with_data(data=index, usage=wgpu.BufferUsage.INDEX)

  def draw (self, st: State) -> None:
    first_instance = st.get_shader().commit_matrix(st)
    st.render_pass.set_vertex_buffer(0, self.coord_vbo)
    st.render_pass.set_vertex_buffer(1, self.texcoord_vbo)
    st.render_pass.set_index_buffer(self.ibo, wgpu.IndexFormat.uint32)
    st.render_pass.draw_indexed(6, 1, 0, 0, first_instance)
