#include <GLFW/glfw3.h>

#include "wgpucontext.h"
#include "arcball.h"
#include "scene.h"
#include "state.h"
#include "camera3d.h"
#include "phongmaterial.h"
#include "pointlight.h"
#include "texture.h"
#include "sampler.h"
#include "textureset.h"
#include "transform.h"
#include "node.h"
#include "cube.h"
#include "quad.h"
#include "sphere.h"
#include "shader.h"
#include "pipeline.h"
#include "renderer.h"
#include "error.h"

#include <cassert>
#include <cstdio>

static float viewer_pos[3] = {2.0f, 3.5f, 4.0f};

static WgpuContextPtr ctx;
static RendererPtr renderer;
static ScenePtr scene;
static Camera3DPtr camera;
static ArcballPtr arcball;

static void initialize (void)
{
  camera = Camera3D::Make(viewer_pos[0], viewer_pos[1], viewer_pos[2]);
  arcball = camera->CreateArcball();

  PointLightPtr light = PointLight::Make(0.0f, 0.0f, 0.0f, "camera");

  PhongMaterialPtr white = PhongMaterial::Make(1.0f, 1.0f, 1.0f);
  PhongMaterialPtr red = PhongMaterial::Make(1.0f, 0.5f, 0.5f);
  TexturePtr paper = Texture::Make(ctx->GetDevice(), "decal_texture",
                                    "../images/paper.jpg");
  SamplerPtr paperSampler = Sampler::Make(ctx->GetDevice(), "decal_sampler");

  TransformPtr trf1 = Transform::Make();
  trf1->Scale(3.0f, 0.3f, 3.0f);
  trf1->Translate(0.0f, -1.0f, 0.0f);
  TransformPtr trf2 = Transform::Make();
  trf2->Scale(0.5f, 0.5f, 0.5f);
  trf2->Translate(0.0f, 1.0f, 0.0f);
  TransformPtr trf3 = Transform::Make();
  trf3->Translate(0.8f, 0.0f, 0.8f);
  trf3->Rotate(30.0f, 0.0f, 1.0f, 0.0f);
  trf3->Rotate(90.0f, -1.0f, 0.0f, 0.0f);
  trf3->Scale(0.5f, 0.7f, 1.0f);

  CubePtr cube = Cube::Make(ctx->GetDevice());
  QuadPtr quad = Quad::Make(ctx->GetDevice());
  SpherePtr sphere = Sphere::Make(ctx->GetDevice());

  ShaderPtr shader = Shader::Make(ctx->GetDevice(), "../shaders/ilum_frag/lit.wgsl", light, "world");
  VertexBufferSpec coordBuf, normalBuf;
  coordBuf.arrayStride = 3 * 4;
  coordBuf.attributes.push_back({std::string("coord"), std::nullopt, WGPUVertexFormat_Float32x3, 0});
  normalBuf.arrayStride = 3 * 4;
  normalBuf.attributes.push_back({std::string("normal"), std::nullopt, WGPUVertexFormat_Float32x3, 0});
  shader->SetVertexBuffers({coordBuf, normalBuf});
  PipelineDesc pdesc;
  WGPUPrimitiveState primitive = {};
  primitive.cullMode = WGPUCullMode_Back;
  pdesc.primitive = primitive;
  PipelinePtr pipeline = Pipeline::Make(shader, ctx->GetSurfaceFormat(), pdesc);

  // depth_bias/depth_bias_slope_scale replace the old dynamic PolygonOffset
  // (avoids z-fighting between the paper decal and the "floor" right below it)
  ShaderPtr shdTex = Shader::Make(ctx->GetDevice(), "../shaders/ilum_frag/textured.wgsl", light, "world");
  VertexBufferSpec texCoordBuf, texcoordBuf;
  texCoordBuf.arrayStride = 2 * 4;
  texCoordBuf.attributes.push_back({std::string("coord"), std::nullopt, WGPUVertexFormat_Float32x2, 0});
  texcoordBuf.arrayStride = 2 * 4;
  texcoordBuf.attributes.push_back({std::string("texcoord"), std::nullopt, WGPUVertexFormat_Float32x2, 0});
  shdTex->SetVertexBuffers({texCoordBuf, texcoordBuf});
  PipelineDesc pdescTex;
  WGPUPrimitiveState primitiveTex = {};
  primitiveTex.cullMode = WGPUCullMode_None;
  pdescTex.primitive = primitiveTex;
  pdescTex.depthStencilMode = DepthStencilMode::Explicit;
  WGPUDepthStencilState dsTex = {};
  dsTex.format = WGPUTextureFormat_Depth24Plus;
  dsTex.depthWriteEnabled = WGPUOptionalBool_True;
  dsTex.depthCompare = WGPUCompareFunction_Less;
  dsTex.depthBias = -1;
  dsTex.depthBiasSlopeScale = -1.0f;
  dsTex.stencilReadMask = 0xFFFFFFFF;
  dsTex.stencilWriteMask = 0xFFFFFFFF;
  pdescTex.depthStencil = dsTex;
  PipelinePtr pipelineTex = Pipeline::Make(shdTex, ctx->GetSurfaceFormat(), pdescTex);

  // white is used under both shaders (the plain sphere and the
  // textured quad), so it's registered on each independently.
  shader->AddMaterial(white.get());
  shader->AddMaterial(red.get());
  shdTex->AddMaterial(white.get());
  TextureSetPtr paperTextures = TextureSet::Make({paper.get(), paperSampler.get()});
  shdTex->AddTextureSet(paperTextures.get());

  // build scene
  NodePtr root = Node::Make(pipeline, {
    Node::Make(trf1, {red}, {cube}),
    Node::Make(pipelineTex, trf3, {white, paperTextures}, {quad}),
    Node::Make(trf2, {white}, {sphere}),
  });
  scene = Scene::Make(root);
}

