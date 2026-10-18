// Single 2D triangle with vertices (-1,0), (1,0), (0,1). One vertex
// buffer bound at both slot 0 (position) and slot 1 (texcoord). Ported
// from python/triangle.py.

import { Shape } from "./shape.js";
import { createBufferWithData } from "./gpuutil.js";

export class Triangle extends Shape {
  constructor (device) {
    super();
    const coord = new Float32Array([-1, 0, 1, 0, 0, 1]);
    this.vbo = createBufferWithData(device, coord, GPUBufferUsage.VERTEX);
  }

  // Binds the buffer to both slots and issues an unindexed draw(3).
  draw (st) {
    const firstInstance = st.getShader().commitMatrix(st);
    st.renderPass.setVertexBuffer(0, this.vbo); // position
    st.renderPass.setVertexBuffer(1, this.vbo); // same data serves as texcoord
    st.renderPass.draw(3, 1, 0, firstInstance);
  }
}
