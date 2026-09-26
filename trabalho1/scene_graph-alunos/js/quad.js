// Flat grid spanning [0,1]x[0,1], subdivided into nx by ny cells via
// Grid. Coord/texcoord buffers bound at slots 0/1 - two buffers, and not
// the same one twice, because t runs opposite to y. Ported from
// python/quad.py.

import { Shape } from "./shape.js";
import { Grid } from "./grid.js";
import { createBufferWithData } from "./gpuutil.js";

export class Quad extends Shape {
  constructor (device, nx = 1, ny = 1) {
    super();
    const grid = new Grid(nx, ny);
    this.nind = grid.indexCount();
    this.coordVbo = createBufferWithData(device, grid.getCoords(), GPUBufferUsage.VERTEX);
    this.texcoordVbo = createBufferWithData(device, grid.getTexcoords(), GPUBufferUsage.VERTEX);
    this.ibo = createBufferWithData(device, grid.getIndices(), GPUBufferUsage.INDEX);
  }

  draw (st) {
    const firstInstance = st.getShader().commitMatrix(st);
    st.renderPass.setVertexBuffer(0, this.coordVbo);
    st.renderPass.setVertexBuffer(1, this.texcoordVbo);
    st.renderPass.setIndexBuffer(this.ibo, "uint32");
    st.renderPass.drawIndexed(this.nind, 1, 0, 0, firstInstance);
  }
}
