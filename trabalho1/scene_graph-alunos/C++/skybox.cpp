#include "skybox.h"
#include "state.h"
#include "shader.h"

SkyBoxPtr SkyBox::Make (WGPUDevice device)
{
  return SkyBoxPtr(new SkyBox(device));
}

SkyBox::SkyBox (WGPUDevice device)
{
  static const float coords[] = {
    -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f,
  };

  WGPUBufferDescriptor desc = {};
  desc.size = sizeof(coords);
  desc.usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst;
  m_vbo = wgpuDeviceCreateBuffer(device, &desc);
  WGPUQueue queue = wgpuDeviceGetQueue(device);
  wgpuQueueWriteBuffer(queue, m_vbo, 0, coords, sizeof(coords));
  wgpuQueueRelease(queue);
}

SkyBox::~SkyBox ()
{
  wgpuBufferRelease(m_vbo);
}

void SkyBox::Draw (StatePtr st)
{
  // like any other Shape: re-centering on the eye is SkyBoxTransform's job
  uint32_t firstInstance = (uint32_t) st->GetShader()->CommitMatrix(st);

  WGPURenderPassEncoder pass = st->GetRenderPass();
  wgpuRenderPassEncoderSetVertexBuffer(pass, 0, m_vbo, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderDraw(pass, 36, 1, 0, firstInstance);
}

SkyBoxTransformPtr SkyBoxTransform::Make ()
{
  return SkyBoxTransformPtr(new SkyBoxTransform());
}

void SkyBoxTransform::Load (StatePtr st) const
{
  // overrides whatever the ancestors accumulated (Push, not PushMatrix);
  // Unload is inherited from Transform and pops it back off.
  Vec4 origin = {0, 0, 0, 1};
  Vec4 peye = Mat4MulVec4(st->GetInverseViewMatrix(), origin);
  st->PushMatrixAbsolute(Mat4Translate(Mat4Identity(), Vec3{peye.x, peye.y, peye.z}));
}
