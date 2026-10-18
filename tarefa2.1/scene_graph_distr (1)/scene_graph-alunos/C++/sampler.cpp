#include "sampler.h"

Sampler::Sampler (WGPUDevice device, const std::string& varname,
                   WGPUAddressMode addressModeU, WGPUAddressMode addressModeV,
                   WGPUFilterMode magFilter, WGPUFilterMode minFilter, WGPUMipmapFilterMode mipmapFilter,
                   std::optional<WGPUCompareFunction> compare)
  : m_varname(varname)
{
  WGPUSamplerDescriptor desc = {};
  desc.addressModeU = addressModeU;
  desc.addressModeV = addressModeV;
  desc.magFilter = magFilter;
  desc.minFilter = minFilter;
  desc.mipmapFilter = mipmapFilter;
  desc.lodMinClamp = 0.0f;
  desc.lodMaxClamp = 32.0f;
  desc.maxAnisotropy = 1;
  if (compare) desc.compare = *compare;
  m_sampler = wgpuDeviceCreateSampler(device, &desc);
}

SamplerPtr Sampler::Make (WGPUDevice device, const std::string& varname,
                           WGPUAddressMode addressModeU, WGPUAddressMode addressModeV,
                           WGPUFilterMode magFilter, WGPUFilterMode minFilter, WGPUMipmapFilterMode mipmapFilter,
                           std::optional<WGPUCompareFunction> compare)
{
  return SamplerPtr(new Sampler(device, varname, addressModeU, addressModeV, magFilter, minFilter, mipmapFilter, compare));
}

Sampler::~Sampler ()
{
  wgpuSamplerRelease(m_sampler);
}

const std::string& Sampler::GetVarName () const { return m_varname; }

WGPUBindGroupEntry Sampler::MakeEntry (uint32_t binding) const
{
  WGPUBindGroupEntry entry = {};
  entry.binding = binding;
  entry.sampler = m_sampler;
  return entry;
}
