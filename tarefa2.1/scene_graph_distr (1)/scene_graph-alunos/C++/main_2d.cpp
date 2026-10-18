#include <GLFW/glfw3.h>

#include "wgpucontext.h"
#include "scene.h"
#include "state.h"
#include "camera2d.h"
#include "colormaterial.h"
#include "transform.h"
#include "quad.h"
#include "triangle.h"
#include "node.h"
#include "shader.h"
#include "pipeline.h"
#include "renderer.h"
#include "engine.h"
#include "error.h"

#include <cassert>
#include <cstdio>

static WgpuContextPtr ctx;
static RendererPtr renderer;
static ScenePtr scene;
static Camera2DPtr camera;

class MovePointer;
using MovePointerPtr = std::shared_ptr<MovePointer>;
class MovePointer : public Engine
{
  TransformPtr m_trf;
protected:
  MovePointer (TransformPtr trf)
  : m_trf(trf)
  {
  }
public:
  static MovePointerPtr Make (TransformPtr trf)
  {
    return MovePointerPtr(new MovePointer(trf));
  }
  virtual void Update (float dt)
  {
    m_trf->Rotate(6.0f * dt, 0.0f, 0.0f, -1.0f);
  }
};

static void initialize (void)
{
  camera = Camera2D::Make(0, 10, 0, 10);

  TransformPtr trf1 = Transform::Make();
  trf1->Translate(3.0f, 3.0f, -0.5f);
  trf1->Scale(4.0f, 4.0f, 1.0f);
  ColorMaterialPtr faceMaterial = ColorMaterial::Make(1.0f, 1.0f, 1.0f);
  NodePtr face = Node::Make(trf1, {faceMaterial}, {Quad::Make(ctx->GetDevice())});

  TransformPtr trf2 = Transform::Make();
  trf2->Translate(5.0f, 5.0f, 0.0f);
  TransformPtr trf3 = Transform::Make();
  trf3->Scale(0.1f, 2.0f, 1.0f);
  ColorMaterialPtr pointerMaterial = ColorMaterial::Make(1.0f, 0.0f, 0.0f);
  NodePtr pointer = Node::Make(trf2, {Node::Make(trf3, {pointerMaterial}, {Triangle::Make(ctx->GetDevice())})});

  ShaderPtr shader = Shader::Make(ctx->GetDevice(), "../shaders/2d/shader.wgsl");
  VertexBufferSpec posBuf;
  posBuf.arrayStride = 2 * 4;
  posBuf.attributes.push_back({std::string("pos"), std::nullopt, WGPUVertexFormat_Float32x2, 0});
  shader->SetVertexBuffers({posBuf});

  PipelineDesc pdesc;
  pdesc.depthStencilMode = DepthStencilMode::None;
  PipelinePtr pipeline = Pipeline::Make(shader, ctx->GetSurfaceFormat(), pdesc);
  shader->AddMaterial(faceMaterial.get());
  shader->AddMaterial(pointerMaterial.get());

  // build scene
  NodePtr root = Node::Make(pipeline, {face, pointer});
  scene = Scene::Make(root);
  scene->AddEngine(MovePointer::Make(trf2));
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

static void update (float dt)
{
  scene->Update(dt);
}

int main ()
{
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#ifdef __APPLE__
  glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
#endif

  glfwSetErrorCallback(error);

  GLFWwindow* win = glfwCreateWindow(600, 600, "2D scene", nullptr, nullptr);
  assert(win);
  glfwSetFramebufferSizeCallback(win, resize);
  glfwSetKeyCallback(win, keyboard);

  int fbw, fbh;
  glfwGetFramebufferSize(win, &fbw, &fbh);
  ctx = WgpuContext::Make(win, fbw, fbh);
  renderer = Renderer::Make(ctx->GetDevice(), false, {0.8, 1.0, 1.0, 1.0});

  initialize();

  float t0 = (float) glfwGetTime();
  while (!glfwWindowShouldClose(win)) {
    float t = (float) glfwGetTime();
    update(t - t0);
    t0 = t;
    display();
    glfwPollEvents();
  }
  ctx = nullptr;
  glfwDestroyWindow(win);
  glfwTerminate();
  return 0;
}
