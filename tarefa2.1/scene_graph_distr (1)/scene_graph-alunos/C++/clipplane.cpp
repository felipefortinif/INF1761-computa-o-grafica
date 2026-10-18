#include "clipplane.h"

ClipPlane::ClipPlane (ShaderPtr shader, float a, float b, float c, float d) : m_shader(shader)
{
  SetPlane(a, b, c, d);
  SetColor(0.5f, 0.5f, 0.5f);
}

void ClipPlane::SetPlane (float a, float b, float c, float d)
{
  m_shader->SetValue("clip_plane", Vec4{a, b, c, d});
}

void ClipPlane::SetColor (float r, float g, float b)
{
  m_shader->SetValue("clip_plane_color", Vec4{r, g, b, 1.0f});
}
