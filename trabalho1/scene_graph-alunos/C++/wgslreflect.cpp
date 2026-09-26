#include "wgslreflect.h"
#include "error.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>

using RawFields = std::vector<std::pair<std::string, std::string>>;
using RawStructMap = std::map<std::string, RawFields>;

static const std::set<std::string> kNormalizable = {"i32", "f32", "vec2", "vec3", "vec4", "mat4x4"};

static std::string Trim (const std::string& s)
{
  size_t a = s.find_first_not_of(" \t\r\n");
  if (a == std::string::npos) return "";
  size_t b = s.find_last_not_of(" \t\r\n");
  return s.substr(a, b - a + 1);
}

static std::vector<std::string> SplitOnComma (const std::string& s)
{
  std::vector<std::string> parts;
  size_t start = 0;
  for (size_t i = 0; i <= s.size(); i++) {
    if (i == s.size() || s[i] == ',') {
      parts.push_back(s.substr(start, i - start));
      start = i + 1;
    }
  }
  return parts;
}

static std::string ReadFile (const std::string& filename)
{
  std::ifstream file(filename);
  if (!file) Error::Fatal("could not open WGSL file: " + filename);
  std::stringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

// WGSL comments, stripped before any parsing: a trailing "// ..." on a
// struct field line would otherwise be glued onto the *next* field's name
// (fields are split on ","), silently making HasField() miss it - and a
// miss means the field is never written, so it reads as zero on the GPU.
static std::string StripComments (const std::string& code)
{
  std::string out;
  out.reserve(code.size());
  for (size_t i = 0; i < code.size(); ) {
    if (code[i] == '/' && i + 1 < code.size() && code[i+1] == '/') {
      while (i < code.size() && code[i] != '\n') i++;          // keeps the '\n'
    } else if (code[i] == '/' && i + 1 < code.size() && code[i+1] == '*') {
      i += 2;
      while (i + 1 < code.size() && !(code[i] == '*' && code[i+1] == '/')) {
        if (code[i] == '\n') out += '\n';                      // keeps line numbers
        i++;
      }
      i = (i + 1 < code.size()) ? i + 2 : code.size();
    } else {
      out += code[i++];
    }
  }
  return out;
}

static RawStructMap ParseStructs (const std::string& code)
{
  RawStructMap structs;
  static const std::regex structRe(R"(struct\s+(\w+)\s*\{([^}]*)\})");
  static const std::regex attrPrefixRe(R"(@\w+(?:\([^)]*\))?\s*)");
  for (auto it = std::sregex_iterator(code.begin(), code.end(), structRe);
       it != std::sregex_iterator(); ++it) {
    const std::smatch& m = *it;
    std::string name = m[1].str();
    RawFields fields;
    for (const std::string& rawPart : SplitOnComma(m[2].str())) {
      std::string part = Trim(std::regex_replace(rawPart, attrPrefixRe, ""));
      if (part.empty()) continue;
      size_t colon = part.find(':');
      if (colon == std::string::npos) continue;
      std::string fname = Trim(part.substr(0, colon));
      std::string ftype = Trim(part.substr(colon + 1));
      if (!fname.empty() && !ftype.empty()) fields.push_back({fname, ftype});
    }
    structs[name] = fields;
  }
  return structs;
}

// Resolves one raw field type to either a primitive name or, for a
// reference to another struct declared in this file, a nested schema.
// `stack` is the chain of struct names being resolved, to fail instead of
// recursing forever on a cyclic struct reference.
static WgslFieldType NormalizeType (const std::string& wgslType, const RawStructMap& structs,
                                     std::vector<std::string> stack = {})
{
  std::string base = Trim(wgslType.substr(0, wgslType.find('<')));
  WgslFieldType result;
  if (kNormalizable.count(base)) {
    result.primitive = base;
    return result;
  }
  auto it = structs.find(base);
  if (it != structs.end()) {
    if (std::find(stack.begin(), stack.end(), base) != stack.end())
      Error::Fatal("Recursive struct definition involving: " + base);
    std::vector<std::string> nextStack = stack;
    nextStack.push_back(base);
    for (const auto& [fname, ftype] : it->second)
      result.structFields.push_back({fname, NormalizeType(ftype, structs, nextStack)});
    return result;
  }
  Error::Fatal("Unsupported WGSL type in uniform block: '" + wgslType + "'");
  return result;
}

static const std::map<std::string, WGPUTextureViewDimension>& TextureViewDims ()
{
  static const std::map<std::string, WGPUTextureViewDimension> dims = {
    {"texture_1d", WGPUTextureViewDimension_1D},
    {"texture_2d", WGPUTextureViewDimension_2D},
    {"texture_2d_array", WGPUTextureViewDimension_2DArray},
    {"texture_cube", WGPUTextureViewDimension_Cube},
    {"texture_cube_array", WGPUTextureViewDimension_CubeArray},
    {"texture_3d", WGPUTextureViewDimension_3D},
    {"texture_depth_2d", WGPUTextureViewDimension_2D},
    {"texture_depth_2d_array", WGPUTextureViewDimension_2DArray},
    {"texture_depth_cube", WGPUTextureViewDimension_Cube},
    {"texture_depth_cube_array", WGPUTextureViewDimension_CubeArray},
  };
  return dims;
}

static WGPUBindGroupLayoutEntry TextureEntry (int binding, const std::string& wgslType)
{
  std::string base = Trim(wgslType.substr(0, wgslType.find('<')));
  WGPUBindGroupLayoutEntry entry = {};
  entry.binding = (uint32_t) binding;
  entry.visibility = WGPUShaderStage_Fragment;
  entry.texture.sampleType = base.rfind("texture_depth", 0) == 0
    ? WGPUTextureSampleType_Depth : WGPUTextureSampleType_Float;
  auto it = TextureViewDims().find(base);
  entry.texture.viewDimension = it != TextureViewDims().end() ? it->second : WGPUTextureViewDimension_2D;
  return entry;
}

static WGPUBindGroupLayoutEntry SamplerEntry (int binding, const std::string& wgslType)
{
  WGPUBindGroupLayoutEntry entry = {};
  entry.binding = (uint32_t) binding;
  entry.visibility = WGPUShaderStage_Fragment;
  entry.sampler.type = wgslType == "sampler_comparison"
    ? WGPUSamplerBindingType_Comparison : WGPUSamplerBindingType_Filtering;
  return entry;
}

static WGPUBindGroupLayoutEntry UniformEntry (int binding)
{
  WGPUBindGroupLayoutEntry entry = {};
  entry.binding = (uint32_t) binding;
  entry.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
  entry.buffer.type = WGPUBufferBindingType_Uniform;
  return entry;
}

static WGPUBindGroupLayoutEntry StorageArrayEntry (int binding)
{
  WGPUBindGroupLayoutEntry entry = {};
  entry.binding = (uint32_t) binding;
  entry.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
  entry.buffer.type = WGPUBufferBindingType_ReadOnlyStorage;
  return entry;
}

WgslSchema WgslReflect::RegisterFields (int idx, const std::string& varname,
                                         const RawFields& rawFields, const RawStructMap& structs)
{
  WgslSchema normalized;
  for (const auto& [fname, ftype] : rawFields)
    normalized.push_back({fname, NormalizeType(ftype, structs)});
  m_uniformVars[varname] = idx;
  for (const auto& [fname, _] : normalized) {
    auto it = m_fields.find(fname);
    if (it != m_fields.end() && it->second != idx)
      Error::Fatal("Field '" + fname + "' declared in more than one group (" +
                   std::to_string(it->second) + " and " + std::to_string(idx) +
                   ") - field names must be unique across a shader's uniform blocks");
    m_fields[fname] = idx;
  }
  return normalized;
}

WgslReflect::WgslReflect (const std::string& filename) : m_filename(filename)
{
  std::string code = StripComments(ReadFile(filename));
  RawStructMap structs = ParseStructs(code);

  static const std::regex bindingRe(
    R"(@group\(\s*(\d+)\s*\)\s*@binding\(\s*(\d+)\s*\)\s*var(?:<([^>]*)>)?\s+(\w+)\s*:\s*([^;]+);)");
  static const std::regex storageArrayRe(R"(array<\s*(\w+)\s*>)");

  for (auto it = std::sregex_iterator(code.begin(), code.end(), bindingRe);
       it != std::sregex_iterator(); ++it) {
    const std::smatch& m = *it;
    int idx = std::stoi(m[1].str());
    int binding = std::stoi(m[2].str());
    std::string addrSpace = Trim(m[3].str());
    std::string varname = m[4].str();
    std::string typeExpr = Trim(m[5].str());

    GroupInfo& g = m_groups[idx];

    if (typeExpr.rfind("texture", 0) == 0) {
      g.entries.push_back(TextureEntry(binding, typeExpr));
      m_textures[varname] = {idx, binding};
      continue;
    }
    if (typeExpr == "sampler" || typeExpr == "sampler_comparison") {
      g.entries.push_back(SamplerEntry(binding, typeExpr));
      m_samplers[varname] = {idx, binding};
      continue;
    }

    std::smatch arrMatch;
    if (addrSpace.rfind("storage", 0) == 0 && std::regex_match(typeExpr, arrMatch, storageArrayRe)) {
      std::string structName = arrMatch[1].str();
      auto sIt = structs.find(structName);
      if (sIt == structs.end())
        Error::Fatal("Storage array '" + varname + "' (group " + std::to_string(idx) +
                     ") references undeclared struct: '" + structName + "'");
      g.entries.push_back(StorageArrayEntry(binding));
      g.hasStorageArrayFields = true;
      g.storageArrayFields = RegisterFields(idx, varname, sIt->second, structs);
      continue;
    }

    auto sIt = structs.find(typeExpr);
    if (sIt == structs.end())
      Error::Fatal("Uniform '" + varname + "' (group " + std::to_string(idx) +
                   ") references undeclared struct: '" + typeExpr + "'");
    g.entries.push_back(UniformEntry(binding));
    g.hasUniformFields = true;
    g.uniformFields = RegisterFields(idx, varname, sIt->second, structs);
  }

  int expected = 0;
  for (const auto& [idx, info] : m_groups) {
    (void) info;
    if (idx != expected)
      Error::Fatal("Bind group indices must be contiguous starting at 0 in " + filename);
    expected++;
  }

  static const std::regex vsMainStartRe(R"(fn\s+vs_main\s*\()");
  std::smatch startMatch;
  if (std::regex_search(code, startMatch, vsMainStartRe)) {
    size_t i = (size_t) (startMatch.position(0) + startMatch.length(0));
    size_t start = i;
    int depth = 1;
    while (i < code.size() && depth > 0) {
      if (code[i] == '(') depth++;
      else if (code[i] == ')') depth--;
      i++;
    }
    std::string params = code.substr(start, i - 1 - start);
    static const std::regex vertexParamRe(R"(@location\(\s*(\d+)\s*\)\s*(\w+)\s*:\s*([^,]+))");
    for (auto pit = std::sregex_iterator(params.begin(), params.end(), vertexParamRe);
         pit != std::sregex_iterator(); ++pit) {
      const std::smatch& pm = *pit;
      m_vertexInputs[pm[2].str()] = std::stoi(pm[1].str());
    }
  }

  for (const auto& [varname, idx] : m_uniformVars)
    m_groupVarNames[idx] = varname;
}

WgslReflectPtr WgslReflect::Make (const std::string& filename)
{
  return WgslReflectPtr(new WgslReflect(filename));
}

std::vector<int> WgslReflect::GroupIndices () const
{
  std::vector<int> indices;
  for (const auto& [idx, info] : m_groups) { (void) info; indices.push_back(idx); }
  return indices;
}

const std::vector<WGPUBindGroupLayoutEntry>& WgslReflect::LayoutEntries (int group) const
{
  auto it = m_groups.find(group);
  if (it == m_groups.end()) Error::Fatal("no such bind group: " + std::to_string(group));
  return it->second.entries;
}

const WgslSchema& WgslReflect::UniformFields (int group) const
{
  auto it = m_groups.find(group);
  if (it == m_groups.end() || !it->second.hasUniformFields)
    Error::Fatal("group " + std::to_string(group) + " has no uniform fields");
  return it->second.uniformFields;
}

const WgslSchema& WgslReflect::StorageArrayFields (int group) const
{
  auto it = m_groups.find(group);
  if (it == m_groups.end() || !it->second.hasStorageArrayFields)
    Error::Fatal("group " + std::to_string(group) + " has no storage array fields");
  return it->second.storageArrayFields;
}

bool WgslReflect::HasStorageArrayFields (int group) const
{
  auto it = m_groups.find(group);
  return it != m_groups.end() && it->second.hasStorageArrayFields;
}

bool WgslReflect::HasUniformFields (int group) const
{
  auto it = m_groups.find(group);
  return it != m_groups.end() && it->second.hasUniformFields;
}

std::optional<int> WgslReflect::UniformVarGroup (const std::string& varname) const
{
  auto it = m_uniformVars.find(varname);
  if (it == m_uniformVars.end()) return std::nullopt;
  return it->second;
}

std::optional<std::string> WgslReflect::GroupVarName (int group) const
{
  auto it = m_groupVarNames.find(group);
  if (it == m_groupVarNames.end()) return std::nullopt;
  return it->second;
}

std::optional<int> WgslReflect::FieldGroup (const std::string& name) const
{
  auto it = m_fields.find(name);
  if (it == m_fields.end()) return std::nullopt;
  return it->second;
}

std::optional<std::pair<int, int>> WgslReflect::TextureBinding (const std::string& name) const
{
  auto it = m_textures.find(name);
  if (it == m_textures.end()) return std::nullopt;
  return it->second;
}

std::optional<std::pair<int, int>> WgslReflect::SamplerBinding (const std::string& name) const
{
  auto it = m_samplers.find(name);
  if (it == m_samplers.end()) return std::nullopt;
  return it->second;
}

std::optional<int> WgslReflect::VertexLocation (const std::string& name) const
{
  auto it = m_vertexInputs.find(name);
  if (it == m_vertexInputs.end()) return std::nullopt;
  return it->second;
}

std::set<int> WgslReflect::VertexLocations () const
{
  std::set<int> locs;
  for (const auto& [name, loc] : m_vertexInputs) { (void) name; locs.insert(loc); }
  return locs;
}
