from __future__ import annotations

from typing import TYPE_CHECKING

import numpy as np
import wgpu
from shape import Shape

if TYPE_CHECKING:
  from state import State

class Mesh (Shape):
  """Loads an indexed triangle mesh from a text format ("V"/"N"/"T" lines
  for coords/normals/triangles; blank lines, "#" comments, and "--"
  section separators are ignored - the latter appears in some of the
  shipped .msh files, apparently as a purely visual break between the
  V/N block and the T block). Vertex buffers: slot 0 = coords, slot 1
  = normals, plus a uint32 index buffer. Raises ValueError (with
  filename:line) on a malformed/unrecognized record, a vertex/normal
  count mismatch, or an out-of-range triangle index."""

  def __init__ (self, device: wgpu.GPUDevice, filename: str) -> None:
    coords: list[float] = []
    normals: list[float] = []
    indices: list[int] = []
    with open(filename) as f:
      for lineno, line in enumerate(f, start=1):
        elems = line.split()
        if not elems or elems[0].startswith("#") or elems[0].startswith("--"):
          continue
        tag = elems[0]
        try:
          if tag == "V":
            if len(elems) != 4:
              raise ValueError(f"'V' record needs 3 values, got {len(elems) - 1}")
            coords.extend(float(e) for e in elems[1:4])
          elif tag == "N":
            if len(elems) != 4:
              raise ValueError(f"'N' record needs 3 values, got {len(elems) - 1}")
            normals.extend(float(e) for e in elems[1:4])
          elif tag == "T":
            if len(elems) != 4:
              raise ValueError(f"'T' record needs 3 values, got {len(elems) - 1}")
            indices.extend(int(e) for e in elems[1:4])
          else:
            raise ValueError(f"unrecognized record type '{tag}'")
        except ValueError as e:
          raise ValueError(f"{filename}:{lineno}: {e}") from None

    nverts = len(coords) // 3
    if len(normals) // 3 != nverts:
      raise ValueError(
        f"{filename}: {nverts} vertex coords but {len(normals) // 3} normals - "
        "a normal is required for every vertex (Mesh.draw always binds vertex slot 1 to it)"
      )
    if not indices:
      raise ValueError(f"{filename}: no triangles ('T' records) found")
    bad = [i for i in indices if i < 0 or i >= nverts]
    if bad:
      raise ValueError(f"{filename}: triangle index {bad[0]} out of range for {nverts} vertices")

    vcoords = np.array(coords,dtype='float32')
    vnormals = np.array(normals,dtype='float32')
    vindices = np.array(indices,dtype='uint32')
    self.nind: int = len(indices)
    self.coord_vbo: wgpu.GPUBuffer = device.create_buffer_with_data(data=vcoords, usage=wgpu.BufferUsage.VERTEX)
    self.normal_vbo: wgpu.GPUBuffer = device.create_buffer_with_data(data=vnormals, usage=wgpu.BufferUsage.VERTEX)
    self.ibo: wgpu.GPUBuffer = device.create_buffer_with_data(data=vindices, usage=wgpu.BufferUsage.INDEX)

  def draw (self, st: State) -> None:
    first_instance = st.get_shader().commit_matrix(st)
    st.render_pass.set_vertex_buffer(0, self.coord_vbo)
    st.render_pass.set_vertex_buffer(1, self.normal_vbo)
    st.render_pass.set_index_buffer(self.ibo, wgpu.IndexFormat.uint32)
    st.render_pass.draw_indexed(self.nind, 1, 0, 0, first_instance)
