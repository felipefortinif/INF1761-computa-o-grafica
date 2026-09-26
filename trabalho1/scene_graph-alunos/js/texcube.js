// Cubemap texture bound to `varname`, loaded from a single cross-layout
// image file (4x3 grid: right,left,bottom,top,front,back) and sliced
// into the 6 layers of a rgba8unorm-srgb cube texture. A pure
// resource-holder - no load/unload of its own, see TextureSet. Ported
// from python/texcube.py.

// Standard cubemap layer order in WebGPU: +X,-X,+Y,-Y,+Z,-Z. The
// sub-images of the cross layout (right,left,bottom,top,front,back) map
// to these layers in this order:
const LAYER_FOR_CROP_INDEX = [0, 1, 3, 2, 4, 5]; // right,left,bottom,top,front,back -> layer

export class TexCube {
  // Fetches and decodes `url`'s cross layout, crops it into 6 sub-images
  // (via an offscreen canvas - JS has no PIL) and uploads each to its
  // corresponding cube layer.
  static async load (device, varname, url) {
    const blob = await (await fetch(url)).blob();
    const bitmap = await createImageBitmap(blob, { colorSpaceConversion: "none" });
    const width = bitmap.width;
    const height = bitmap.height;
    const w = Math.floor(width / 4);
    const h = Math.floor(height / 3);
    const x = [2 * w, 0, w, w, w, 3 * w];
    const y = [h, h, 2 * h, 0, h, h];

    const canvas = new OffscreenCanvas(width, height);
    const ctx = canvas.getContext("2d", { willReadFrequently: true });
    ctx.drawImage(bitmap, 0, 0);

    // "-srgb": same reasoning as in texture.js - the source image is
    // already sRGB-encoded, the GPU undoes the curve when sampling.
    const tex = device.createTexture({
      size: [w, h, 6], format: "rgba8unorm-srgb", dimension: "2d",
      usage: GPUTextureUsage.TEXTURE_BINDING | GPUTextureUsage.COPY_DST,
    });
    for (let i = 0; i < 6; i++) {
      const { data } = ctx.getImageData(x[i], y[i], w, h);
      const layer = LAYER_FOR_CROP_INDEX[i];
      device.queue.writeTexture(
        { texture: tex, origin: [0, 0, layer] }, data,
        { bytesPerRow: w * 4, rowsPerImage: h }, [w, h, 1],
      );
    }

    const texcube = Object.create(TexCube.prototype);
    texcube.varname = varname;
    texcube.tex = tex;
    texcube.view = tex.createView({ dimension: "cube" });
    return texcube;
  }

  getTexture () {
    return this.tex;
  }

  // The GPU resource TextureSet/Shader.addTextureSet binds - this
  // cubemap's view.
  get resource () {
    return this.view;
  }
}
