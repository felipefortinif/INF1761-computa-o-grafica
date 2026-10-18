from __future__ import annotations

import numpy as np
import wgpu

# simplified for the didactic use case (LUTs/transfer functions):
# only float32, 1 or 4 components per texel.
_FORMAT = {1: "r32float", 4: "rgba32float"}

class Texture1D:
  """1D texture bound to `varname`, meant for LUTs/transfer functions.
  Holds only float32 data, with 1 or 4 components per texel. A pure
  resource-holder - no load/unload of its own, see TextureSet.

  set_data (build/replace the texture) and update_data (upload new
  values into the existing one) are deliberately separate: once a bind
  group has been built from this texture's view (see retain - called
  by Shader.add_texture_set), replacing the texture would silently
  invalidate that bind group, which would still point at the old,
  destroyed one. set_data refuses to run while retained; update_data
  never touches the texture/view identity at all, so it's always safe."""

  def __init__ (self, device: wgpu.GPUDevice, varname: str, array: np.ndarray | None = None) -> None:
    """Leaves the texture unset (tex/view are None) unless `array` is given,
    in which case set_data builds it immediately."""
    self.device = device
    self.varname = varname
    self.tex: wgpu.GPUTexture | None = None
    self.view: wgpu.GPUTextureView | None = None
    self._width: int | None = None
    self._format: str | None = None
    self._ref_count = 0
    if array is not None:
      self.set_data(array)

  def retain (self) -> None:
    """Marks one more bind group as referencing this texture's current
    view - called by Shader.add_texture_set. Blocks set_data until a
    matching release()."""
    self._ref_count += 1

  def release (self) -> None:
    """Undoes one retain(). No current caller ever removes a
    TextureSet's registration, so this exists for completeness/future
    use rather than anything exercised today."""
    assert self._ref_count > 0, "release() called more often than retain()"
    self._ref_count -= 1

  def _format_for (self, array: np.ndarray) -> tuple[int, int, str]:
    width = array.shape[0]
    ncomp = 1 if array.ndim == 1 else array.shape[1]
    fmt = _FORMAT.get(ncomp)
    if fmt is None:
      raise RuntimeError("Unsupported array shape for Texture1D: " + str(array.shape))
    return width, ncomp, fmt

  def set_data (self, array: np.ndarray) -> None:
    """(Re)creates the texture from `array` - always allowed the first
    time (nothing retains it yet), but raises if any bind group
    currently references this texture (see retain), since replacing
    the GPU object would silently invalidate it. Use update_data
    instead to just upload new values into the same-shaped texture.
    Shape (width,) gives r32float, shape (width, 4) gives rgba32float;
    other shapes raise."""
    if self._ref_count > 0:
      raise RuntimeError(
        f"Texture1D is referenced by {self._ref_count} bind group(s) - "
        "cannot replace it (use update_data for a same-shape value update)"
      )
    array = np.asarray(array, dtype='float32')
    width, ncomp, fmt = self._format_for(array)
    if self.tex is not None:
      self.tex.destroy()
    self.tex = self.device.create_texture(
      size=(width, 1, 1), format=fmt, dimension="1d",
      usage=wgpu.TextureUsage.TEXTURE_BINDING | wgpu.TextureUsage.COPY_DST,
    )
    self.view = self.tex.create_view()
    self._width = width
    self._format = fmt
    self.device.queue.write_texture(
      {"texture": self.tex}, array.tobytes(),
      {"bytes_per_row": width * ncomp * 4}, (width, 1, 1),
    )

  def update_data (self, array: np.ndarray) -> None:
    """Uploads `array` into the existing texture - always allowed,
    regardless of retain()s, since the texture/view identity never
    changes. Raises if `array`'s width/format doesn't match the
    texture set_data last built (use set_data to change shape)."""
    if self.tex is None:
      raise RuntimeError("update_data called before set_data has ever built a texture")
    array = np.asarray(array, dtype='float32')
    width, ncomp, fmt = self._format_for(array)
    if width != self._width or fmt != self._format:
      raise RuntimeError(
        f"update_data requires the existing shape (width={self._width}, format={self._format}), "
        f"got (width={width}, format={fmt}) - use set_data to change shape"
      )
    self.device.queue.write_texture(
      {"texture": self.tex}, array.tobytes(),
      {"bytes_per_row": width * ncomp * 4}, (width, 1, 1),
    )

  def get_texture (self) -> wgpu.GPUTexture | None:
    """Returns the underlying texture, or None if set_data hasn't run yet."""
    return self.tex

  @property
  def resource (self) -> wgpu.GPUTextureView:
    """The GPU resource TextureSet/Shader.add_texture_set binds - this
    texture's view. Only valid after set_data has run."""
    assert self.view is not None
    return self.view
