#include "texcube.h"
#include "error.h"
#include "stb_image.h"
#include <vector>
#include <cstring>

// Standard cubemap layer order in WebGPU: +X,-X,+Y,-Y,+Z,-Z. The
// sub-images of the cross layout (right,left,bottom,top,front,back) map
// to these layers in this order:
static const int kLayerForCropIndex[6] = {0, 1, 3, 2, 4, 5};

TexCube::TexCube (WGPUDevice device, const std::string& varname, const std::string& filename)
  : m_varname(varname)
{
  int width, height, channels;
  unsigned char* pixels = stbi_load(filename.c_str(), &width, &height, &channels, 4);
  if (!pixels) Error::Fatal("could not load image: " + filename);

  int w = width / 4;
  int h = height / 3;
  int cropX[6] = {2 * w, 0, w, w, w, 3 * w};
  int cropY[6] = {h, h, 2 * h, 0, h, h};

  WGPUTextureDescriptor texDesc = {};
  texDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
  texDesc.dimension = WGPUTextureDimension_2D;
  texDesc.size = {(uint32_t) w, (uint32_t) h, 6};
  texDesc.format = WGPUTextureFormat_RGBA8UnormSrgb;
  texDesc.mipLevelCount = 1;
  texDesc.sampleCount = 1;
  m_tex = wgpuDeviceCreateTexture(device, &texDesc);

  WGPUQueue queue = wgpuDeviceGetQueue(device);
  std::vector<uint8_t> layerBuf((size_t) w * h * 4);
  for (int i = 0; i < 6; i++) {
    for (int row = 0; row < h; row++) {
      const unsigned char* src = pixels + ((size_t) (cropY[i] + row) * width + cropX[i]) * 4;
      memcpy(layerBuf.data() + (size_t) row * w * 4, src, (size_t) w * 4);
    }
    WGPUTexelCopyTextureInfo dst = {};
    dst.texture = m_tex;
    dst.origin = {0, 0, (uint32_t) kLayerForCropIndex[i]};
    WGPUTexelCopyBufferLayout layout = {};
    layout.bytesPerRow = (uint32_t) w * 4;
    layout.rowsPerImage = (uint32_t) h;
    WGPUExtent3D writeSize = {(uint32_t) w, (uint32_t) h, 1};
    wgpuQueueWriteTexture(queue, &dst, layerBuf.data(), layerBuf.size(), &layout, &writeSize);
  }
  wgpuQueueRelease(queue);
  stbi_image_free(pixels);

  WGPUTextureViewDescriptor viewDesc = {};
  viewDesc.format = WGPUTextureFormat_RGBA8UnormSrgb;
  viewDesc.dimension = WGPUTextureViewDimension_Cube;
  viewDesc.baseMipLevel = 0;
  viewDesc.mipLevelCount = 1;
  viewDesc.baseArrayLayer = 0;
  viewDesc.arrayLayerCount = 6;
  viewDesc.aspect = WGPUTextureAspect_All;
  m_view = wgpuTextureCreateView(m_tex, &viewDesc);
}

TexCubePtr TexCube::Make (WGPUDevice device, const std::string& varname, const std::string& filename)
{
  return TexCubePtr(new TexCube(device, varname, filename));
}

TexCube::~TexCube ()
{
  wgpuTextureViewRelease(m_view);
  wgpuTextureRelease(m_tex);
}

WGPUTexture TexCube::GetTexture () const { return m_tex; }
const std::string& TexCube::GetVarName () const { return m_varname; }

WGPUBindGroupEntry TexCube::MakeEntry (uint32_t binding) const
{
  WGPUBindGroupEntry entry = {};
  entry.binding = binding;
  entry.textureView = m_view;
  return entry;
}
