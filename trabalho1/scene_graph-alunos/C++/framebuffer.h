#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <wgpu.h>
#include <vector>
#include <optional>

// WebGPU has no framebuffer-object handle - a render pass references
// GPUTextureViews directly. This is just a named pair of ready-made
// attachment descriptors, in the exact shape
// wgpuCommandEncoderBeginRenderPass expects. Building the individual
// descriptors (view, clear_value/load_op, etc.) is always the caller's
// job - nothing is inferred here (see Algorithm).
struct Framebuffer {
  std::vector<WGPURenderPassColorAttachment> colorAttachments;
  std::optional<WGPURenderPassDepthStencilAttachment> depthStencilAttachment;
};

#endif
