// Straight-line (lerp) interpolation between two keyframe values p0 and
// p1. Ported from python/luxor/linearinterpolator.py.

import * as gm from "../graphicsmath.js";
import { Interpolator } from "./interpolator.js";

export class LinearInterpolator extends Interpolator {
  // Stores start p0 and end p1 keyframe values.
  constructor (p0, p1) {
    super();
    this.p0 = p0;
    this.p1 = p1;
  }

  // Linearly interpolates between p0 and p1 at t (0..1).
  interpolate (t) {
    return gm.vec3(
      (1.0 - t) * this.p0[0] + t * this.p1[0],
      (1.0 - t) * this.p0[1] + t * this.p1[1],
      (1.0 - t) * this.p0[2] + t * this.p1[2],
    );
  }
}
