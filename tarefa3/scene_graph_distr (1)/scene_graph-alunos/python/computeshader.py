from __future__ import annotations

from typing import TYPE_CHECKING

import wgpu

import shaderutl as sutl

if TYPE_CHECKING:
  from texbuffer import TexBuffer

class ComputeShader:
  """Wraps a WGSL compute shader (entry point `main`) and the storage
  buffers it operates on. TexBuffers attached via attach_tex_buffer become
  bind group 0 entries in attachment order, matching the WGSL bindings.
  Pipeline and bind group are built lazily on the first dispatch call."""

  def __init__ (self, device: wgpu.GPUDevice, wgsl_path: str) -> None:
    """Compiles the WGSL compute module at `wgsl_path` and initializes
    empty texbuffer/pipeline/bind_group state."""
    self.device = device
    code = sutl.readfile(wgsl_path)
    self.module = device.create_shader_module(code=code)
    self.texbuffers: list[TexBuffer] = []
    self.pipeline: wgpu.GPUComputePipeline | None = None
    self.bind_group: wgpu.GPUBindGroup | None = None

  def attach_tex_buffer (self, texbuffer: TexBuffer) -> None:
    """Adds `texbuffer` as the next bind group 0 entry. Must be called
    before the first dispatch."""
    self.texbuffers.append(texbuffer)

  def dispatch (self, nx: int, ny: int = 1, nz: int = 1) -> None:
    """Runs the compute shader over a (nx, ny, nz) grid of workgroups,
    building the pipeline and bind group on the first call. Retains
    every attached TexBuffer at that point, so a later
    TexBuffer.set_data can refuse to replace a buffer this bind group
    already depends on - see TexBuffer.retain/set_data."""
    if self.pipeline is None:
      bind_group_layout = self.device.create_bind_group_layout(entries=[
        {"binding": i, "visibility": wgpu.ShaderStage.COMPUTE, "buffer": {"type": wgpu.BufferBindingType.storage}}
        for i in range(len(self.texbuffers))
      ])
      layout = self.device.create_pipeline_layout(bind_group_layouts=[bind_group_layout])
      self.pipeline = self.device.create_compute_pipeline(layout=layout, compute={"module": self.module, "entry_point": "main"})
      self.bind_group = self.device.create_bind_group(layout=bind_group_layout, entries=[
        {"binding": i, "resource": {"buffer": tb.get_buffer(), "offset": 0, "size": tb.nbytes}}
        for i, tb in enumerate(self.texbuffers)
      ])
      for tb in self.texbuffers:
        tb.retain()

    assert self.bind_group is not None  # set alongside self.pipeline just above

    encoder = self.device.create_command_encoder()
    pass_ = encoder.begin_compute_pass()
    pass_.set_pipeline(self.pipeline)
    pass_.set_bind_group(0, self.bind_group)
    pass_.dispatch_workgroups(nx, ny, nz)
    pass_.end()
    self.device.queue.submit([encoder.finish()])
