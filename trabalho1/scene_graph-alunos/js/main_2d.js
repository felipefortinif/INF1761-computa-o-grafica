// Ported from python/main_2d.py: a 2D scene with a colored "face" quad
// and a rotating triangle "pointer", driven by a per-frame Engine.

import { Camera2D } from "./camera2d.js";
import { ColorMaterial } from "./colormaterial.js";
import { Transform } from "./transform.js";
import { Quad } from "./quad.js";
import { Triangle } from "./triangle.js";
import { Node } from "./node.js";
import { Shader } from "./shader.js";
import { Pipeline } from "./pipeline.js";
import { Scene } from "./scene.js";
import { Renderer } from "./renderer.js";
import { Engine } from "./engine.js";

let canvas, device, context, renderer, camera, scene;
let lastT = 0.0;

class MovePointer extends Engine {
  constructor (trf) {
    super();
    this.trf = trf;
  }

  update (dt) {
    this.trf.rotate(6 * dt, 0, 0, -1);
  }
}

async function initialize (device, targetFormat) {
  camera = new Camera2D(0, 10, 0, 10);

  const trf1 = new Transform();
  trf1.translate(3, 3, -0.5);
  trf1.scale(4, 4, 1);
  const faceMaterial = new ColorMaterial(1, 1, 1);
  const face = new Node({ trf: trf1, apps: [faceMaterial], shps: [new Quad(device)] });

  const trf2 = new Transform();
  trf2.translate(5, 5, 0);
  const trf3 = new Transform();
  trf3.scale(0.1, 2, 1);
  const pointerMaterial = new ColorMaterial(1, 0, 0);
  const pointer = new Node({
    trf: trf2,
    nodes: [new Node({ trf: trf3, apps: [pointerMaterial], shps: [new Triangle(device)] })],
  });

  const shader = await Shader.load(device, "../shaders/2d/shader.wgsl");
  shader.setVertexBuffers([
    { arrayStride: 2 * 4, stepMode: "vertex",
      attributes: [{ format: "float32x2", offset: 0, varName: "pos" }] },
  ]);
  const pipeline = new Pipeline(shader, targetFormat, { depthStencil: null });
  shader.addMaterial(faceMaterial);
  shader.addMaterial(pointerMaterial);

  const root = new Node({ pipeline, nodes: [face, pointer] });
  scene = new Scene(root);
  scene.addEngine(new MovePointer(trf2));
}

function update (dt) {
  scene.update(dt);
}

function draw () {
  const t = performance.now() / 1000;
  update(t - lastT);
  lastT = t;

  const targetTexture = context.getCurrentTexture();
  renderer.render(targetTexture, scene, camera);
  requestAnimationFrame(draw);
}

function onKey (event) {
  // "q" quitting a window doesn't map to a browser tab - left as a no-op.
}

async function main () {
  canvas = document.getElementById("canvas");
  const adapter = await navigator.gpu.requestAdapter();
  device = await adapter.requestDevice();
  context = canvas.getContext("webgpu");
  // Preferred format WITH "-srgb": the GPU encodes linear->sRGB
  // automatically on output - correct as long as the shader does its
  // lighting math in linear space.
  const targetFormat = navigator.gpu.getPreferredCanvasFormat();
  context.configure({ device, format: targetFormat });

  renderer = new Renderer(device, { clearValue: [0.8, 1.0, 1.0, 1.0] });

  await initialize(device, targetFormat);

  window.addEventListener("keydown", onKey);
  lastT = performance.now() / 1000;
  requestAnimationFrame(draw);
}

main();
