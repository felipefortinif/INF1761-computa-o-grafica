from __future__ import annotations

# Renderiza a cena de main_spot.py fora da tela (sem abrir janela) e salva
# PNGs, para conferir o efeito do spot e gerar as figuras do relatorio.
#   python screenshot.py [pasta_de_saida]

import math
import os
import sys

import main_spot  # ajusta sys.path e o modulo "node" antes dos imports abaixo

import graphics_math as gm
import numpy as np
import wgpu
from PIL import Image

from renderer import Renderer

WIDTH, HEIGHT = 800, 600
FORMAT = "rgba8unorm"

def render (device, renderer, scene, camera, path: str) -> None:
  texture = device.create_texture(
    size=(WIDTH, HEIGHT, 1), format=FORMAT,
    usage=wgpu.TextureUsage.RENDER_ATTACHMENT | wgpu.TextureUsage.COPY_SRC,
  )
  renderer.render(texture, scene, camera)
  bytes_per_row = (WIDTH * 4 + 255) // 256 * 256
  buffer = device.create_buffer(size=bytes_per_row * HEIGHT,
                                usage=wgpu.BufferUsage.COPY_DST | wgpu.BufferUsage.MAP_READ)
  encoder = device.create_command_encoder()
  encoder.copy_texture_to_buffer(
    {"texture": texture},
    {"buffer": buffer, "bytes_per_row": bytes_per_row, "rows_per_image": HEIGHT},
    (WIDTH, HEIGHT, 1),
  )
  device.queue.submit([encoder.finish()])
  buffer.map_sync(wgpu.MapMode.READ)
  data = np.frombuffer(buffer.read_mapped(), dtype=np.uint8).reshape(HEIGHT, bytes_per_row)
  buffer.unmap()
  rgba = data[:, :WIDTH * 4].reshape(HEIGHT, WIDTH, 4)
  Image.fromarray(np.ascontiguousarray(rgba)).convert("RGB").save(path)
  print("salvo:", path)

def orbit (camera, yaw: float, pitch: float) -> None:
  """Simula um arrasto do arcball: gira a vista em torno do centro."""
  arcball = camera.get_arcball()
  d = arcball.distance
  m = gm.mat4(1)
  m = gm.translate(m, gm.vec3(0, 0, -d))
  m = gm.rotate(m, gm.radians(pitch), gm.vec3(1, 0, 0))
  m = gm.rotate(m, gm.radians(yaw), gm.vec3(0, 1, 0))
  m = gm.translate(m, gm.vec3(0, 0, d))
  arcball.mat = m

def main () -> None:
  out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(main_spot.HERE, "images")
  os.makedirs(out, exist_ok=True)
  adapter = wgpu.gpu.request_adapter_sync()
  device = adapter.request_device_sync()
  renderer = Renderer(device, depth_test=True, clear_value=(1.0, 1.0, 1.0, 1.0))
  camera, scene, light = main_spot.build_scene(device, FORMAT)
  camera.create_arcball()

  render(device, renderer, scene, camera, os.path.join(out, "spot_inicial.png"))
  orbit(camera, -40.0, 15.0)
  render(device, renderer, scene, camera, os.path.join(out, "spot_arcball.png"))
  orbit(camera, 0.0, 0.0)

  # mesma vista inicial, variando o expoente
  for exponent in (0.0, 40.0):
    light.set_exponent(exponent)
    render(device, renderer, scene, camera, os.path.join(out, f"spot_exp{exponent:.0f}.png"))

if __name__ == "__main__":
  main()
