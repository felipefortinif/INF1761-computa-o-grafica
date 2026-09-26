#include "material.h"
#include "state.h"
#include "shader.h"

Material::Material () : m_revision(0) {}

void Material::Set (const std::string& name, const UniformValue& value)
{
  m_values[name] = value;
  m_revision++;
}

UniformValue Material::Get (const std::string& name, const UniformValue& defaultValue) const
{
  auto it = m_values.find(name);
  return it != m_values.end() ? it->second : defaultValue;
}

void Material::WriteFields (UniformBlock& block) const
{
  for (const auto& [name, value] : m_values)
    if (block.HasField(name)) block.Set(name, value);
}

int Material::GetRevision () const { return m_revision; }

void Material::Load (StatePtr st) { st->GetShader()->BindMaterial(st, this); }
void Material::Unload (StatePtr st) { st->GetShader()->UnbindMaterial(st); }
