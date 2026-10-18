#include <memory>
class Light;
using LightPtr = std::shared_ptr<Light>;

#ifndef LIGHT_H
#define LIGHT_H

#include <map>
#include <string>
#include "uniformbuffer.h"

class Node;
using NodePtr = std::shared_ptr<Node>;
class State;
using StatePtr = std::shared_ptr<State>;
class UniformBlock;

// Base for shader-defined lights. Stores arbitrary named values and
// handles the source-space to shader-space plumbing; deliberately does
// not know which values are geometric - every concrete light implements
// Map(matrix) from scratch.
class Light {
protected:
  std::string m_space; // "world" | "camera" | "local"
  NodePtr m_reference; // only meaningful for "local"
  std::map<std::string, UniformValue> m_values;

  Light (const std::string& space);

public:
  virtual ~Light () {}

  void Set (const std::string& name, const UniformValue& value);
  UniformValue Get (const std::string& name, const UniformValue& defaultValue) const;
  void SetAmbient (float r, float g, float b);
  void SetDiffuse (float r, float g, float b);
  void SetSpecular (float r, float g, float b);
  void SetReference (NodePtr reference);
  NodePtr GetReference () const;

  // Returns this light's values mapped by `matrix`. Concrete lights must
  // implement their own mapping semantics and must not mutate m_values.
  virtual std::map<std::string, UniformValue> Map (const Mat4& matrix) const = 0;

  // Matrix from this light's source space to the shader's world- or
  // camera-space lighting coordinates.
  Mat4 GetMappingMatrix (StatePtr st, const std::string& lightingSpace) const;

  // Builds the source-to-lighting-space matrix, delegates geometric
  // semantics to Map(), and writes fields declared by the shader.
  void WriteFields (UniformBlock& block, StatePtr st, const std::string& lightingSpace) const;
};

#endif
