#include <memory>
class Pipeline;
using PipelinePtr = std::shared_ptr<Pipeline>;

#ifndef PIPELINE_H
#define PIPELINE_H

#include <wgpu.h>
#include <optional>
#include "shader.h"

class State;
using StatePtr = std::shared_ptr<State>;

// Distinguishes "use the default depth-stencil state" from an explicit
// override, from no depth-stencil attachment at all (mirrors the _UNSET
// sentinel vs. None distinction in pipeline.py).
enum class DepthStencilMode { UseDefault, Explicit, None };

struct PipelineDesc {
  std::optional<WGPUPrimitiveState> primitive;
  DepthStencilMode depthStencilMode = DepthStencilMode::UseDefault;
  WGPUDepthStencilState depthStencil = {};
  std::optional<WGPUMultisampleState> multisample;
  std::optional<WGPUBlendState> blend;
  WGPUColorWriteMask writeMask = WGPUColorWriteMask_All;
  std::optional<int> stencilReference;
};

// Rasterizer/target configuration: primitive, depth_stencil, and
// multisample state, plus the color target's format/blend/write_mask.
// Bound to exactly one Shader at construction time, which supplies the
// vertex buffer and bind group layouts. Builds the immutable
// GPURenderPipeline once, eagerly.
class Pipeline : public std::enable_shared_from_this<Pipeline> {
  ShaderPtr m_shader;
  WGPUDevice m_device;
  std::optional<int> m_stencilReference;
  WGPURenderPipeline m_gpuPipeline;

protected:
  Pipeline (ShaderPtr shader, WGPURenderPipeline gpuPipeline, std::optional<int> stencilReference);

public:
  static PipelinePtr Make (ShaderPtr shader, std::optional<WGPUTextureFormat> targetFormat,
                            const PipelineDesc& desc = {});
  ~Pipeline ();

  ShaderPtr GetShader () const;

  // Binds this Pipeline's GPURenderPipeline and its Shader's "matrix"
  // storage array on the render pass. Skips the rebind if this Pipeline
  // is already the one last bound.
  void Activate (StatePtr st);

  void Load (StatePtr st);
  void Unload (StatePtr st);
};

#endif
