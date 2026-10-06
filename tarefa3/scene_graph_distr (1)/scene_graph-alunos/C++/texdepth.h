#include <memory>
class TexDepth;
using TexDepthPtr = std::shared_ptr<TexDepth>;

#ifndef TEXDEPTH_H
#define TEXDEPTH_H

#include <wgpu.h>
#include <string>
#include "textureset.h"

// Depth texture (depth32float) bound to varname, usable both as a render
// pass's depth-stencil attachment and as a sampled texture (e.g. shadow
// mapping). Shaders must declare it as texture_depth_2d, not
// texture_2d<f32>. The sampler is a separate object. A pure
// resource-holder - no Load/Unload of its own, see TextureSet.
class TexDepth : public TextureItem {
  std::string m_varname;
  int m_width, m_height;
  WGPUTexture m_tex;
  WGPUTextureView m_view;
protected:
  TexDepth (WGPUDevice device, const std::string& varname, int width, int height);
public:
  static TexDepthPtr Make (WGPUDevice device, const std::string& varname, int width, int height);
  ~TexDepth ();

  // Returns the raw texture, for a depth-stencil attachment. Use
  // GetView() (the same view) for a render-pass attachment descriptor.
  WGPUTexture GetTexture () const;
  WGPUTextureView GetView () const;
  int GetWidth () const;
  int GetHeight () const;

  const std::string& GetVarName () const override;
  WGPUBindGroupEntry MakeEntry (uint32_t binding) const override;
};

#endif
