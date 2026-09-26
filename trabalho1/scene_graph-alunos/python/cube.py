from __future__ import annotations

from typing import TYPE_CHECKING

import numpy as np
import wgpu
from shape import Shape

if TYPE_CHECKING:
  from state import State

class Cube (Shape):
  """Axis-aligned unit cube spanning x,z in [-0.5,0.5], y in [0,1]. Vertex
  buffers: slot 0=coord, 1=normal, 2=tangent, 3=texcoord, plus a uint32
  index buffer (36 indices)."""

  def __init__ (self, device: wgpu.GPUDevice) -> None:
    coords = np.array([
      # back face: counter clockwise
      -0.5, 0.0,-0.5,
      -0.5, 1.0,-0.5,
      0.5, 1.0,-0.5,
      0.5, 0.0,-0.5,
      # front face: counter clockwise
      -0.5, 0.0, 0.5,
      0.5, 0.0, 0.5,
      0.5, 1.0, 0.5,
      -0.5, 1.0, 0.5,
      # letf face: counter clockwise
      -0.5, 0.0,-0.5,
      -0.5, 0.0, 0.5,
      -0.5, 1.0, 0.5,
      -0.5, 1.0,-0.5,
      # right face: counter clockwise
      0.5, 0.0,-0.5,
      0.5, 1.0,-0.5,
      0.5, 1.0, 0.5,
      0.5, 0.0, 0.5,
      # botton face: counter clockwise
      -0.5, 0.0,-0.5,
      0.5, 0.0,-0.5,
      0.5, 0.0, 0.5,
      -0.5, 0.0, 0.5,
      # top face: counter clockwise
      -0.5, 1.0,-0.5,
      -0.5, 1.0, 0.5,
      0.5, 1.0, 0.5,
      0.5, 1.0,-0.5
    ], dtype = 'float32')
    normals = np.array([
      # back face: counter clockwise
      0.0, 0.0,-1.0,
      0.0, 0.0,-1.0,
      0.0, 0.0,-1.0,
      0.0, 0.0,-1.0,
      # front face: counter clockwise
      0.0, 0.0, 1.0,
      0.0, 0.0, 1.0,
      0.0, 0.0, 1.0,
      0.0, 0.0, 1.0,
      # left face: counter clockwise
      -1.0, 0.0, 0.0,
      -1.0, 0.0, 0.0,
      -1.0, 0.0, 0.0,
      -1.0, 0.0, 0.0,
      # right face: counter clockwise
      1.0, 0.0, 0.0,
      1.0, 0.0, 0.0,
      1.0, 0.0, 0.0,
      1.0, 0.0, 0.0,
      # botton face: counter clockwise
      0.0,-1.0, 0.0,
      0.0,-1.0, 0.0,
      0.0,-1.0, 0.0,
      0.0,-1.0, 0.0,
      # top face: counter clockwise
      0.0, 1.0, 0.0,
      0.0, 1.0, 0.0,
      0.0, 1.0, 0.0,
      0.0, 1.0, 0.0,
    ], dtype = 'float32')
    # t cresce para baixo: o par de vértices de baixo de cada face recebe t = 1
    texcoords = np.array([
      # back face: counter clockwise
      0.0, 1.0,
      1.0, 1.0,
      1.0, 0.0,
      0.0, 0.0,
      # front face: counter clockwise
      0.0, 1.0,
      1.0, 1.0,
      1.0, 0.0,
      0.0, 0.0,
      # left face: counter clockwise
      0.0, 1.0,
      1.0, 1.0,
      1.0, 0.0,
      0.0, 0.0,
      # right face: counter clockwise
      0.0, 1.0,
      1.0, 1.0,
      1.0, 0.0,
      0.0, 0.0,
      # botton face: counter clockwise
      0.0, 1.0,
      1.0, 1.0,
      1.0, 0.0,
      0.0, 0.0,
      # top face: counter clockwise
      0.0, 1.0,
      1.0, 1.0,
      1.0, 0.0,
      0.0, 0.0,
    ], dtype = 'float32')
    tangents = np.array([
      # back face: counter clockwise
      -1.0, 0.0, 0.0,
      -1.0, 0.0, 0.0,
      -1.0, 0.0, 0.0,
      -1.0, 0.0, 0.0,
      # front face: counter clockwise
      1.0, 0.0, 0.0,
      1.0, 0.0, 0.0,
      1.0, 0.0, 0.0,
      1.0, 0.0, 0.0,
      # left face: counter clockwise
      0.0, 1.0, 0.0,
      0.0, 1.0, 0.0,
      0.0, 1.0, 0.0,
      0.0, 1.0, 0.0,
      # right face: counter clockwise
      0.0,-1.0, 0.0,
      0.0,-1.0, 0.0,
      0.0,-1.0, 0.0,
      0.0,-1.0, 0.0,
      # botton face: counter clockwise
      -1.0, 0.0, 0.0,
      -1.0, 0.0, 0.0,
      -1.0, 0.0, 0.0,
      -1.0, 0.0, 0.0,
      # top face: counter clockwise
      1.0, 0.0, 0.0,
      1.0, 0.0, 0.0,
      1.0, 0.0, 0.0,
      1.0, 0.0, 0.0,
    ], dtype = 'float32')
    index = np.array([
      0,1,2,0,2,3,
      4,5,6,4,6,7,
      8,9,10,8,10,11,
      12,13,14,12,14,15,
      16,17,18,16,18,19,
      20,21,22,20,22,23
    ], dtype = 'uint32')

    self.coord_vbo: wgpu.GPUBuffer = device.create_buffer_with_data(data=coords, usage=wgpu.BufferUsage.VERTEX)
    self.normal_vbo: wgpu.GPUBuffer = device.create_buffer_with_data(data=normals, usage=wgpu.BufferUsage.VERTEX)
    self.tangent_vbo: wgpu.GPUBuffer = device.create_buffer_with_data(data=tangents, usage=wgpu.BufferUsage.VERTEX)
    self.texcoord_vbo: wgpu.GPUBuffer = device.create_buffer_with_data(data=texcoords, usage=wgpu.BufferUsage.VERTEX)
    self.ibo: wgpu.GPUBuffer = device.create_buffer_with_data(data=index, usage=wgpu.BufferUsage.INDEX)

  def draw (self, st: State) -> None:
    first_instance = st.get_shader().commit_matrix(st)
    st.render_pass.set_vertex_buffer(0, self.coord_vbo)
    st.render_pass.set_vertex_buffer(1, self.normal_vbo)
    st.render_pass.set_vertex_buffer(2, self.tangent_vbo)
    st.render_pass.set_vertex_buffer(3, self.texcoord_vbo)
    st.render_pass.set_index_buffer(self.ibo, wgpu.IndexFormat.uint32)
    st.render_pass.draw_indexed(36, 1, 0, 0, first_instance)
