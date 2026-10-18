from __future__ import annotations

import numpy as np
import wgpu

from computeshader import *
from texbuffer import *

def initialize (device: wgpu.GPUDevice) -> None:
  buf = TexBuffer(device, "data", np.array([1, 2, 3, 4], dtype='float32'))
  cs = ComputeShader(device, "../shaders/cs/compute.wgsl")
  cs.attach_tex_buffer(buf)
  cs.dispatch(1)
  print(buf.get_data())

def main () -> None:
  # exemplo puramente de compute — não precisa de janela/canvas
  adapter = wgpu.gpu.request_adapter_sync()
  device = adapter.request_device_sync()
  initialize(device)

if __name__ == "__main__":
  main()