static void display ()
{
  WGPUSurfaceTexture surfaceTex = ctx->GetCurrentTexture();
  if (surfaceTex.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
      surfaceTex.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal)
    return;
  renderer->Render(surfaceTex.texture, scene, camera);
  ctx->Present();
  wgpuTextureRelease(surfaceTex.texture);
}

static void error (int code, const char* msg)
{
  printf("GLFW error %d: %s\n", code, msg);
  glfwTerminate();
  exit(0);
}

static void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
  (void) scancode; (void) mods;
  if (key == GLFW_KEY_Q && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static void resize (GLFWwindow* win, int width, int height)
{
  (void) win;
  if (width > 0 && height > 0) ctx->Configure(width, height);
}

static void cursorpos (GLFWwindow* win, double x, double y)
{
  // convert screen pos (upside down) to framebuffer pos (e.g., retina displays)
  int wn_w, wn_h, fb_w, fb_h;
  glfwGetWindowSize(win, &wn_w, &wn_h);
  glfwGetFramebufferSize(win, &fb_w, &fb_h);
  x = x * fb_w / wn_w;
  y = (wn_h - y) * fb_h / wn_h;
  arcball->AccumulateMouseMotion((float) x, (float) y, (float) fb_w, (float) fb_h);
}
static void cursorinit (GLFWwindow* win, double x, double y)
{
  int wn_w, wn_h, fb_w, fb_h;
  glfwGetWindowSize(win, &wn_w, &wn_h);
  glfwGetFramebufferSize(win, &fb_w, &fb_h);
  x = x * fb_w / wn_w;
  y = (wn_h - y) * fb_h / wn_h;
  arcball->InitMouseMotion((float) x, (float) y);
  glfwSetCursorPosCallback(win, cursorpos);
}
static void mousebutton (GLFWwindow* win, int button, int action, int mods)
{
  (void) button; (void) mods;
  if (action == GLFW_PRESS)
    glfwSetCursorPosCallback(win, cursorinit);
  else // GLFW_RELEASE
    glfwSetCursorPosCallback(win, nullptr);
}

int main ()
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#ifdef __APPLE__
  glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
#endif

  glfwSetErrorCallback(error);

  GLFWwindow* win = glfwCreateWindow(600, 400, "3D scene", nullptr, nullptr);
  assert(win);
  glfwSetFramebufferSizeCallback(win, resize);
  glfwSetKeyCallback(win, keyboard);
  glfwSetMouseButtonCallback(win, mousebutton);

  int fbw, fbh;
  glfwGetFramebufferSize(win, &fbw, &fbh);
  ctx = WgpuContext::Make(win, fbw, fbh);
  renderer = Renderer::Make(ctx->GetDevice(), true, {1.0, 1.0, 1.0, 1.0});

  initialize();

  while (!glfwWindowShouldClose(win)) {
    display();
    glfwPollEvents();
  }
  ctx = nullptr;
  glfwDestroyWindow(win);
  glfwTerminate();
  return 0;
}
