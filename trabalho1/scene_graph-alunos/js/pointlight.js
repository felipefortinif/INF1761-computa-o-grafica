// Point light whose position is translated by coordinate mappings.
// Ported from python/pointlight.py.

import * as gm from "./graphicsmath.js";
import { Light } from "./light.js";

export class PointLight extends Light {
  constructor (x, y, z, space = "world", values = {}) {
    super(space, values);
    this.setPosition(x, y, z);
  }

  setPosition (x, y, z) {
    this.set("light_position", gm.vec4(x, y, z, 1.0));
  }

  map (matrix) {
    const position = this.get("light_position");
    return { light_position: gm.mat4MulVec4(matrix, position) };
  }
}
