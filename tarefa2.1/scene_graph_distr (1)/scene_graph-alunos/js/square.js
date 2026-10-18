// Flat quad spanning [-1,1] in x and y (z=0), with separate coord and
// texcoord buffers bound at slots 0/1. Ported from python/square.py.

import { Shape } from "./shape.js";
import { createBufferWithData } from "./gpuutil.js";

export class Square extends Shape {
  constructor (device) {
    super();
    const coord = new Float32Array([-1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, 1.0]);
    // t grows downward: the bottom vertex gets t = 1
    const texcoord = new Float32Array([0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0]);
    const index = new Uint32Array([0, 1, 2, 0, 2, 3]);
    this.coordVbo = createBufferWithData(device, coord, GPUBufferUsage.VERTEX);
    this.texcoordVbo = createBufferWithData(device, texcoord, GPUBufferUsage.VERTEX);
    this.ibo = createBufferWithData(device, index, GPUBufferUsage.INDEX);
  }

  draw (st) {
    const firstInstance = st.getShader().commitMatrix(st);
    st.renderPass.setVertexBuffer(0, this.coordVbo);
    st.renderPass.setVertexBuffer(1, this.texcoordVbo);
    st.renderPass.setIndexBuffer(this.ibo, "uint32");
    st.renderPass.drawIndexed(6, 1, 0, 0, firstInstance);
  }
}
