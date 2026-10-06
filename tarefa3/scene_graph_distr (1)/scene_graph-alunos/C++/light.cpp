#include "light.h"
#include "state.h"
#include "node.h"
#include "error.h"

Light::Light (const std::string& space) : m_space(space)
{
  if (space != "world" && space != "camera" && space != "local")
    Error::Fatal("Light space must be 'world', 'camera', or 'local'");
  m_values["light_ambient"] = Vec3{0.2f, 0.2f, 0.2f};
  m_values["light_diffuse"] = Vec3{0.8f, 0.8f, 0.8f};
  m_values["light_specular"] = Vec3{1.0f, 1.0f, 1.0f};
}

void Light::Set (const std::string& name, const UniformValue& value) { m_values[name] = value; }

UniformValue Light::Get (const std::string& name, const UniformValue& defaultValue) const
{
  auto it = m_values.find(name);
  return it != m_values.end() ? it->second : defaultValue;
}

void Light::SetAmbient (float r, float g, float b) { Set("light_ambient", Vec3{r, g, b}); }
void Light::SetDiffuse (float r, float g, float b) { Set("light_diffuse", Vec3{r, g, b}); }
void Light::SetSpecular (float r, float g, float b) { Set("light_specular", Vec3{r, g, b}); }

void Light::SetReference (NodePtr reference)
{
  if (m_space != "local") Error::Fatal("A reference node can only be assigned to a local-space light");
  m_reference = reference;
}

NodePtr Light::GetReference () const { return m_reference; }

Mat4 Light::GetMappingMatrix (StatePtr st, const std::string& lightingSpace) const
{
  if (lightingSpace != "world" && lightingSpace != "camera")
    Error::Fatal("Shader lighting space must be 'world' or 'camera'");

  if (m_space == "local") {
    if (!m_reference) Error::Fatal("Local-space light has no reference node");
    Mat4 model = m_reference->GetModelMatrix();
    return lightingSpace == "world" ? model : Mat4Multiply(st->GetViewMatrix(), model);
  }

  if (m_space == lightingSpace) return Mat4Identity();
  if (m_space == "world") return st->GetViewMatrix();
  return st->GetInverseViewMatrix();
}

void Light::WriteFields (UniformBlock& block, StatePtr st, const std::string& lightingSpace) const
{
  Mat4 matrix = GetMappingMatrix(st, lightingSpace);
  std::map<std::string, UniformValue> values = m_values;
  for (auto& [name, value] : Map(matrix)) values[name] = value;
  for (auto& [name, value] : values)
    if (block.HasField(name)) block.Set(name, value);
}
