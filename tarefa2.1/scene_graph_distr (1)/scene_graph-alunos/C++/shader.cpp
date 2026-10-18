#include "shader.h"
#include "state.h"
#include "material.h"
#include "textureset.h"
#include "error.h"
#include <fstream>
#include <sstream>
#include <set>

static std::string ReadFile (const std::string& path)
{
  std::ifstream file(path);
  if (!file) Error::Fatal("could not open WGSL file: " + path);
  std::stringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

Shader::Shader (WGPUDevice device, const std::string& wgslPath, LightPtr light,
                 const std::string& space, size_t maxInstances)
  : m_device(device), m_light(light), m_space(space), m_frameStateId(0), m_matrixTick(-1), m_matrixRow(-1)
{
  std::string code = ReadFile(wgslPath);
  WGPUShaderSourceWGSL wgslSource = {};
  wgslSource.chain.sType = WGPUSType_ShaderSourceWGSL;
  wgslSource.code = {code.c_str(), code.size()};
  WGPUShaderModuleDescriptor moduleDesc = {};
  moduleDesc.nextInChain = &wgslSource.chain;
  m_module = wgpuDeviceCreateShaderModule(device, &moduleDesc);

  m_reflection = WgslReflect::Make(wgslPath);

  for (int g : m_reflection->GroupIndices()) {
    const std::vector<WGPUBindGroupLayoutEntry>& entries = m_reflection->LayoutEntries(g);
    WGPUBindGroupLayoutDescriptor layoutDesc = {};
    layoutDesc.entryCount = entries.size();
    layoutDesc.entries = entries.data();
    m_layouts[g] = wgpuDeviceCreateBindGroupLayout(device, &layoutDesc);
  }

  std::optional<int> matrixGroup = m_reflection->UniformVarGroup("matrix");
  if (matrixGroup) {
    const WgslSchema& fields = m_reflection->StorageArrayFields(*matrixGroup);
    m_matrixArray = std::make_unique<StorageArray>(device, fields, m_layouts[*matrixGroup], *matrixGroup, maxInstances);
  }

  m_materialGroup = m_reflection->UniformVarGroup("material");

  m_globalGroup = m_reflection->UniformVarGroup("global");
  if (m_globalGroup) {
    const WgslSchema& fields = m_reflection->UniformFields(*m_globalGroup);
    m_globalBlock = std::make_unique<UniformBlock>(device, fields, m_layouts[*m_globalGroup], *m_globalGroup);
  }

  std::vector<int> textureGroups;
  for (int g : m_reflection->GroupIndices())
    if (!m_reflection->HasUniformFields(g) && !m_reflection->HasStorageArrayFields(g))
      textureGroups.push_back(g);
  if (textureGroups.size() > 1)
    Error::Fatal("shader declares more than one texture/sampler-only group - merge them into a single @group");
  m_textureGroup = textureGroups.empty() ? std::nullopt : std::optional<int>(textureGroups[0]);
}

ShaderPtr Shader::Make (WGPUDevice device, const std::string& wgslPath, LightPtr light,
                         const std::string& space, size_t maxInstances)
{
  return ShaderPtr(new Shader(device, wgslPath, light, space, maxInstances));
}

Shader::~Shader ()
{
  for (auto& [g, layout] : m_layouts) wgpuBindGroupLayoutRelease(layout);
  for (auto& [ts, bg] : m_textureSets) wgpuBindGroupRelease(bg);
  wgpuShaderModuleRelease(m_module);
}

WGPUDevice Shader::GetDevice () const { return m_device; }

int Shader::CommitMatrix (StatePtr st)
{
  if (!m_matrixArray) Error::Fatal("CommitMatrix called on a shader with no \"matrix\" storage array declared");
  if (st->GetId() != m_frameStateId) {
    m_frameStateId = st->GetId();
    m_matrixArray->Reset();
    m_matrixTick = -1;
    m_matrixRow = -1;
  }
  st->RegisterMatrixShader(shared_from_this());
  int latest = st->GetMatrixVersion();
  if (latest != m_matrixTick) {
    std::map<std::string, UniformValue> values;
    for (const auto& [name, type] : m_matrixArray->GetSchema()) {
      (void) type;
      std::optional<Mat4> v = st->GetDerived(name);
      if (v) values[name] = *v;
    }
    m_matrixRow = m_matrixArray->Append(values);
    m_matrixTick = latest;
  }
  return m_matrixRow;
}

WGPUBindGroup Shader::GetMatrixBindGroup () const
{
  if (!m_matrixArray) Error::Fatal("GetMatrixBindGroup called on a shader with no \"matrix\" storage array declared");
  return m_matrixArray->GetBindGroup();
}

int Shader::GetMatrixGroupIndex () const
{
  if (!m_matrixArray) Error::Fatal("GetMatrixGroupIndex called on a shader with no \"matrix\" storage array declared");
  return m_matrixArray->GetGroupIndex();
}

bool Shader::DeclaresMatrixField (const std::string& name) const
{
  if (!m_matrixArray) return false;
  for (const auto& [fname, ftype] : m_matrixArray->GetSchema()) {
    (void) ftype;
    if (fname == name) return true;
  }
  return false;
}

void Shader::AddMaterial (Material* mat)
{
  if (!m_materialGroup) Error::Fatal("this shader has no \"material\" group");
  const WgslSchema& fields = m_reflection->UniformFields(*m_materialGroup);
  auto block = std::make_unique<UniformBlock>(m_device, fields, m_layouts[*m_materialGroup], *m_materialGroup);
  block->Begin();
  mat->WriteFields(*block);
  block->End();
  m_materialRevisions[mat] = mat->GetRevision();
  m_materials[mat] = std::move(block);
}

void Shader::BindMaterial (StatePtr st, Material* mat)
{
  if (!m_materialGroup) return;
  auto it = m_materials.find(mat);
  if (it == m_materials.end()) Error::Fatal("Material was used under a shader it was never AddMaterial()'d to");
  UniformBlock* block = it->second.get();
  if (m_materialRevisions[mat] != mat->GetRevision()) {
    block->Begin();
    mat->WriteFields(*block);
    block->End();
    m_materialRevisions[mat] = mat->GetRevision();
  }
  st->PushBindGroup(block->GetGroupIndex(), block->GetBindGroup());
}

void Shader::UnbindMaterial (StatePtr st)
{
  if (!m_materialGroup) return;
  st->PopBindGroup(*m_materialGroup);
}

void Shader::SetValue (const std::string& name, const UniformValue& value) { m_values[name] = value; }

UniformValue Shader::GetValue (const std::string& name, const UniformValue& defaultValue) const
{
  auto it = m_values.find(name);
  return it != m_values.end() ? it->second : defaultValue;
}

Vec4 Shader::ComputeCameraPosition (StatePtr st) const
{
  Vec4 cameraPosition = {0, 0, 0, 1};
  if (GetLightingSpace() == "world") cameraPosition = Mat4MulVec4(st->GetInverseViewMatrix(), cameraPosition);
  return cameraPosition;
}

// The matrix taking this shader's lighting space to NDC - the scene's
// half of the old per-instance MVP. In camera space the view transform is
// already folded into "vertex", so only the projection remains; in world
// space it is projection * view.
Mat4 Shader::ComputeProjectionMatrix (StatePtr st) const
{
  if (GetLightingSpace() == "camera") return st->GetProjMatrix();
  return st->GetViewProjMatrix();
}

void Shader::FlushMatrices ()
{
  if (m_matrixArray) m_matrixArray->Flush();
}

void Shader::CommitGlobal (StatePtr st)
{
  UniformBlock* block = m_globalBlock.get();
  if (!block) return;
  block->Begin();
  if (block->HasField("projection")) block->Set("projection", ComputeProjectionMatrix(st));
  if (block->HasField("camera_position")) block->Set("camera_position", ComputeCameraPosition(st));
  if (m_light) m_light->WriteFields(*block, st, GetLightingSpace());
  for (const auto& [name, value] : m_values) block->Set(name, value);
  block->End();
  st->PushBindGroup(block->GetGroupIndex(), block->GetBindGroup());
}

void Shader::UnbindGlobal (StatePtr st)
{
  if (!m_globalBlock) return;
  st->PopBindGroup(m_globalBlock->GetGroupIndex());
}

std::pair<int, int> Shader::ResolveBinding (const std::string& varname) const
{
  std::optional<std::pair<int, int>> texBinding = m_reflection->TextureBinding(varname);
  if (texBinding) return *texBinding;
  std::optional<std::pair<int, int>> samplerBinding = m_reflection->SamplerBinding(varname);
  if (samplerBinding) return *samplerBinding;
  Error::Fatal("'" + varname + "' is not declared by this shader");
  return {-1, -1};
}

void Shader::AddTextureSet (TextureSet* ts)
{
  if (!m_textureGroup) Error::Fatal("this shader has no texture/sampler group");
  std::vector<WGPUBindGroupEntry> entries;
  std::set<int> seen;
  for (TextureItem* item : ts->GetItems()) {
    auto [group, binding] = ResolveBinding(item->GetVarName());
    if (group != *m_textureGroup) Error::Fatal("'" + item->GetVarName() + "' does not belong to this shader's texture group");
    if (seen.count(binding)) Error::Fatal("binding " + std::to_string(binding) + " supplied more than once");
    seen.insert(binding);
    entries.push_back(item->MakeEntry((uint32_t) binding));
  }
  size_t expected = m_reflection->LayoutEntries(*m_textureGroup).size();
  if (entries.size() != expected)
    Error::Fatal("texture set covers " + std::to_string(entries.size()) + " binding(s), shader declares " +
                 std::to_string(expected));

  WGPUBindGroupDescriptor groupDesc = {};
  groupDesc.layout = m_layouts[*m_textureGroup];
  groupDesc.entryCount = entries.size();
  groupDesc.entries = entries.data();
  m_textureSets[ts] = wgpuDeviceCreateBindGroup(m_device, &groupDesc);

  for (TextureItem* item : ts->GetItems()) item->Retain();
}

void Shader::BindTextureSet (StatePtr st, TextureSet* ts)
{
  if (!m_textureGroup) return;
  auto it = m_textureSets.find(ts);
  if (it == m_textureSets.end()) Error::Fatal("TextureSet was used under a shader it was never AddTextureSet()'d to");
  st->PushBindGroup(*m_textureGroup, it->second);
}

void Shader::UnbindTextureSet (StatePtr st)
{
  if (!m_textureGroup) return;
  st->PopBindGroup(*m_textureGroup);
}


WGPUShaderModule Shader::GetModule () const { return m_module; }

std::vector<WGPUBindGroupLayout> Shader::GetBindGroupLayouts () const
{
  std::vector<WGPUBindGroupLayout> layouts;
  for (int g : m_reflection->GroupIndices()) layouts.push_back(m_layouts.at(g));
  return layouts;
}

const std::string& Shader::GetLightingSpace () const { return m_space; }

void Shader::SetVertexBuffers (const std::vector<VertexBufferSpec>& buffers)
{
  m_vertexBuffers.clear();
  m_vertexAttributeStorage.clear();
  m_vertexAttributeStorage.resize(buffers.size());

  std::set<int> covered;
  for (size_t i = 0; i < buffers.size(); i++) {
    const VertexBufferSpec& buf = buffers[i];
    std::vector<WGPUVertexAttribute>& attrs = m_vertexAttributeStorage[i];
    for (const VertexAttributeSpec& attr : buf.attributes) {
      WGPUVertexAttribute wgpuAttr = {};
      wgpuAttr.format = attr.format;
      wgpuAttr.offset = attr.offset;
      int location;
      if (attr.varName) {
        std::optional<int> loc = m_reflection->VertexLocation(*attr.varName);
        if (!loc)
          Error::Fatal("SetVertexBuffers references '" + *attr.varName +
                       "', but this shader doesn't declare a vertex input with that name");
        location = *loc;
      } else {
        location = (int) *attr.shaderLocation;
      }
      wgpuAttr.shaderLocation = (uint32_t) location;
      if (covered.count(location))
        Error::Fatal("Vertex buffer location " + std::to_string(location) + " registered more than once via SetVertexBuffers");
      covered.insert(location);
      attrs.push_back(wgpuAttr);
    }
    WGPUVertexBufferLayout layout = {};
    layout.stepMode = buf.stepMode;
    layout.arrayStride = buf.arrayStride;
    layout.attributeCount = attrs.size();
    layout.attributes = attrs.data();
    m_vertexBuffers.push_back(layout);
  }

  std::set<int> declared = m_reflection->VertexLocations();
  std::vector<int> missing;
  for (int loc : declared) if (!covered.count(loc)) missing.push_back(loc);
  if (!missing.empty()) {
    std::string list;
    for (size_t i = 0; i < missing.size(); i++) list += (i ? ", " : "") + std::to_string(missing[i]);
    Error::Fatal("Shader declares vertex input location(s) " + list + " with no matching SetVertexBuffers attribute");
  }
}

const std::vector<WGPUVertexBufferLayout>& Shader::GetVertexBufferLayout () const { return m_vertexBuffers; }
