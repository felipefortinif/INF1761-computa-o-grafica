from __future__ import annotations

# Tarefa 2.1 - Cena 3D simples: uma mesa (cubos) com uma caixa amarela, uma
# esfera verde sobre a caixa e uma esfera vermelha sobre o tampo.
# Iluminacao por fragmento com uma luz pontual; camera com arcball.
#
# Controles:
#   arrastar com o botao do mouse -> gira a camera (arcball)
#   roda do mouse                 -> zoom
#   r                             -> volta a vista inicial
#   q                             -> sai

import os
import sys
from typing import Any

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
# ha um pacote "node" de terceiros pre-carregado no startup que sombreia o
# node.py do framework
sys.modules.pop("node", None)

import graphics_math as gm
import wgpu
from rendercanvas.glfw import RenderCanvas, loop

from camera3d import *
from pointlight import *
from shader import *
from pipeline import *
from phongmaterial import *
from transform import *
from node import *
from scene import *
from renderer import *
from cube import *
from sphere import *

SHADER_PATH = os.path.join(HERE, "..", "shaders", "ilum_frag", "lit_point.wgsl")

canvas: RenderCanvas
device: wgpu.GPUDevice
context: Any
renderer: Renderer
camera: Camera3D
scene: Scene

# dimensoes da mesa (unidades de mundo)
TABLE_W = 3.0       # largura (x)
TABLE_D = 2.0       # profundidade (z)
TABLE_H = 1.0       # altura das pernas
TOP_T = 0.2         # espessura do tampo
LEG_T = 0.2         # espessura das pernas
TOP_Y = TABLE_H + TOP_T   # altura da superficie do tampo

def make_material (r: float, g: float, b: float, spec: float, shininess: float) -> PhongMaterial:
  mat = PhongMaterial(r, g, b)
  mat.set_specular(spec, spec, spec)
  mat.set_shininess(shininess)
  return mat

def box (x: float, y: float, z: float, sx: float, sy: float, sz: float) -> Transform:
  """Transformacao de um cubo unitario (x,z em [-0.5,0.5], y em [0,1]) para
  uma caixa de dimensoes (sx,sy,sz) com o centro da base em (x,y,z)."""
  trf = Transform()
  trf.translate(x, y, z)
  trf.scale(sx, sy, sz)
  return trf

def sphere_at (x: float, y: float, z: float, r: float) -> Transform:
  """Transformacao da esfera unitaria para uma esfera de raio r com centro em (x,y,z)."""
  trf = Transform()
  trf.translate(x, y, z)
  trf.scale(r, r, r)
  return trf

def build_scene (device: wgpu.GPUDevice, target_format: str) -> tuple[Camera3D, Scene]:
  """Cria camera, luz, materiais e o grafo de cena."""
  cam = Camera3D(2.2, 3.0, 4.0)
  cam.set_center(0.0, TABLE_H, 0.0)

  # luz pontual fixa no mundo, acima e a frente da mesa; o shader trabalha
  # no espaco da camera e o framework mapeia a posicao com a matriz de vista
  light = PointLight(2.0, 5.0, 3.0, space="world")
  light.set_ambient(0.25, 0.25, 0.25)
  light.set_diffuse(0.8, 0.8, 0.8)
  light.set_specular(1.0, 1.0, 1.0)

  shader = Shader(device, SHADER_PATH, light=light, space="camera")
  shader.set_vertex_buffers([
    {"array_stride": 3 * 4, "step_mode": "vertex",
     "attributes": [{"format": "float32x3", "offset": 0, "var_name": "coord"}]},
    {"array_stride": 3 * 4, "step_mode": "vertex",
     "attributes": [{"format": "float32x3", "offset": 0, "var_name": "normal"}]},
  ])
  pipeline = Pipeline(shader, target_format, primitive={"cull_mode": "back"})

  wood = make_material(0.85, 0.85, 0.85, 0.3, 16.0)    # mesa (cinza claro)
  yellow = make_material(0.85, 0.60, 0.08, 0.4, 32.0)  # caixa
  green = make_material(0.35, 0.75, 0.30, 0.8, 64.0)   # esfera pequena
  red = make_material(0.95, 0.20, 0.20, 0.8, 64.0)     # esfera grande
  for mat in (wood, yellow, green, red):
    shader.add_material(mat)

  cube = Cube(device)
  sphere = Sphere(device)

  # mesa: tampo + 4 pernas, todos cubos escalados
  lx = TABLE_W/2 - LEG_T/2
  lz = TABLE_D/2 - LEG_T/2
  legs = [
    Node(None, box(sx*lx, 0.0, sz*lz, LEG_T, TABLE_H, LEG_T), None, [cube])
    for sx in (-1, 1) for sz in (-1, 1)
  ]
  table = Node(None, None, [wood],
               nodes = [Node(None, box(0.0, TABLE_H, 0.0, TABLE_W, TOP_T, TABLE_D), None, [cube])] + legs)

  # caixa amarela sobre o tampo, a frente a direita,
  # com a esfera verde apoiada nela; o grupo so translada, entao a
  # esfera nao herda a escala da caixa
  BOX_W, BOX_H, BOX_D = 0.9, 0.35, 0.7
  GREEN_R = 0.22
  trf_group = Transform()
  trf_group.translate(0.6, TOP_Y, 0.3)
  box_group = Node(None, trf_group,
                   nodes = [
                     Node(None, box(0.0, 0.0, 0.0, BOX_W, BOX_H, BOX_D), [yellow], [cube]),
                     Node(None, sphere_at(0.15, BOX_H + GREEN_R, 0.0, GREEN_R), [green], [sphere]),
                   ])

  # esfera vermelha apoiada diretamente no tampo, ao fundo a esquerda
  RED_R = 0.4
  red_ball = Node(None, sphere_at(-0.85, TOP_Y + RED_R, -0.4, RED_R), [red], [sphere])

  root = Node(pipeline, nodes = [table, box_group, red_ball])
  return cam, Scene(root)

def draw () -> None:
  target_texture = context.get_current_texture()
  renderer.render(target_texture, scene, camera)

def on_key (event: Any) -> None:
  if event["key"] == "q":
    canvas.close()
  elif event["key"] == "r":
    camera.get_arcball().mat = gm.mat4(1)

def on_wheel (event: Any) -> None:
  # aproxima/afasta ao longo do eixo de visao (dz proporcional a distancia)
  camera.get_arcball().translate(0.0, 0.0, -event["dy"] * 0.001)

def main () -> None:
  global canvas, device, context, renderer, camera, scene

  canvas = RenderCanvas(size=(800, 600), title="comp grafica", update_mode="continuous", max_fps=60)
  adapter = wgpu.gpu.request_adapter_sync()
  device = adapter.request_device_sync()
  context = canvas.get_context("wgpu")
  target_format = context.get_preferred_format(device.adapter)
  context.configure(device=device, format=target_format)

  renderer = Renderer(device, depth_test=True, clear_value=(1.0, 1.0, 1.0, 1.0))

  camera, scene = build_scene(device, target_format)
  arcball = camera.create_arcball()
  arcball.attach(canvas)

  canvas.add_event_handler(on_key, "key_down")
  canvas.add_event_handler(on_wheel, "wheel")
  canvas.request_draw(draw)
  loop.run()

if __name__ == "__main__":
  main()
