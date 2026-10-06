#include <memory>
class WgslReflect;
using WgslReflectPtr = std::shared_ptr<WgslReflect>;

#ifndef WGSLREFLECT_H
#define WGSLREFLECT_H

#include <wgpu.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <utility>

// Resolved WGSL field type: either a primitive name (one of "i32", "f32",
// "vec2", "vec3", "vec4", "mat4x4") or, for a field whose type is itself a
// struct declared in the same file, a nested schema.
struct WgslFieldType {
  std::string primitive;
  std::vector<std::pair<std::string, WgslFieldType>> structFields;
};
using WgslSchema = std::vector<std::pair<std::string, WgslFieldType>>;

// Parses one WGSL shader file (regex/manual-scan based, not a full parser)
// into bind-group layout entries, uniform/storage-array field schemas, and
// vertex input locations. Mirrors python/wgslreflect.py's ShaderReflection.
class WgslReflect {
  struct GroupInfo {
    std::vector<WGPUBindGroupLayoutEntry> entries;
    bool hasUniformFields = false;
    WgslSchema uniformFields;
    bool hasStorageArrayFields = false;
    WgslSchema storageArrayFields;
  };

  std::string m_filename;
  std::map<int, GroupInfo> m_groups;
  std::map<std::string, int> m_fields;
  std::map<std::string, std::pair<int, int>> m_textures;
  std::map<std::string, std::pair<int, int>> m_samplers;
  std::map<std::string, int> m_vertexInputs;
  std::map<std::string, int> m_uniformVars;
  std::map<int, std::string> m_groupVarNames;

  using RawFields = std::vector<std::pair<std::string, std::string>>;
  using RawStructMap = std::map<std::string, RawFields>;
  WgslSchema RegisterFields (int idx, const std::string& varname,
                              const RawFields& rawFields, const RawStructMap& structs);

protected:
  WgslReflect (const std::string& filename);

public:
  static WgslReflectPtr Make (const std::string& filename);

  std::vector<int> GroupIndices () const;
  const std::vector<WGPUBindGroupLayoutEntry>& LayoutEntries (int group) const;
  const WgslSchema& UniformFields (int group) const;
  const WgslSchema& StorageArrayFields (int group) const;
  bool HasStorageArrayFields (int group) const;
  bool HasUniformFields (int group) const;

  std::optional<int> UniformVarGroup (const std::string& varname) const;
  std::optional<std::string> GroupVarName (int group) const;
  std::optional<int> FieldGroup (const std::string& name) const;
  std::optional<std::pair<int, int>> TextureBinding (const std::string& name) const;
  std::optional<std::pair<int, int>> SamplerBinding (const std::string& name) const;
  std::optional<int> VertexLocation (const std::string& name) const;
  std::set<int> VertexLocations () const;
};

#endif
