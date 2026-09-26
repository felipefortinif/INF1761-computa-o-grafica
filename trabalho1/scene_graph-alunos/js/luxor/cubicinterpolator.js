// Cubic Hermite spline interpolation between two keyframes. p0/p1 are
// endpoint values at t=0/1; m0/m1 are their tangent vectors. Ported from
// python/luxor/cubicinterpolator.py.

import * as gm from "../graphicsmath.js";
import { Interpolator } from "./interpolator.js";

export class CubicInterpolator extends Interpolator {
  // Stores endpoints p0/p1 and tangents m0/m1.
  constructor (p0, m0, p1, m1) {
    super();
    this.p0 = p0;
    this.m0 = m0;
    this.p1 = p1;
    this.m1 = m1;
  }

  // Evaluates the Hermite basis functions at t (0..1).
  interpolate (t) {
    const t2 = t * t;
    const t3 = t * t2;
    const a = 2 * t3 - 3 * t2 + 1;
    const b = t3 - 2 * t2 + t;
    const c = -2 * t3 + 3 * t2;
    const d = t3 - t2;
    return gm.vec3(
      a * this.p0[0] + b * this.m0[0] + c * this.p1[0] + d * this.m1[0],
      a * this.p0[1] + b * this.m0[1] + c * this.p1[1] + d * this.m1[1],
      a * this.p0[2] + b * this.m0[2] + c * this.p1[2] + d * this.m1[2],
    );
  }
}
