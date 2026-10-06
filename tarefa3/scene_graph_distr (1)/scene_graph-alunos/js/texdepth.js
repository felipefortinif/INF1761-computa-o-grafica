// Depth texture (depth32float) bound to `varname`, usable both as a
// render pass's depth-stencil attachment and as a sampled texture (e.g.
// shadow mapping). Shaders must declare it as texture_depth_2d, not
// texture_2d<f32>. The sampler is a separate object. A pure
// resource-holder - no load/unload of its own, see TextureSet. Ported
// from python/texdepth.py.

export class TexDepth {
  // Allocates a depth32float texture of size width x height and its view.
  constructor (device, varname, width, height) {
    this.varname = varname;
    this.width = width;
    this.height = height;
    this.tex = device.createTexture({
      size: [width, height, 1], format: "depth32float",
      usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.TEXTURE_BINDING,
    });
    this.view = this.tex.createView();
  }

  // Returns the raw texture, for a depth-stencil attachment. Use
  // `resource` (the same view) for sampling.
  getTexture () {
    return this.tex;
  }

  // The GPU resource TextureSet/Shader.addTextureSet binds - this depth
  // texture's view.
  get resource () {
    return this.view;
  }
}
