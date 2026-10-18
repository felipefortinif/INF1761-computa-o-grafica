// Ported from python/main_3d.py: a lit cube/sphere plus a textured
// decal quad over a floor, with an arcball-controlled camera.

import * as gm from "./graphicsmath.js";
import { Camera3D } from "./camera3d.js";
import { PointLight } from "./pointlight.js";
import { Shader } from "./shader.js";
import { Pipeline } from "./pipeline.js";
import { PhongMaterial } from "./phongmaterial.js";
import { Texture } from "./texture.js";
import { Sampler } from "./sampler.js";
import { TextureSet } from "./textureset.js";
import { Transform } from "./transform.js";
import { Node } from "./node.js";
import { Scene } from "./scene.js";
import { Renderer } from "./renderer.js";
import { Cube } from "./cube.js";
import { Sphere } from "./sphere.js";
import { Quad } from "./quad.js";

let canvas, device, context, renderer, camera, scene;

const viewerPos = gm.vec3(2.0, 3.5, 4.0);

async function initialize (device, targetFormat) {
  camera = new Camera3D(viewerPos[0], viewerPos[1], viewerPos[2]);
  const arcball = camera.createArcball();
  arcball.attach(canvas);

  const light = new PointLight(0.0, 0.0, 0.0, "camera");

  const white = new PhongMaterial(1.0, 1.0, 1.0);
  const red = new PhongMaterial(1.0, 0.5, 0.5);
  const paper = await Texture.load(device, "decal_texture", "../images/paper.jpg");
  const paperSampler = new Sampler(device, "decal_sampler");

  const trf1 = new Transform();
  trf1.scale(3.0, 0.3, 3.0);
  trf1.translate(0.0, -1.0, 0.0);
  const trf2 = new Transform();
  trf2.scale(0.5, 0.5, 0.5);
  trf2.translate(0.0, 1.0, 0.0);
  const trf3 = new Transform();
  trf3.translate(0.8, 0.0, 0.8);
  trf3.rotate(30.0, 0.0, 1.0, 0.0);
  trf3.rotate(90.0, -1.0, 0.0, 0.0);
  trf3.scale(0.5, 0.7, 1.0);

  const cube = new Cube(device);
  const quad = new Quad(device);
  const sphere = new Sphere(device);

  const shader = await Shader.load(device, "../shaders/ilum_frag/lit.wgsl", { light, space: "world" });
  shader.setVertexBuffers([
    { arrayStride: 3 * 4, stepMode: "vertex",
      attributes: [{ format: "float32x3", offset: 0, varName: "coord" }] },
    { arrayStride: 3 * 4, stepMode: "vertex",
      attributes: [{ format: "float32x3", offset: 0, varName: "normal" }] },
  ]);
  const pipeline = new Pipeline(shader, targetFormat, { primitive: { cullMode: "back" } });

  // depthBias/depthBiasSlopeScale replace the old dynamic PolygonOffset
  // (avoids z-fighting between the paper decal and the "floor" right below it)
  const shdTex = await Shader.load(device, "../shaders/ilum_frag/textured.wgsl", { light, space: "world" });
  shdTex.setVertexBuffers([
    { arrayStride: 2 * 4, stepMode: "vertex",
      attributes: [{ format: "float32x2", offset: 0, varName: "coord" }] },
    { arrayStride: 2 * 4, stepMode: "vertex",
      attributes: [{ format: "float32x2", offset: 0, varName: "texcoord" }] },
  ]);
  const pipelineTex = new Pipeline(shdTex, targetFormat, {
    primitive: { cullMode: "none" },
    depthStencil: { depthBias: -1, depthBiasSlopeScale: -1 },
  });

  // white is used under both shaders (the plain sphere and the
  // textured quad), so it's registered on each independently.
  shader.addMaterial(white);
  shader.addMaterial(red);
  shdTex.addMaterial(white);
  const paperTextures = new TextureSet([paper, paperSampler]);
  shdTex.addTextureSet(paperTextures);

  const root = new Node({
    pipeline,
    nodes: [
      new Node({ trf: trf1, apps: [red], shps: [cube] }),
      new Node({ pipeline: pipelineTex, trf: trf3, apps: [white, paperTextures], shps: [quad] }),
      new Node({ trf: trf2, apps: [white], shps: [sphere] }),
    ],
  });
  scene = new Scene(root);
}

function draw () {
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

  renderer = new Renderer(device, { depthTest: true, clearValue: [1.0, 1.0, 1.0, 1.0] });

  await initialize(device, targetFormat);

  window.addEventListener("keydown", onKey);
  requestAnimationFrame(draw);
}

main();
