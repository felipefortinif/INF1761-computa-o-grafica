#include <memory>
class State;
using StatePtr = std::shared_ptr<State>;

#ifndef STATE_H
#define STATE_H

#include <wgpu.h>
#include <cstdint>
#include <utility>
#include <vector>
#include <map>
#include <string>
#include <optional>
#include "camera.h"
#include "graphicsmath.h"
#include "uniformbuffer.h"

class Pipeline;
using PipelinePtr = std::shared_ptr<Pipeline>;
class Shader;
using ShaderPtr = std::shared_ptr<Shader>;

// Per-render-pass state threaded through Scene::Render/Node::Render: the
// active camera/device/render pass, the pipeline stack, a per-group
// bind-group stack (used by Material/TextureSet), and a flat named-value
// stack per "matrix"-group field name (fed by Transform, read by
// Shader::CommitMatrix). "material"/"global"/textures don't go through
// this value stack at all - each Shader owns its own persistent buffers.
class State : public std::enable_shared_from_this<State> {
  CameraPtr m_camera;
  WGPUDevice m_device;
  WGPURenderPassEncoder m_renderPass;
  std::pair<int, int> m_canvasSize;
  std::vector<PipelinePtr> m_pipelineStack;
  PipelinePtr m_activePipeline;
  std::map<int, std::vector<WGPUBindGroup>> m_bindGroupStacks;
  // the only inherited thing that isn't a bind group: the accumulated
  // matrix. A dedicated stack, with no name - this was a generic
  // map<string, vector> that in practice only ever held "matrix".
  std::vector<Mat4> m_matrixStack;
  int m_matrixVersion;
  // vertex/normal are not traversal state: they are derived from the
  // accumulated matrix at each drawing Node, read by
  // Shader::CommitMatrix in that same Node, then discarded. Plain
  // values, therefore - not stacks.
  std::map<std::string, Mat4> m_derived;
  // shaders that appended a matrix row in this pass, so Renderer /
  // Algorithm know whose array to flush before submit
  std::vector<ShaderPtr> m_matrixShaders;
  Mat4 m_view, m_proj, m_viewProj;
  std::optional<Mat4> m_inverseView;
  uint64_t m_id;

protected:
  State (CameraPtr camera, WGPUDevice device, WGPURenderPassEncoder renderPass, std::pair<int, int> canvasSize);

public:
  static StatePtr Make (CameraPtr camera, WGPUDevice device, WGPURenderPassEncoder renderPass,
                         std::pair<int, int> canvasSize);

  void PushPipeline (PipelinePtr pip);
  void PopPipeline ();
  PipelinePtr GetPipeline () const;
  PipelinePtr TryGetPipeline () const;
  ShaderPtr GetShader () const;
  PipelinePtr GetActivePipeline () const;
  void SetActivePipeline (PipelinePtr pip);

  void PushBindGroup (int groupIndex, WGPUBindGroup bindGroup);
  void PopBindGroup (int groupIndex);

  // Pushes the current top composed with `mat` - matrix accumulation,
  // where each Transform composes onto its ancestors' matrix rather
  // than overriding it. Identity is the implicit bottom of the stack.
  void PushMatrix (const Mat4& mat);
  // Pushes `mat` *without* composing, discarding what the ancestors
  // accumulated - for the rare transform that positions itself in the
  // scene rather than relative to its parent (see SkyBoxTransform).
  void PushMatrixAbsolute (const Mat4& mat);
  void PopMatrix ();
  // The accumulated matrix of the path down to here - identity if no
  // Transform pushed anything yet.
  Mat4 GetMatrix () const;
  // Bumped by every push/pop, so Shader::CommitMatrix can tell whether
  // the accumulated matrix changed since the row it last handed out.
  int GetMatrixVersion () const;
  // The matrix LoadMatrices derived for `name` in the Node being drawn,
  // or nullopt if this shader doesn't declare it. Read by CommitMatrix.
  std::optional<Mat4> GetDerived (const std::string& name) const;

  void RegisterMatrixShader (ShaderPtr shader);
  const std::vector<ShaderPtr>& GetMatrixShaders () const;

  CameraPtr GetCamera () const;
  const Mat4& GetViewMatrix () const;
  const Mat4& GetProjMatrix () const;
  const Mat4& GetViewProjMatrix () const;
  const Mat4& GetInverseViewMatrix ();

  WGPURenderPassEncoder GetRenderPass () const;
  std::pair<int, int> GetCanvasSize () const;

  // Unique per-instance id, monotonically increasing across every State
  // ever constructed. Shader::CommitMatrix uses this (not pointer
  // identity) to detect "this is a new render pass" - a State's heap
  // address can be reused by the very next frame's State once the
  // previous one is destroyed, which would silently defeat a raw
  // pointer comparison.
  uint64_t GetId () const;

  void LoadMatrices ();
  void UnloadMatrices ();
};

#endif
