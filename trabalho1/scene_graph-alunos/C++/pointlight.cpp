#include "pointlight.h"

PointLight::PointLight (float x, float y, float z, const std::string& space) : Light(space)
{
  SetPosition(x, y, z);
}

PointLightPtr PointLight::Make (float x, float y, float z, const std::string& space)
{
  return PointLightPtr(new PointLight(x, y, z, space));
}

void PointLight::SetPosition (float x, float y, float z) { Set("light_position", Vec4{x, y, z, 1.0f}); }

std::map<std::string, UniformValue> PointLight::Map (const Mat4& matrix) const
{
  Vec4 position = std::get<Vec4>(Get("light_position", Vec4{0, 0, 0, 1}));
  return {{"light_position", Mat4MulVec4(matrix, position)}};
}
