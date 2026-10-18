from __future__ import annotations

from typing import Any
import time

import wgpu
from rendercanvas.glfw import RenderCanvas, loop

from camera2d import *
from colormaterial import *
from transform import *
from quad import *
from triangle import *
from node import *
from shader import *
from pipeline import *
from scene import *
from renderer import *
from engine import *

canvas: RenderCanvas
device: wgpu.GPUDevice
context: Any
renderer: Renderer
camera: Camera2D
scene: Scene
last_t: float = 0.0

class MovePointer(Engine):
  def __init__ (self, trf: Transform) -> None:
    self.trf = trf
  def update (self, dt: float) -> None:
    self.trf.rotate(6*dt,0,0,-1)

def initialize (device: wgpu.GPUDevice, target_format: str) -> None:
  # create objects
  global camera
  camera = Camera2D(0,10,0,10)

  trf1 = Transform()
  trf1.translate(3,3,-0.5)
  trf1.scale(4,4,1)
  face_material = ColorMaterial(1,1,1)
  face = Node(trf=trf1,apps=[face_material],shps=[Quad(device)])
  trf2 = Transform()
  trf2.translate(5,5,0)
  trf3 = Transform()
  trf3.scale(0.1,2,1)
  pointer_material = ColorMaterial(1,0,0)
  pointer = Node(trf=trf2,nodes=[Node(trf=trf3,apps=[pointer_material],shps=[Triangle(device)])])

  shader = Shader(device, "../shaders/2d/shader.wgsl")
  shader.set_vertex_buffers([
    {"array_stride": 2 * 4, "step_mode": "vertex",
     "attributes": [{"format": "float32x2", "offset": 0, "var_name": "pos"}]},
  ])
  pipeline = Pipeline(shader, target_format, depth_stencil=None)
  shader.add_material(face_material)
  shader.add_material(pointer_material)

  # build scene
  root = Node(pipeline, nodes = [face,pointer])
  global scene
  scene = Scene(root)
  scene.add_engine(MovePointer(trf2))

def update (dt: float) -> None:
  scene.update(dt)

def draw () -> None:
  global last_t
  t = time.perf_counter()
  update(t - last_t)
  last_t = t

  target_texture = context.get_current_texture()
  renderer.render(target_texture, scene, camera)

def on_key (event: Any) -> None:
  if event["key"] == "q":
    canvas.close()

def main () -> None:
  global canvas, device, context, renderer, last_t

  canvas = RenderCanvas(size=(600, 600), title="2D scene", update_mode="continuous", max_fps=60)
  adapter = wgpu.gpu.request_adapter_sync()
  device = adapter.request_device_sync()
  context = canvas.get_context("wgpu")
  # formato preferido COM "-srgb": a GPU codifica de linear para sRGB
  # automaticamente na saída — correto desde que o shader faça a conta de
  # iluminação em espaço linear.
  target_format = context.get_preferred_format(device.adapter)
  context.configure(device=device, format=target_format)

  renderer = Renderer(device, clear_value=(0.8, 1.0, 1.0, 1.0))

  initialize(device, target_format)

  canvas.add_event_handler(on_key, "key_down")
  last_t = time.perf_counter()
  canvas.request_draw(draw)
  loop.run()

if __name__ == "__main__":
  main()
