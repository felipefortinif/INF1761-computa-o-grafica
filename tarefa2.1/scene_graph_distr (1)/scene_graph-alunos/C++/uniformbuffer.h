#ifndef UNIFORMBUFFER_H
#define UNIFORMBUFFER_H

#include <wgpu.h>
#include <string>
#include <map>
#include <vector>
#include <variant>
#include "wgslreflect.h"
#include "graphicsmath.h"

// A value written into a StorageArray row or a UniformBlock field.
using UniformValue = std::variant<int, float, Vec2, Vec3, Vec4, Mat4>;

// A growable `var<storage, read> name: array<StructType>` buffer - used
// for the "matrix" group, where every draw call needs its own row (many
// transforms per frame, appended rather than overwritten). Pre-allocated
// to maxRows; Append fails fast if that capacity is exceeded.
class StorageArray {
  WGPUDevice m_device;
  WGPUQueue m_queue;
  WgslSchema m_schema;
  int m_groupIndex;
  size_t m_maxRows;
  std::map<std::string, size_t> m_offsets;
  std::map<std::string, std::string> m_types;
  size_t m_stride;
  size_t m_count;
  WGPUBuffer m_buffer;
  WGPUBindGroup m_bindGroup;

  std::vector<uint8_t> m_staging;
public:
  StorageArray (WGPUDevice device, const WgslSchema& schema, WGPUBindGroupLayout layout,
                int groupIndex, size_t maxRows, int binding = 0);
  ~StorageArray ();

  // Packs into the CPU staging buffer and returns the row index;
  // nothing reaches the GPU until Flush.
  int Append (const std::map<std::string, UniformValue>& values);
  // Uploads every row appended since Reset in one write - must run
  // before the pass's submit (write_buffer is on the queue timeline).
  void Flush ();
  void Reset ();
  WGPUBindGroup GetBindGroup () const;
  int GetGroupIndex () const;
  const WgslSchema& GetSchema () const;
};

// A single-instance `var<uniform> name: StructType` buffer + bind group,
// built once and reused for its owner's whole lifetime. Written via
// Begin/Set/End: Set fails fast on a field this block's schema doesn't
// declare - callers that want to skip an optional field must call
// HasField first.
class UniformBlock {
  WGPUDevice m_device;
  WGPUQueue m_queue;
  int m_groupIndex;
  std::map<std::string, size_t> m_offsets;
  std::map<std::string, std::string> m_types;
  size_t m_size;
  std::vector<uint8_t> m_pending;
  bool m_hasPending;
  WGPUBuffer m_buffer;
  WGPUBindGroup m_bindGroup;

public:
  UniformBlock (WGPUDevice device, const WgslSchema& schema, WGPUBindGroupLayout layout,
                int groupIndex, int binding = 0);
  ~UniformBlock ();

  bool HasField (const std::string& name) const;
  void Begin ();
  void Set (const std::string& name, const UniformValue& value);
  void End ();
  WGPUBindGroup GetBindGroup () const;
  int GetGroupIndex () const;
};

#endif
