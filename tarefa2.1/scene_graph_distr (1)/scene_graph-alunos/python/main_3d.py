from __future__ import annotations

from typing import Any

import graphics_math as gm
import wgpu
from rendercanvas.glfw import RenderCanvas, loop

from camera3d import *
from pointlight import *
from shader import *
from pipeline import *
from phongmaterial import *
from texture import *
from sampler import *
from textureset import *
from transform import *
from node import *
from scene import *
from renderer import *
from cube import *
from sphere import *
from quad import *

canvas: RenderCanvas
device: wgpu.GPUDevice
context: Any
renderer: Renderer
camera: Camera3D
scene: Scene

viewer_pos: gm.Vec3 = gm.vec3(2.0, 3.5, 4.0)

def initialize (device: wgpu.GPUDevice, target_format: str) -> None:
  global camera, scene

  # create objects
  camera = Camera3D(viewer_pos[0],viewer_pos[1],viewer_pos[2])
  arcball = camera.create_arcball()
  arcball.attach(canvas)

  light = PointLight(0.0,0.0,0.0, space="camera")

  white = PhongMaterial(1.0,1.0,1.0)
  red = PhongMaterial(1.0,0.5,0.5)
  paper = Texture(device,"decal_texture","../images/paper.jpg")
  paper_sampler = Sampler(device,"decal_sampler")

  trf1 = Transform()
  trf1.scale(3.0,0.3,3.0)
  trf1.translate(0.0,-1.0,0.0)
  trf2 = Transform()
  trf2.scale(0.5,0.5,0.5)
  trf2.translate(0.0,1.0,0.0)
  trf3 = Transform()
  trf3.translate(0.8,0.0,0.8)
  trf3.rotate(30.0,0.0,1.0,0.0)
  trf3.rotate(90.0,-1.0,0.0,0.0)
  trf3.scale(0.5,0.7,1.0)

  cube = Cube(device)
  quad = Quad(device)
  sphere = Sphere(device)

  shader = Shader(device, "../shaders/ilum_frag/lit.wgsl", light=light, space="world")
  shader.set_vertex_buffers([
    {"array_stride": 3 * 4, "step_mode": "vertex",
     "attributes": [{"format": "float32x3", "offset": 0, "var_name": "coord"}]},
    {"array_stride": 3 * 4, "step_mode": "vertex",
     "attributes": [{"format": "float32x3", "offset": 0, "var_name": "normal"}]},
  ])
  pipeline = Pipeline(shader, target_format, primitive={"cull_mode": "back"})

  # depth_bias/depth_bias_slope_scale replace the old dynamic PolygonOffset
  # (avoids z-fighting between the paper decal and the "floor" right below it)
  shd_tex = Shader(device, "../shaders/ilum_frag/textured.wgsl", light=light, space="world")
  shd_tex.set_vertex_buffers([
    {"array_stride": 2 * 4, "step_mode": "vertex",
     "attributes": [{"format": "float32x2", "offset": 0, "var_name": "coord"}]},
    {"array_stride": 2 * 4, "step_mode": "vertex",
     "attributes": [{"format": "float32x2", "offset": 0, "var_name": "texcoord"}]},
  ])
  pipeline_tex = Pipeline(shd_tex, target_format, primitive={"cull_mode": "none"},
                           depth_stencil={"depth_bias": -1, "depth_bias_slope_scale": -1})

  # white is used under both shaders (the plain sphere and the
  # textured quad), so it's registered on each independently.
  shader.add_material(white)
  shader.add_material(red)
  shd_tex.add_material(white)
  paper_textures = TextureSet([paper, paper_sampler])
  shd_tex.add_texture_set(paper_textures)

  # build scene
  root = Node(pipeline,
              nodes = [
                        Node(None,trf1,[red],[cube]),
                        Node(pipeline_tex,trf3,[white,paper_textures],[quad]),
                        Node(None,trf2,[white],[sphere])
                      ]
              )
  scene = Scene(root)

def draw () -> None:
  target_texture = context.get_current_texture()
  renderer.render(target_texture, scene, camera)

def on_key (event: Any) -> None:
  if event["key"] == "q":
    canvas.close()

def main () -> None:
  global canvas, device, context, renderer

  canvas = RenderCanvas(size=(640, 480), title="3D scene", update_mode="continuous", max_fps=60)
  adapter = wgpu.gpu.request_adapter_sync()
  device = adapter.request_device_sync()
  context = canvas.get_context("wgpu")
  # formato preferido COM "-srgb": a GPU codifica de linear para sRGB
  # automaticamente na saída — correto desde que o shader faça a conta de
  # iluminação em espaço linear.
  target_format = context.get_preferred_format(device.adapter)
  context.configure(device=device, format=target_format)

  renderer = Renderer(device, depth_test=True, clear_value=(1.0, 1.0, 1.0, 1.0))

  initialize(device, target_format)

  canvas.add_event_handler(on_key, "key_down")
  canvas.request_draw(draw)
  loop.run()

if __name__ == "__main__":
  main()
