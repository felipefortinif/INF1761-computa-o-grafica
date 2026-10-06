#include <memory>
class TextureSet;
using TextureSetPtr = std::shared_ptr<TextureSet>;

#ifndef TEXTURESET_H
#define TEXTURESET_H

#include <wgpu.h>
#include <string>
#include <vector>
#include <initializer_list>
#include "appearance.h"

// Common interface for anything a TextureSet can bind: a texture (as a
// WGPUTextureView) or a sampler. varname must match the WGSL binding
// it's meant for - resolved to (group, binding) via the shader's
// reflection in Shader::AddTextureSet.
class TextureItem {
public:
  virtual ~TextureItem () {}
  virtual const std::string& GetVarName () const = 0;
  virtual WGPUBindGroupEntry MakeEntry (uint32_t binding) const = 0;
  virtual void Retain () {}
};

// Bundles already-built texture/sampler items into one persistent
// GPUBindGroup, built by Shader::AddTextureSet (which validates the
// bundle covers the shader's texture/sampler group exactly). Load/Unload
// delegate to Shader::BindTextureSet/UnbindTextureSet.
class TextureSet : public Appearance {
  std::vector<TextureItem*> m_items;

protected:
  TextureSet (std::initializer_list<TextureItem*> items);

public:
  static TextureSetPtr Make (std::initializer_list<TextureItem*> items);
  const std::vector<TextureItem*>& GetItems () const;

  void Load (StatePtr st) override;
  void Unload (StatePtr st) override;
};

#endif
