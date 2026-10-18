// Encoder/render-pass/submit mechanics for multi-pass techniques (shadow
// mapping, planar shadows, reflections) that don't fit Renderer's
// one-pass shape. Stateless - build once, reuse for the app's lifetime.
// Attachments come from a caller-built Framebuffer; pipeline/transform
// selection stays the app's job via Node. Ported from python/algorithm.py.
//
// Deviation from python: `renderPass` takes `size` as an explicit
// parameter instead of reading it off an attachment's view - unlike
// wgpu-py's GPUTextureView, the browser's native GPUTextureView exposes
// no width/height/size introspection at all (the same gap the C++ port
// hit against webgpu.h - see js_webgpu_port_progress memory).

import { State } from "./state.js";

export class Algorithm {
  constructor (device) {
    this.device = device;
  }

  // Encodes, runs and submits one full render pass into `framebuffer`
  // (whose attachments are all sized `size`, [width, height]), rendering
  // `scene` through `camera`. Self-contained - safe to call multiple
  // times per frame for multi-pass techniques.
  renderPass (framebuffer, scene, camera, size) {
    const encoder = this.device.createCommandEncoder();
    const passDesc = { colorAttachments: framebuffer.colorAttachments };
    // WebGPU's dictionary members reject an explicit null - "absent"
    // means the key must be omitted entirely, not set to null.
    if (framebuffer.depthStencilAttachment !== null) {
      passDesc.depthStencilAttachment = framebuffer.depthStencilAttachment;
    }
    const renderPass = encoder.beginRenderPass(passDesc);
    const st = new State(camera, this.device, renderPass, size);
    scene.render(st);
    // the traversal only packed rows on the CPU; one write per shader here,
    // before submit (writeBuffer is on the queue, draws are in the encoder)
    for (const shd of st.getMatrixShaders()) shd.flushMatrices();
    renderPass.end();
    this.device.queue.submit([encoder.finish()]);
  }
}
