// Directional light whose direction ignores translation and is
// normalized. Ported from python/directionallight.py.

import * as gm from "./graphicsmath.js";
import { Light } from "./light.js";

export class DirectionalLight extends Light {
  constructor (x, y, z, space = "world", values = {}) {
    super(space, values);
    this.setDirection(x, y, z);
  }

  setDirection (x, y, z) {
    this.set("light_direction", gm.vec3(x, y, z));
  }

  map (matrix) {
    const d = this.get("light_direction");
    const direction = gm.mat4MulVec4(matrix, gm.vec4(d[0], d[1], d[2], 0.0));
    return { light_direction: gm.normalize(gm.vec3(direction[0], direction[1], direction[2])) };
  }
}
