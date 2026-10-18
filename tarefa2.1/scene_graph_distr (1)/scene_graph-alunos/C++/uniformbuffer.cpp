#include "uniformbuffer.h"
#include "error.h"
#include <cstring>
#include <algorithm>

// (align, size) in bytes, per WGSL layout rules for the "uniform"/
// "storage" address spaces (WebGPU's equivalent of GLSL's std140/std430):
// each field is placed at the next offset that's a multiple of its own
// type's alignment, fields are laid out in declaration order with no
// reordering, and the overall element size is rounded up to 16 bytes.
static std::pair<size_t, size_t> PrimitiveAlignSize (const std::string& wgslType)
{
  if (wgslType == "i32") return {4, 4};
  if (wgslType == "f32") return {4, 4};
  if (wgslType == "vec2") return {8, 8};
  if (wgslType == "vec3") return {16, 12};
  if (wgslType == "vec4") return {16, 16};
  if (wgslType == "mat4x4") return {16, 64};
  Error::Fatal("Unsupported wgsl_type in uniformbuffer: " + wgslType);
  return {0, 0};
}

static size_t AlignOffset (size_t offset, size_t alignment)
{
  return (offset + alignment - 1) / alignment * alignment;
}

struct LayoutSchemaResult {
  std::map<std::string, size_t> offsets;
  std::map<std::string, std::string> types;
  size_t naturalSize;
  size_t align;
};

static LayoutSchemaResult LayoutSchema (const WgslSchema& schema);

static std::pair<size_t, size_t> TypeAlignSize (const WgslFieldType& type)
{
  if (!type.primitive.empty()) return PrimitiveAlignSize(type.primitive);
  LayoutSchemaResult sub = LayoutSchema(type.structFields);
  return {sub.align, AlignOffset(sub.naturalSize, sub.align)};
}

// Lays out `schema`'s own fields starting at offset 0, recursing into any
// nested struct field so the returned offsets/types are flat, with a
// dotted name ("light.color") for every leaf reached through a nested
// struct.
static LayoutSchemaResult LayoutSchema (const WgslSchema& schema)
{
  LayoutSchemaResult result;
  result.naturalSize = 0;
  result.align = 1;
  size_t offset = 0;
  for (const auto& [name, type] : schema) {
    auto [fieldAlign, size] = TypeAlignSize(type);
    result.align = std::max(result.align, fieldAlign);
    offset = AlignOffset(offset, fieldAlign);
    if (type.primitive.empty()) {
      LayoutSchemaResult sub = LayoutSchema(type.structFields);
      for (const auto& [subName, subOffset] : sub.offsets) {
        result.offsets[name + "." + subName] = offset + subOffset;
        result.types[name + "." + subName] = sub.types[subName];
      }
    } else {
      result.offsets[name] = offset;
      result.types[name] = type.primitive;
    }
    offset += size;
  }
  result.naturalSize = offset;
  return result;
}

// Computes every leaf field's byte offset and WGSL primitive type (flat,
// dotted-name for nested structs) and the overall 16-byte-rounded element
// size, shared by StorageArray (stride) and UniformBlock (buffer size).
static void ComputeLayout (const WgslSchema& schema, std::map<std::string, size_t>& offsets,
                            std::map<std::string, std::string>& types, size_t& size)
{
  LayoutSchemaResult r = LayoutSchema(schema);
  offsets = r.offsets;
  types = r.types;
  size = AlignOffset(r.naturalSize, 16);
}

template <typename T>
static T GetOrFatal (const UniformValue& value, const std::string& wgslType, const std::string& name)
{
  if (!std::holds_alternative<T>(value))
    Error::Fatal("uniform value for field '" + name + "' does not match wgsl type '" + wgslType + "'");
  return std::get<T>(value);
}

static void PackInto (uint8_t* dest, const std::string& name, const UniformValue& value, const std::string& wgslType)
{
  if (wgslType == "i32") {
    int v = GetOrFatal<int>(value, wgslType, name);
    memcpy(dest, &v, 4);
  } else if (wgslType == "f32") {
    float v = std::holds_alternative<float>(value) ? std::get<float>(value)
             : (float) GetOrFatal<int>(value, wgslType, name);
    memcpy(dest, &v, 4);
  } else if (wgslType == "vec2") {
    Vec2 v = GetOrFatal<Vec2>(value, wgslType, name);
    memcpy(dest, &v, 8);
  } else if (wgslType == "vec3") {
    Vec3 v = GetOrFatal<Vec3>(value, wgslType, name);
    memcpy(dest, &v, 12);
  } else if (wgslType == "vec4") {
    Vec4 v = GetOrFatal<Vec4>(value, wgslType, name);
    memcpy(dest, &v, 16);
  } else if (wgslType == "mat4x4") {
    Mat4 v = GetOrFatal<Mat4>(value, wgslType, name);
    memcpy(dest, v.m, 64);
  } else {
    Error::Fatal("Unsupported wgsl_type in uniformbuffer: " + wgslType);
  }
}

