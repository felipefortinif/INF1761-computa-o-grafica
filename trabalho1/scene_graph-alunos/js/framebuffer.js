// Named pair of ready-made attachment descriptors, in the exact shape
// encoder.beginRenderPass({colorAttachments, depthStencilAttachment})
// expects. Building the individual dicts (view, clearValue/loadOp, etc.)
// is always the caller's job - nothing is inferred here (see Algorithm).
// Ported from python/framebuffer.py.
//
// WebGPU has no framebuffer (FBO) handle object - a render pass
// references GPUTextureViews directly.

export class Framebuffer {
  constructor (colorAttachments, depthStencilAttachment = null) {
    this.colorAttachments = colorAttachments;
    this.depthStencilAttachment = depthStencilAttachment;
  }
}
