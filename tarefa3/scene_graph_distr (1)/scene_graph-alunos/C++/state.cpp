#include "state.h"
#include "pipeline.h"
#include "shader.h"
#include "error.h"

static uint64_t NextStateId ()
{
  static uint64_t next = 1;
  return next++;
}

State::State (CameraPtr camera, WGPUDevice device, WGPURenderPassEncoder renderPass, std::pair<int, int> canvasSize)
  : m_camera(camera), m_device(device), m_renderPass(renderPass), m_canvasSize(canvasSize),
    m_matrixVersion(0), m_id(NextStateId())
{
  m_view = camera->GetViewMatrix();
  m_proj = camera->GetProjMatrix(canvasSize);
  m_viewProj = Mat4Multiply(m_proj, m_view);
}

StatePtr State::Make (CameraPtr camera, WGPUDevice device, WGPURenderPassEncoder renderPass,
                       std::pair<int, int> canvasSize)
{
  return StatePtr(new State(camera, device, renderPass, canvasSize));
}

void State::PushPipeline (PipelinePtr pip) { m_pipelineStack.push_back(pip); }
void State::PopPipeline () { m_pipelineStack.pop_back(); }

PipelinePtr State::GetPipeline () const
{
  if (m_pipelineStack.empty()) Error::Fatal("Pipeline not defined");
  return m_pipelineStack.back();
}

PipelinePtr State::TryGetPipeline () const { return m_pipelineStack.empty() ? nullptr : m_pipelineStack.back(); }

ShaderPtr State::GetShader () const { return GetPipeline()->GetShader(); }

PipelinePtr State::GetActivePipeline () const { return m_activePipeline; }
void State::SetActivePipeline (PipelinePtr pip) { m_activePipeline = pip; }

void State::PushBindGroup (int groupIndex, WGPUBindGroup bindGroup)
{
  m_bindGroupStacks[groupIndex].push_back(bindGroup);
  wgpuRenderPassEncoderSetBindGroup(m_renderPass, (uint32_t) groupIndex, bindGroup, 0, nullptr);
}

void State::PopBindGroup (int groupIndex)
{
  std::vector<WGPUBindGroup>& stack = m_bindGroupStacks[groupIndex];
  stack.pop_back();
  if (!stack.empty())
    wgpuRenderPassEncoderSetBindGroup(m_renderPass, (uint32_t) groupIndex, stack.back(), 0, nullptr);
}

void State::PushMatrix (const Mat4& mat)
{
  m_matrixStack.push_back(Mat4Multiply(GetMatrix(), mat));
  m_matrixVersion++;
}

void State::PushMatrixAbsolute (const Mat4& mat)
{
  m_matrixStack.push_back(mat);
  m_matrixVersion++;
}

void State::PopMatrix ()
{
  m_matrixStack.pop_back();
  m_matrixVersion++;
}

Mat4 State::GetMatrix () const
{
  return m_matrixStack.empty() ? Mat4Identity() : m_matrixStack.back();
}

int State::GetMatrixVersion () const { return m_matrixVersion; }

void State::RegisterMatrixShader (ShaderPtr shader)
{
  for (const ShaderPtr& s : m_matrixShaders)
    if (s == shader) return;
  m_matrixShaders.push_back(shader);
}

const std::vector<ShaderPtr>& State::GetMatrixShaders () const { return m_matrixShaders; }

CameraPtr State::GetCamera () const { return m_camera; }
const Mat4& State::GetViewMatrix () const { return m_view; }
const Mat4& State::GetProjMatrix () const { return m_proj; }
const Mat4& State::GetViewProjMatrix () const { return m_viewProj; }

const Mat4& State::GetInverseViewMatrix ()
{
  if (!m_inverseView) m_inverseView = Mat4Inverse(m_view);
  return *m_inverseView;
}

WGPURenderPassEncoder State::GetRenderPass () const { return m_renderPass; }
std::pair<int, int> State::GetCanvasSize () const { return m_canvasSize; }
uint64_t State::GetId () const { return m_id; }

void State::LoadMatrices ()
{
  // "projection" is deliberately NOT here: it takes the lighting space to
  // NDC and is the same for the whole pass, so it belongs to the scene, not
  // to a node - Shader::CommitGlobal writes it into the "global" block.
  ShaderPtr shd = GetShader();
  Mat4 mat = GetMatrix();

  bool needsVertex = shd->DeclaresMatrixField("vertex");
  bool needsNormal = shd->DeclaresMatrixField("normal");
  if (needsVertex || needsNormal) {
    Mat4 mv = mat;
    if (shd->GetLightingSpace() == "camera") mv = Mat4Multiply(GetViewMatrix(), mv);
    if (needsVertex) m_derived["vertex"] = mv;
    if (needsNormal) {
      // A rank-deficient transform (e.g. the planar-shadow projection,
      // which flattens geometry onto a plane) has no inverse; the pass
      // that uses one never lights anything, so identity is a safe
      // stand-in and beats aborting the program mid-frame.
      Mat4 inv;
      m_derived["normal"] = Mat4TryInverse(mv, &inv) ? Mat4Transpose(inv) : Mat4Identity();
    }
  }
}

std::optional<Mat4> State::GetDerived (const std::string& name) const
{
  auto it = m_derived.find(name);
  if (it == m_derived.end()) return std::nullopt;
  return it->second;
}

void State::UnloadMatrices ()
{
  // nothing is restored: no descendant ever inherits these - the next
  // Node that draws derives its own from its own accumulated matrix.
  m_derived.clear();
}
