from __future__ import annotations

import wgpu

from node import *
from mesh import *
from phongmaterial import *
from transform import *
from luxor.luxorengine import *

class Luxor:
  """Builds the Luxo-lamp scene graph (base, arm segments, head, bulb) as
  nested Nodes, plus the matching LuxorEngine that animates it."""

  def __init__ (self, device: wgpu.GPUDevice) -> None:
    """Load meshes/materials, build the Node hierarchy (self.node the
    root, self.light_node the bulb), and construct the matching
    LuxorEngine."""
    base_a = Mesh(device, "../../meshes/luxor/base_a.msh")
    base_b = Mesh(device, "../../meshes/luxor/base_b.msh")
    haste1 = Mesh(device, "../../meshes/luxor/haste1.msh")
    haste2 = Mesh(device, "../../meshes/luxor/haste2.msh")
    haste3_a = Mesh(device, "../../meshes/luxor/haste3_a.msh")
    haste3_b = Mesh(device, "../../meshes/luxor/haste3_b.msh")
    cupula_a = Mesh(device, "../../meshes/luxor/cupula_a.msh")
    cupula_b = Mesh(device, "../../meshes/luxor/cupula_b.msh")
    lampada = Mesh(device, "../../meshes/luxor/lampada.msh")
    red = PhongMaterial(1.0,0.0,0.0)
    white = PhongMaterial(1.0,1.0,1.0)
    self.materials = [red, white]
    trf_all = Transform()
    trf_base = Transform()
    trf_haste1 = Transform()
    trf_haste2 = Transform()
    trf_haste3 = Transform()
    trf_cupula = Transform()
    trf_lampada = Transform()
    trf_haste1.translate(0.0,4.0,0.0)
    trf_haste2.translate(0.0,17.15,0.0)
    trf_haste3.translate(0.0,16.78,0.0)
    trf_cupula.translate(0.0,18.12,0.0)
    trf_lampada.translate(0.0,8.4,9.0)
    self.light_node = Node(None,trf_lampada,[white],[lampada])
    self.node = Node(None,trf_all,[red],
                     nodes = [
                               Node(None,trf_base,shps=[base_a,base_b],nodes=[
                                 Node(None,trf_haste1,shps=[haste1],nodes=[
                                   Node(None,trf_haste2,shps=[haste2],nodes=[
                                     Node(None,trf_haste3,shps=[haste3_a,haste3_b],nodes=[
                                       Node(None,trf_cupula,shps=[cupula_a,cupula_b],nodes=[
                                         self.light_node
                                       ])
                                     ])
                                   ])
                                 ])
                               ])
                             ]
                    )
    self.engine = LuxorEngine(trf_all,trf_base,trf_haste1,trf_haste2,trf_haste3,trf_cupula,trf_lampada)

  def get_node (self) -> Node:
    return self.node

  def get_light_node (self) -> Node:
    return self.light_node

  def get_engine (self) -> LuxorEngine:
    return self.engine

  def get_materials (self) -> list[PhongMaterial]:
    """Returns every Material this Luxor lamp uses, so the caller can
    shader.add_material(...) each of them once its Shader exists."""
    return self.materials