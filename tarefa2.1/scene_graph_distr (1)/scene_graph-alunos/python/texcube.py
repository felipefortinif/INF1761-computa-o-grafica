from __future__ import annotations

from PIL import Image
import numpy as np
import wgpu

# standard cubemap layer order in WebGPU: +X,-X,+Y,-Y,+Z,-Z. The
# sub-images of the cross layout (right,left,bottom,top,front,back) map to
# these layers in this order:
_LAYER_FOR_CROP_INDEX = [0, 1, 3, 2, 4, 5]  # right,left,bottom,top,front,back -> layer

class TexCube:
  """Cubemap texture bound to `varname`, loaded from a single cross-layout
  image file (4x3 grid: right,left,bottom,top,front,back) and sliced into
  the 6 layers of a rgba8unorm-srgb cube texture. A pure resource-holder
  - no load/unload of its own, see TextureSet."""

  def __init__ (self, device: wgpu.GPUDevice, varname: str, filename: str) -> None:
    """Loads `filename`'s cross layout, crops it into 6 sub-images and
    uploads each to its corresponding cube layer."""
    self.varname = varname
    img = Image.open(filename).convert("RGBA")
    width, height = img.size
    w = width // 4
    h = height // 3
    x = [2*w,  0,  w,  w,  w,3*w]
    y = [  h,  h,2*h,  0,  h,  h]

    # "-srgb": same reasoning as in texture.py - the source image is
    # already sRGB-encoded, the GPU undoes the curve when sampling.
    self.tex: wgpu.GPUTexture = device.create_texture(
      size=(w, h, 6), format="rgba8unorm-srgb", dimension="2d",
      usage=wgpu.TextureUsage.TEXTURE_BINDING | wgpu.TextureUsage.COPY_DST,
    )
    for i in range(6):
      subimg = img.crop((x[i],y[i],x[i]+w,y[i]+h))
      layer = _LAYER_FOR_CROP_INDEX[i]
      device.queue.write_texture(
        {"texture": self.tex, "origin": (0, 0, layer)}, np.array(subimg).tobytes(),
        {"bytes_per_row": w * 4, "rows_per_image": h}, (w, h, 1),
      )
    self.view: wgpu.GPUTextureView = self.tex.create_view(dimension="cube")

  def get_texture (self) -> wgpu.GPUTexture:
    return self.tex

  @property
  def resource (self) -> wgpu.GPUTextureView:
    """The GPU resource TextureSet/Shader.add_texture_set binds - this
    cubemap's view."""
    return self.view
