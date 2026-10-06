from __future__ import annotations

from typing import TYPE_CHECKING

import wgpu

if TYPE_CHECKING:
  from camera import Camera
  from scene import Scene
  from framebuffer import Framebuffer

class Algorithm:
  """Encoder/render-pass/submit mechanics for multi-pass techniques (shadow
  mapping, planar shadows, reflections) that don't fit Renderer's
  one-pass shape. Stateless - build once, reuse for the app's lifetime.
  Attachments come from a caller-built Framebuffer; pipeline/transform
  selection stays the app's job via Node."""

  def __init__ (self, device: wgpu.GPUDevice) -> None:
    self.device = device

  def render_pass (self, framebuffer: Framebuffer, scene: Scene, camera: Camera) -> None:
    """Encodes, runs and submits one full render pass into `framebuffer`,
    rendering `scene` through `camera`. Self-contained - safe to call
    multiple times per frame for multi-pass techniques."""
    if framebuffer.color_attachments:
      w, h, _ = framebuffer.color_attachments[0]["view"].size
    elif framebuffer.depth_stencil_attachment is not None:
      w, h, _ = framebuffer.depth_stencil_attachment["view"].size
    else:
      raise ValueError("Framebuffer needs at least one color or depth/stencil attachment")
    size = (w, h)

    encoder = self.device.create_command_encoder()
    render_pass = encoder.begin_render_pass(
      color_attachments=framebuffer.color_attachments,
      depth_stencil_attachment=framebuffer.depth_stencil_attachment,
    )
    from state import State
    st = State(camera, self.device, render_pass, size)
    scene.render(st)
    # a travessia so montou as linhas na CPU; um write_buffer por shader,
    # aqui, antes do submit (write_buffer esta na fila, o draw no encoder)
    for shd in st.get_matrix_shaders():
      shd.flush_matrices()
    render_pass.end()
    self.device.queue.submit([encoder.finish()])
