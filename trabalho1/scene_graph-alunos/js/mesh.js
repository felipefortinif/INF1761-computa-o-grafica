// Loads an indexed triangle mesh from a text format ("V"/"N"/"T" lines
// for coords/normals/triangles; blank lines, "#" comments, and "--"
// section separators are ignored). Vertex buffers: slot 0 = coords, slot
// 1 = normals, plus a uint32 index buffer. Throws (with filename:line)
// on a malformed/unrecognized record, a vertex/normal count mismatch, or
// an out-of-range triangle index. Ported from python/mesh.py.

import { Shape } from "./shape.js";
import { createBufferWithData } from "./gpuutil.js";

export class Mesh extends Shape {
  // Parses an already-fetched mesh `text` (from `filename`, used only for
  // error messages). Prefer Mesh.load, which fetches `filename` first -
  // the browser has no synchronous file read.
  constructor (device, filename, text) {
    super();
    const coords = [];
    const normals = [];
    const indices = [];
    const lines = text.split("\n");
    for (let lineno = 1; lineno <= lines.length; lineno++) {
      const elems = lines[lineno - 1].trim().split(/\s+/).filter((e) => e.length > 0);
      if (elems.length === 0 || elems[0].startsWith("#") || elems[0].startsWith("--")) continue;
      const tag = elems[0];
      try {
        if (tag === "V") {
          if (elems.length !== 4) throw new Error(`'V' record needs 3 values, got ${elems.length - 1}`);
          coords.push(parseFloat(elems[1]), parseFloat(elems[2]), parseFloat(elems[3]));
        } else if (tag === "N") {
          if (elems.length !== 4) throw new Error(`'N' record needs 3 values, got ${elems.length - 1}`);
          normals.push(parseFloat(elems[1]), parseFloat(elems[2]), parseFloat(elems[3]));
        } else if (tag === "T") {
          if (elems.length !== 4) throw new Error(`'T' record needs 3 values, got ${elems.length - 1}`);
          indices.push(parseInt(elems[1], 10), parseInt(elems[2], 10), parseInt(elems[3], 10));
        } else {
          throw new Error(`unrecognized record type '${tag}'`);
        }
      } catch (e) {
        throw new Error(`${filename}:${lineno}: ${e.message}`);
      }
    }

    const nverts = coords.length / 3;
    if (normals.length / 3 !== nverts) {
      throw new Error(
        `${filename}: ${nverts} vertex coords but ${normals.length / 3} normals - ` +
        "a normal is required for every vertex (Mesh.draw always binds vertex slot 1 to it)"
      );
    }
    if (indices.length === 0) throw new Error(`${filename}: no triangles ('T' records) found`);
    const bad = indices.find((i) => i < 0 || i >= nverts);
    if (bad !== undefined) throw new Error(`${filename}: triangle index ${bad} out of range for ${nverts} vertices`);

    this.nind = indices.length;
    this.coordVbo = createBufferWithData(device, new Float32Array(coords), GPUBufferUsage.VERTEX);
    this.normalVbo = createBufferWithData(device, new Float32Array(normals), GPUBufferUsage.VERTEX);
    this.ibo = createBufferWithData(device, new Uint32Array(indices), GPUBufferUsage.INDEX);
  }

  // Fetches `filename` and constructs a Mesh from it.
  static async load (device, filename) {
    const response = await fetch(filename);
    if (!response.ok) throw new Error(`Failed to fetch ${filename}: ${response.status} ${response.statusText}`);
    const text = await response.text();
    return new Mesh(device, filename, text);
  }

  draw (st) {
    const firstInstance = st.getShader().commitMatrix(st);
    st.renderPass.setVertexBuffer(0, this.coordVbo);
    st.renderPass.setVertexBuffer(1, this.normalVbo);
    st.renderPass.setIndexBuffer(this.ibo, "uint32");
    st.renderPass.drawIndexed(this.nind, 1, 0, 0, firstInstance);
  }
}
