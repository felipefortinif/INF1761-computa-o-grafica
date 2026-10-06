// Orthographic 2D camera defined by a world-space view window
// (xmin/xmax/ymin/ymax). View matrix is always identity. Ported from
// python/camera2d.py.

import * as gm from "./graphicsmath.js";
import { Camera } from "./camera.js";

export class Camera2D extends Camera {
  // Stores the requested view window; defaults to the symmetric [-1,1] unit square.
  constructor (xmin = -1, xmax = 1, ymin = -1, ymax = 1) {
    super();
    this.xmin = xmin;
    this.xmax = xmax;
    this.ymin = ymin;
    this.ymax = ymax;
  }

  // Orthographic projection from the view window, expanding the shorter
  // axis to match the canvas aspect ratio. Recomputed every frame -
  // depends on canvasSize.
  getProjMatrix (canvasSize) {
    const [w, h] = canvasSize;
    if (w <= 0 || h <= 0) throw new Error(`getProjMatrix needs a canvasSize with both dimensions > 0, got ${canvasSize}`);
    const dx = this.xmax - this.xmin;
    const dy = this.ymax - this.ymin;
    let xmin, xmax, ymin, ymax;
    if (w / h > dx / dy) {
      const xc = (this.xmin + this.xmax) / 2;
      xmin = xc - dx / 2 * w / h;
      xmax = xc + dx / 2 * w / h;
      ymin = this.ymin;
      ymax = this.ymax;
    } else {
      const yc = (this.ymin + this.ymax) / 2;
      ymin = yc - dy / 2 * h / w;
      ymax = yc + dy / 2 * h / w;
      xmin = this.xmin;
      xmax = this.xmax;
    }
    // WebGPU's NDC z range is [0,1], not OpenGL's [-1,1]; see Camera3D.getProjMatrix.
    return gm.ortho(xmin, xmax, ymin, ymax, -1, 1);
  }

  // Always identity - 2D scenes are already expressed in the view window's coordinates.
  getViewMatrix () {
    return gm.mat4(1.0);
  }
}
