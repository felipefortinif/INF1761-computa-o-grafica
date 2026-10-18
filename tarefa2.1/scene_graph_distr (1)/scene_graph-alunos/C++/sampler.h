#include <memory>
class Sampler;
using SamplerPtr = std::shared_ptr<Sampler>;

#ifndef SAMPLER_H
#define SAMPLER_H

#include <wgpu.h>
#include <string>
#include <optional>
#include "textureset.h"

// Sampling rule (addressing/filter), independent of any Texture class. A
// pure resource-holder - no Load/Unload of its own, see TextureSet (which
// can freely combine any Texture/Sampler pair or larger set, in any
// order).
class Sampler : public TextureItem {
  std::string m_varname;
  WGPUSampler m_sampler;
protected:
  Sampler (WGPUDevice device, const std::string& varname,
           WGPUAddressMode addressModeU, WGPUAddressMode addressModeV,
           WGPUFilterMode magFilter, WGPUFilterMode minFilter, WGPUMipmapFilterMode mipmapFilter,
           std::optional<WGPUCompareFunction> compare);
public:
  static SamplerPtr Make (WGPUDevice device, const std::string& varname,
                           WGPUAddressMode addressModeU = WGPUAddressMode_Repeat,
                           WGPUAddressMode addressModeV = WGPUAddressMode_Repeat,
                           WGPUFilterMode magFilter = WGPUFilterMode_Linear,
                           WGPUFilterMode minFilter = WGPUFilterMode_Linear,
                           WGPUMipmapFilterMode mipmapFilter = WGPUMipmapFilterMode_Linear,
                           std::optional<WGPUCompareFunction> compare = std::nullopt);
  ~Sampler ();

  const std::string& GetVarName () const override;
  WGPUBindGroupEntry MakeEntry (uint32_t binding) const override;
};

#endif
