from __future__ import annotations

import wgpu

class TexDepth:
  """Depth texture (depth32float) bound to `varname`, usable both as a
  render pass's depth-stencil attachment and as a sampled texture (e.g.
  shadow mapping). Shaders must declare it as `texture_depth_2d`, not
  `texture_2d<f32>`. The sampler is a separate object. A pure
  resource-holder - no load/unload of its own, see TextureSet."""

  def __init__ (self, device: wgpu.GPUDevice, varname: str, width: int, height: int) -> None:
    """Allocates a depth32float texture of size width x height and its view."""
    self.varname = varname
    self.width = width
    self.height = height
    self.tex: wgpu.GPUTexture = device.create_texture(
      size=(width, height, 1), format="depth32float",
      usage=wgpu.TextureUsage.RENDER_ATTACHMENT | wgpu.TextureUsage.TEXTURE_BINDING,
    )
    self.view: wgpu.GPUTextureView = self.tex.create_view()

  def get_texture (self) -> wgpu.GPUTexture:
    """Returns the raw texture, for a depth-stencil attachment. Use
    `resource` (the same view) for sampling."""
    return self.tex

  @property
  def resource (self) -> wgpu.GPUTextureView:
    """The GPU resource TextureSet/Shader.add_texture_set binds - this
    depth texture's view."""
    return self.view
