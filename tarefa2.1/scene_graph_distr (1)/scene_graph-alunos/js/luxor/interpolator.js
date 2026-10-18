// Base class for an animation curve between two keyframe values.
// Subclasses store their keyframe data and implement interpolate.
// Ported from python/luxor/interpolator.py.

export class Interpolator {
  // Maps t in [0,1] to the interpolated Vec3 value. Subclasses override.
  interpolate (t) {
    throw new Error("not implemented");
  }
}
