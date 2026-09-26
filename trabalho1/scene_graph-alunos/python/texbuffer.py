from __future__ import annotations

from typing import cast

import numpy as np
import wgpu

class TexBuffer:
  """Storage buffer para uso em compute shaders. O formato/tipo do elemento
  é declarado no próprio shader WGSL (ex. `array<f32>`).

  set_data (build/replace the buffer) and update_data (upload new
  values into the existing one) are deliberately separate: once a
  ComputeShader's bind group has been built from this buffer (see
  retain - called from ComputeShader.dispatch's first call), replacing
  the buffer would silently invalidate that bind group. set_data
  refuses to run while retained; update_data never touches the buffer
  identity at all, so it's always safe."""

  def __init__ (self, device: wgpu.GPUDevice, varname: str, array: np.ndarray) -> None:
    """Stores `varname`/`device` and builds the buffer from `array`."""
    self.varname = varname
    self.device = device
    self.buffer: wgpu.GPUBuffer | None = None
    self.nbytes: int = 0
    self._ref_count = 0
    self.set_data(array)

  def retain (self) -> None:
    """Marks one more bind group as referencing this buffer - called
    by ComputeShader.dispatch on its first call. Blocks set_data until
    a matching release()."""
    self._ref_count += 1

  def release (self) -> None:
    """Undoes one retain(). No current caller ever detaches a
    ComputeShader's TexBuffer, so this exists for completeness/future
    use rather than anything exercised today."""
    assert self._ref_count > 0, "release() called more often than retain()"
    self._ref_count -= 1

  def set_data (self, array: np.ndarray) -> None:
    """(Re)creates the buffer from `array` - always allowed the first
    time (nothing retains it yet), but raises if any ComputeShader
    dispatch has already bound this buffer (see retain), since
    replacing the GPU object would silently invalidate it. Use
    update_data instead to just upload new values into the
    same-sized buffer. float64 input is silently downcast to float32."""
    if self._ref_count > 0:
      raise RuntimeError(
        f"TexBuffer is referenced by {self._ref_count} bind group(s) - "
        "cannot replace it (use update_data for a same-size value update)"
      )
    if array.dtype == np.float64:
      array = np.array(array, dtype='float32')
    if self.buffer is not None:
      self.buffer.destroy()
    self.dtype = array.dtype
    self.nbytes = array.nbytes
    self.buffer = self.device.create_buffer_with_data(
      data=array, usage=wgpu.BufferUsage.STORAGE | wgpu.BufferUsage.COPY_SRC | wgpu.BufferUsage.COPY_DST,
    )

  def update_data (self, array: np.ndarray) -> None:
    """Uploads `array` into the existing buffer - always allowed,
    regardless of retain()s, since the buffer identity never changes.
    Raises if `array`'s byte size doesn't match the buffer set_data
    last built (use set_data to resize). float64 input is silently
    downcast to float32."""
    if self.buffer is None:
      raise RuntimeError("update_data called before set_data has ever built a buffer")
    if array.dtype == np.float64:
      array = np.array(array, dtype='float32')
    if array.nbytes != self.nbytes:
      raise RuntimeError(
        f"update_data requires the existing size ({self.nbytes} bytes), got {array.nbytes} bytes - "
        "use set_data to resize"
      )
    self.dtype = array.dtype
    self.device.queue.write_buffer(self.buffer, 0, array)

  def get_buffer (self) -> wgpu.GPUBuffer:
    assert self.buffer is not None  # __init__ always calls set_data
    return self.buffer

  def get_data (self) -> np.ndarray:
    """Synchronously reads the buffer back from the GPU, for inspecting
    compute shader output. Not meant for per-frame use."""
    assert self.buffer is not None  # __init__ always calls set_data
    # cast: read_buffer's ArrayLike return type is wider than the stub's
    # np.frombuffer will accept, but it's actually a real buffer at runtime.
    raw = self.device.queue.read_buffer(self.buffer, size=self.nbytes)
    return np.frombuffer(cast(bytes, raw), dtype=self.dtype).copy()
