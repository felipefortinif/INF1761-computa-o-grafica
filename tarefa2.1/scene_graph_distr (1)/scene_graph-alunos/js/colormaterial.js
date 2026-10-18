// Flat, unlit color appearance: an RGB color plus opacity, written into
// the shader's "material" group. Ported from python/colormaterial.py.

import * as gm from "./graphicsmath.js";
import { Material } from "./material.js";

export class ColorMaterial extends Material {
  constructor (r, g, b, opacity = 1) {
    super({ color: gm.vec3(r, g, b), opacity });
  }

  setColor (r, g, b) {
    this.set("color", gm.vec3(r, g, b));
  }

  setOpacity (opacity) {
    this.set("opacity", opacity);
  }
}