// Packs `values` (missing fields left as zero) into one `stride`-sized
// row, fields placed at their precomputed offsets.
static std::vector<uint8_t> PackRow (const std::map<std::string, UniformValue>& values,
                                      const std::map<std::string, std::string>& types,
                                      const std::map<std::string, size_t>& offsets, size_t stride)
{
  std::vector<uint8_t> row(stride, 0);
  for (const auto& [name, wgslType] : types) {
    auto it = values.find(name);
    if (it == values.end()) continue;
    PackInto(row.data() + offsets.at(name), name, it->second, wgslType);
  }
  return row;
}

// --- StorageArray ---

StorageArray::StorageArray (WGPUDevice device, const WgslSchema& schema, WGPUBindGroupLayout layout,
                             int groupIndex, size_t maxRows, int binding)
  : m_device(device), m_queue(wgpuDeviceGetQueue(device)), m_schema(schema),
    m_groupIndex(groupIndex), m_maxRows(maxRows), m_count(0)
{
  ComputeLayout(schema, m_offsets, m_types, m_stride);

  WGPUBufferDescriptor bufDesc = {};
  bufDesc.size = m_stride * m_maxRows;
  bufDesc.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst;
  m_buffer = wgpuDeviceCreateBuffer(m_device, &bufDesc);

  WGPUBindGroupEntry entry = {};
  entry.binding = (uint32_t) binding;
  entry.buffer = m_buffer;
  entry.offset = 0;
  entry.size = m_stride * m_maxRows;

  WGPUBindGroupDescriptor groupDesc = {};
  groupDesc.layout = layout;
  groupDesc.entryCount = 1;
  groupDesc.entries = &entry;
  m_bindGroup = wgpuDeviceCreateBindGroup(m_device, &groupDesc);
}

StorageArray::~StorageArray ()
{
  wgpuBindGroupRelease(m_bindGroup);
  wgpuBufferRelease(m_buffer);
  wgpuQueueRelease(m_queue);
}

int StorageArray::Append (const std::map<std::string, UniformValue>& values)
{
  if (m_count >= m_maxRows)
    Error::Fatal("StorageArray for group " + std::to_string(m_groupIndex) + " exceeded its " +
                 std::to_string(m_maxRows) + "-row capacity (see Shader's max_instances)");
  size_t row = m_count;
  std::vector<uint8_t> packed = PackRow(values, m_types, m_offsets, m_stride);
  if (m_staging.size() < m_maxRows * m_stride) m_staging.resize(m_maxRows * m_stride);
  std::copy(packed.begin(), packed.end(), m_staging.begin() + row * m_stride);
  m_count++;
  return (int) row;
}

void StorageArray::Flush ()
{
  if (m_count == 0) return;
  wgpuQueueWriteBuffer(m_queue, m_buffer, 0, m_staging.data(), m_count * m_stride);
}

void StorageArray::Reset () { m_count = 0; }
WGPUBindGroup StorageArray::GetBindGroup () const { return m_bindGroup; }
int StorageArray::GetGroupIndex () const { return m_groupIndex; }
const WgslSchema& StorageArray::GetSchema () const { return m_schema; }

// --- UniformBlock ---

UniformBlock::UniformBlock (WGPUDevice device, const WgslSchema& schema, WGPUBindGroupLayout layout,
                             int groupIndex, int binding)
  : m_device(device), m_queue(wgpuDeviceGetQueue(device)), m_groupIndex(groupIndex), m_hasPending(false)
{
  ComputeLayout(schema, m_offsets, m_types, m_size);

  WGPUBufferDescriptor bufDesc = {};
  bufDesc.size = m_size;
  bufDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
  m_buffer = wgpuDeviceCreateBuffer(m_device, &bufDesc);

  WGPUBindGroupEntry entry = {};
  entry.binding = (uint32_t) binding;
  entry.buffer = m_buffer;
  entry.offset = 0;
  entry.size = m_size;

  WGPUBindGroupDescriptor groupDesc = {};
  groupDesc.layout = layout;
  groupDesc.entryCount = 1;
  groupDesc.entries = &entry;
  m_bindGroup = wgpuDeviceCreateBindGroup(m_device, &groupDesc);
}

UniformBlock::~UniformBlock ()
{
  wgpuBindGroupRelease(m_bindGroup);
  wgpuBufferRelease(m_buffer);
  wgpuQueueRelease(m_queue);
}

bool UniformBlock::HasField (const std::string& name) const { return m_offsets.count(name) > 0; }

void UniformBlock::Begin ()
{
  m_pending.assign(m_size, 0);
  m_hasPending = true;
}

void UniformBlock::Set (const std::string& name, const UniformValue& value)
{
  auto it = m_offsets.find(name);
  if (it == m_offsets.end()) Error::Fatal("'" + name + "' is not a field of this shader's group");
  if (!m_hasPending) Error::Fatal("UniformBlock::Set called without a matching Begin()");
  PackInto(m_pending.data() + it->second, name, value, m_types.at(name));
}

void UniformBlock::End ()
{
  if (!m_hasPending) Error::Fatal("UniformBlock::End called without a matching Begin()");
  wgpuQueueWriteBuffer(m_queue, m_buffer, 0, m_pending.data(), m_pending.size());
  m_hasPending = false;
}

WGPUBindGroup UniformBlock::GetBindGroup () const { return m_bindGroup; }
int UniformBlock::GetGroupIndex () const { return m_groupIndex; }
