#include "pipeline.h"
#include "state.h"
#include "error.h"

static WGPUDepthStencilState DefaultDepthStencil ()
{
  WGPUDepthStencilState d = {};
  d.format = WGPUTextureFormat_Depth24Plus;
  d.depthWriteEnabled = WGPUOptionalBool_True;
  d.depthCompare = WGPUCompareFunction_Less;
  d.depthBias = 0;
  d.depthBiasSlopeScale = 0.0f;
  d.depthBiasClamp = 0.0f;
  d.stencilReadMask = 0xFFFFFFFF;
  d.stencilWriteMask = 0xFFFFFFFF;
  return d;
}

static WGPUMultisampleState DefaultMultisample ()
{
  WGPUMultisampleState m = {};
  m.count = 1;
  m.mask = 0xFFFFFFFF;
  m.alphaToCoverageEnabled = false;
  return m;
}

Pipeline::Pipeline (ShaderPtr shader, WGPURenderPipeline gpuPipeline, std::optional<int> stencilReference)
  : m_shader(shader), m_device(shader->GetDevice()), m_stencilReference(stencilReference), m_gpuPipeline(gpuPipeline)
{
}

PipelinePtr Pipeline::Make (ShaderPtr shader, std::optional<WGPUTextureFormat> targetFormat, const PipelineDesc& desc)
{
  WGPUPrimitiveState primitive = desc.primitive.value_or(WGPUPrimitiveState{});

  std::optional<WGPUDepthStencilState> depthStencil;
  if (desc.depthStencilMode == DepthStencilMode::UseDefault) depthStencil = DefaultDepthStencil();
  else if (desc.depthStencilMode == DepthStencilMode::Explicit) depthStencil = desc.depthStencil;

  if (!targetFormat && !depthStencil)
    Error::Fatal("Pipeline needs targetFormat or depthStencil (or both) - neither given");

  WGPUMultisampleState multisample = desc.multisample.value_or(DefaultMultisample());

  WGPUDevice device = shader->GetDevice();

  WGPUFragmentState fragment = {};
  WGPUColorTargetState colorTarget = {};
  bool hasFragment = targetFormat.has_value();
  if (hasFragment) {
    colorTarget.format = *targetFormat;
    colorTarget.blend = desc.blend ? &*desc.blend : nullptr;
    colorTarget.writeMask = desc.writeMask;
    fragment.module = shader->GetModule();
    fragment.entryPoint = {"fs_main", WGPU_STRLEN};
    fragment.targetCount = 1;
    fragment.targets = &colorTarget;
  }

  const std::vector<WGPUVertexBufferLayout>& vertexBuffers = shader->GetVertexBufferLayout();
  std::vector<WGPUBindGroupLayout> bindGroupLayouts = shader->GetBindGroupLayouts();

  WGPUPipelineLayoutDescriptor layoutDesc = {};
  layoutDesc.bindGroupLayoutCount = bindGroupLayouts.size();
  layoutDesc.bindGroupLayouts = bindGroupLayouts.data();
  WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &layoutDesc);

  WGPURenderPipelineDescriptor pipelineDesc = {};
  pipelineDesc.layout = pipelineLayout;
  pipelineDesc.vertex.module = shader->GetModule();
  pipelineDesc.vertex.entryPoint = {"vs_main", WGPU_STRLEN};
  pipelineDesc.vertex.bufferCount = vertexBuffers.size();
  pipelineDesc.vertex.buffers = vertexBuffers.data();
  pipelineDesc.primitive = primitive;
  pipelineDesc.depthStencil = depthStencil ? &*depthStencil : nullptr;
  pipelineDesc.multisample = multisample;
  pipelineDesc.fragment = hasFragment ? &fragment : nullptr;

  WGPURenderPipeline gpuPipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);
  wgpuPipelineLayoutRelease(pipelineLayout);

  return PipelinePtr(new Pipeline(shader, gpuPipeline, desc.stencilReference));
}

Pipeline::~Pipeline ()
{
  wgpuRenderPipelineRelease(m_gpuPipeline);
}

ShaderPtr Pipeline::GetShader () const { return m_shader; }

void Pipeline::Activate (StatePtr st)
{
  if (st->GetActivePipeline().get() == this) return;
  wgpuRenderPassEncoderSetPipeline(st->GetRenderPass(), m_gpuPipeline);
  wgpuRenderPassEncoderSetBindGroup(st->GetRenderPass(), (uint32_t) m_shader->GetMatrixGroupIndex(),
                                     m_shader->GetMatrixBindGroup(), 0, nullptr);
  st->SetActivePipeline(shared_from_this());
}

void Pipeline::Load (StatePtr st)
{
  st->PushPipeline(shared_from_this());
  Activate(st);
  m_shader->CommitGlobal(st);
  wgpuRenderPassEncoderSetStencilReference(st->GetRenderPass(), (uint32_t) m_stencilReference.value_or(0));
}

void Pipeline::Unload (StatePtr st)
{
  st->PopPipeline();
  PipelinePtr top = st->TryGetPipeline();
  if (top) top->Activate(st);
  m_shader->UnbindGlobal(st);
  if (top) wgpuRenderPassEncoderSetStencilReference(st->GetRenderPass(), (uint32_t) top->m_stencilReference.value_or(0));
}
