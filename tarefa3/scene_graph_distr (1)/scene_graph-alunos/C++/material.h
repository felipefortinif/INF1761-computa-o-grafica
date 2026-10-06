#include <memory>
class Material;
using MaterialPtr = std::shared_ptr<Material>;

#ifndef MATERIAL_H
#define MATERIAL_H

#include <map>
#include <string>
#include "appearance.h"
#include "uniformbuffer.h"

class UniformBlock;

// Generic named-value "material" appearance plus a revision counter. No
// Shader/GPU knowledge - each Shader that registers this material owns
// the actual UniformBlock and tracks its own last-uploaded revision (see
// Shader::AddMaterial/BindMaterial), so the same Material instance can be
// used under more than one shader safely.
class Material : public Appearance {
  std::map<std::string, UniformValue> m_values;
  int m_revision;

protected:
  Material ();

public:
  void Set (const std::string& name, const UniformValue& value);
  UniformValue Get (const std::string& name, const UniformValue& defaultValue) const;
  void WriteFields (UniformBlock& block) const;
  int GetRevision () const;

  void Load (StatePtr st) override;
  void Unload (StatePtr st) override;
};

#endif
