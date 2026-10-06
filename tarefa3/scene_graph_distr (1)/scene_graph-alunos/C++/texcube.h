#include <memory>
class TexCube;
using TexCubePtr = std::shared_ptr<TexCube>;

#ifndef TEXCUBE_H
#define TEXCUBE_H

#include <wgpu.h>
#include <string>
#include "textureset.h"

// Cubemap texture bound to varname, loaded from a single cross-layout
// image file (4x3 grid: right,left,bottom,top,front,back) and sliced
// into the 6 layers of a rgba8unorm-srgb cube texture. A pure
// resource-holder - no Load/Unload of its own, see TextureSet.
class TexCube : public TextureItem {
  std::string m_varname;
  WGPUTexture m_tex;
  WGPUTextureView m_view;
protected:
  TexCube (WGPUDevice device, const std::string& varname, const std::string& filename);
public:
  static TexCubePtr Make (WGPUDevice device, const std::string& varname, const std::string& filename);
  ~TexCube ();

  WGPUTexture GetTexture () const;
  const std::string& GetVarName () const override;
  WGPUBindGroupEntry MakeEntry (uint32_t binding) const override;
};

#endif
