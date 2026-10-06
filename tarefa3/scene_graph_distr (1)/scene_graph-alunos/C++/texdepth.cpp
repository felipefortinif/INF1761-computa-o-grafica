#include "texdepth.h"

TexDepth::TexDepth (WGPUDevice device, const std::string& varname, int width, int height)
  : m_varname(varname), m_width(width), m_height(height)
{
  WGPUTextureDescriptor desc = {};
  desc.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding;
  desc.dimension = WGPUTextureDimension_2D;
  desc.size = {(uint32_t) width, (uint32_t) height, 1};
  desc.format = WGPUTextureFormat_Depth32Float;
  desc.mipLevelCount = 1;
  desc.sampleCount = 1;
  m_tex = wgpuDeviceCreateTexture(device, &desc);
  m_view = wgpuTextureCreateView(m_tex, nullptr);
}

TexDepthPtr TexDepth::Make (WGPUDevice device, const std::string& varname, int width, int height)
{
  return TexDepthPtr(new TexDepth(device, varname, width, height));
}

TexDepth::~TexDepth ()
{
  wgpuTextureViewRelease(m_view);
  wgpuTextureRelease(m_tex);
}

WGPUTexture TexDepth::GetTexture () const { return m_tex; }
WGPUTextureView TexDepth::GetView () const { return m_view; }
int TexDepth::GetWidth () const { return m_width; }
int TexDepth::GetHeight () const { return m_height; }
const std::string& TexDepth::GetVarName () const { return m_varname; }

WGPUBindGroupEntry TexDepth::MakeEntry (uint32_t binding) const
{
  WGPUBindGroupEntry entry = {};
  entry.binding = binding;
  entry.textureView = m_view;
  return entry;
}
