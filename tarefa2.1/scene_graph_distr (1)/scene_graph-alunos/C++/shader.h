#include <memory>
class Shader;
using ShaderPtr = std::shared_ptr<Shader>;

#ifndef SHADER_H
#define SHADER_H

#include <wgpu.h>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include "wgslreflect.h"
#include "uniformbuffer.h"
#include "light.h"

class State;
using StatePtr = std::shared_ptr<State>;
class Material;
class TextureSet;

// One vertex attribute, resolved to a shaderLocation either directly or
// via the WGSL input variable name it corresponds to (see
// Shader::SetVertexBuffers).
struct VertexAttributeSpec {
  std::optional<std::string> varName;
  std::optional<uint32_t> shaderLocation;
  WGPUVertexFormat format;
  uint64_t offset = 0;
};

struct VertexBufferSpec {
  uint64_t arrayStride;
  WGPUVertexStepMode stepMode = WGPUVertexStepMode_Vertex;
  std::vector<VertexAttributeSpec> attributes;
};

// Wraps a compiled WGSL module: vertex input layout, lighting
// (light/space), and bind groups discovered via reflection. Owns
// everything related to shading - materials, "global" uniforms, and
// textures/samplers - each created and validated once, eagerly, at
// setup, never during graph traversal. Rasterizer state lives in
// Pipeline, not here.
class Shader : public std::enable_shared_from_this<Shader> {
  WGPUDevice m_device;
  LightPtr m_light;
  std::string m_space;

  WGPUShaderModule m_module;
  WgslReflectPtr m_reflection;
  std::map<int, WGPUBindGroupLayout> m_layouts;

  std::unique_ptr<StorageArray> m_matrixArray;
  uint64_t m_frameStateId;
  int m_matrixTick;
  int m_matrixRow;

  std::optional<int> m_materialGroup;
  std::map<Material*, std::unique_ptr<UniformBlock>> m_materials;
  std::map<Material*, int> m_materialRevisions;

  std::optional<int> m_globalGroup;
  std::unique_ptr<UniformBlock> m_globalBlock;
  std::map<std::string, UniformValue> m_values;

  std::optional<int> m_textureGroup;
  std::map<TextureSet*, WGPUBindGroup> m_textureSets;

  std::vector<WGPUVertexBufferLayout> m_vertexBuffers;
  std::vector<std::vector<WGPUVertexAttribute>> m_vertexAttributeStorage;

  Vec4 ComputeCameraPosition (StatePtr st) const;
  Mat4 ComputeProjectionMatrix (StatePtr st) const;
  std::pair<int, int> ResolveBinding (const std::string& varname) const;

protected:
  Shader (WGPUDevice device, const std::string& wgslPath, LightPtr light, const std::string& space, size_t maxInstances);

public:
  static ShaderPtr Make (WGPUDevice device, const std::string& wgslPath, LightPtr light = nullptr,
                          const std::string& space = "camera", size_t maxInstances = 1024);
  ~Shader ();

  WGPUDevice GetDevice () const;

  // --- "matrix" storage array ---
  int CommitMatrix (StatePtr st);
  WGPUBindGroup GetMatrixBindGroup () const;
  int GetMatrixGroupIndex () const;
  bool DeclaresMatrixField (const std::string& name) const;

  // --- "material" group ---
  void AddMaterial (Material* mat);
  void BindMaterial (StatePtr st, Material* mat);
  void UnbindMaterial (StatePtr st);

  // --- "global" group ---
  void SetValue (const std::string& name, const UniformValue& value);
  UniformValue GetValue (const std::string& name, const UniformValue& defaultValue) const;
  void CommitGlobal (StatePtr st);
  // Uploads this pass's matrix rows in one transfer; called by
  // Renderer/Algorithm after the traversal, before submit.
  void FlushMatrices ();
  void UnbindGlobal (StatePtr st);

  // --- textures/samplers ---
  void AddTextureSet (TextureSet* ts);
  void BindTextureSet (StatePtr st, TextureSet* ts);
  void UnbindTextureSet (StatePtr st);

  // --- used by State to resolve a pushed field name to its group ---

  // --- used by Pipeline to build the GPURenderPipeline ---
  WGPUShaderModule GetModule () const;
  std::vector<WGPUBindGroupLayout> GetBindGroupLayouts () const;
  const std::string& GetLightingSpace () const;

  // --- vertex input contract ---
  void SetVertexBuffers (const std::vector<VertexBufferSpec>& buffers);
  const std::vector<WGPUVertexBufferLayout>& GetVertexBufferLayout () const;
};

#endif
