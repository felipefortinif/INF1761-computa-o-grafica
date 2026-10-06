// Unit cube (positions only, slot 0) whose vertex positions double as the
// cubemap sample direction. Pair it with a SkyBoxTransform on its Node -
// re-centering on the eye is that Transform's job, not draw()'s. Ported
// from python/skybox.py.
//
// Note: the original drawing disabled glDepthMask during the draw (the
// skybox never writes to the depth buffer, always staying behind
// everything). In WebGPU depthWriteEnabled is fixed per pipeline -
// configure the skybox's Pipeline with depthWriteEnabled: false if this
// behavior is needed.

import * as gm from "./graphicsmath.js";
import { Shape } from "./shape.js";
import { Transform } from "./transform.js";
import { createBufferWithData } from "./gpuutil.js";

export class SkyBox extends Shape {
  // Uploads the unit cube's 36 vertex positions (6 faces x 2 triangles)
  // to a single vertex buffer.
  constructor (device) {
    super();
    const coords = new Float32Array([
      -1.0, 1.0, -1.0,
      -1.0, -1.0, -1.0,
      1.0, -1.0, -1.0,
      1.0, -1.0, -1.0,
      1.0, 1.0, -1.0,
      -1.0, 1.0, -1.0,

      -1.0, -1.0, 1.0,
      -1.0, -1.0, -1.0,
      -1.0, 1.0, -1.0,
      -1.0, 1.0, -1.0,
      -1.0, 1.0, 1.0,
      -1.0, -1.0, 1.0,

      1.0, -1.0, -1.0,
      1.0, -1.0, 1.0,
      1.0, 1.0, 1.0,
      1.0, 1.0, 1.0,
      1.0, 1.0, -1.0,
      1.0, -1.0, -1.0,

      -1.0, -1.0, 1.0,
      -1.0, 1.0, 1.0,
      1.0, 1.0, 1.0,
      1.0, 1.0, 1.0,
      1.0, -1.0, 1.0,
      -1.0, -1.0, 1.0,

      -1.0, 1.0, -1.0,
      1.0, 1.0, -1.0,
      1.0, 1.0, 1.0,
      1.0, 1.0, 1.0,
      -1.0, 1.0, 1.0,
      -1.0, 1.0, -1.0,

      -1.0, -1.0, -1.0,
      -1.0, -1.0, 1.0,
      1.0, -1.0, -1.0,
      1.0, -1.0, -1.0,
      -1.0, -1.0, 1.0,
      1.0, -1.0, 1.0,
    ]);

    this.vbo = createBufferWithData(device, coords, GPUBufferUsage.VERTEX);
  }

  // Translates the cube to the camera's eye position and draws its 36
  // vertices. Relies on the current Pipeline having
  // depthWriteEnabled=false so it never occludes other geometry.
  // Like any other Shape: re-centering on the eye is SkyBoxTransform's job.
  draw (st) {
    const firstInstance = st.getShader().commitMatrix(st);
    st.renderPass.setVertexBuffer(0, this.vbo);
    st.renderPass.draw(36, 1, 0, firstInstance);
  }
}

// Replaces the accumulated model matrix with a translation to the camera's
// eye, so the skybox stays centered on the viewer and therefore looks
// infinitely far away.
//
// Unlike a plain Transform, which composes onto its ancestors' matrix, this
// one overrides it: it uses State.pushMatrixAbsolute. It is a
// Transform (loaded by Node.render before loadMatrices) precisely so that
// projection/vertex/normal are derived from the overridden matrix, with no
// re-entrant loadMatrices from inside a draw.
export class SkyBoxTransform extends Transform {
  // Pushes translate(eye) onto the "matrix" stack, discarding whatever the
  // ancestors accumulated. unload is inherited: it pops it back off.
  load (st) {
    const origin = gm.vec4(0, 0, 0, 1);
    const peyeH = gm.mat4MulVec4(st.getInverseViewMatrix(), origin);
    const peye = gm.vec3(peyeH[0], peyeH[1], peyeH[2]);
    st.pushMatrixAbsolute(gm.translate(gm.mat4(1), peye));
  }
}
