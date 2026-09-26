#include "directionallight.h"

DirectionalLight::DirectionalLight (float x, float y, float z, const std::string& space) : Light(space)
{
  SetDirection(x, y, z);
}

DirectionalLightPtr DirectionalLight::Make (float x, float y, float z, const std::string& space)
{
  return DirectionalLightPtr(new DirectionalLight(x, y, z, space));
}

void DirectionalLight::SetDirection (float x, float y, float z) { Set("light_direction", Vec3{x, y, z}); }

std::map<std::string, UniformValue> DirectionalLight::Map (const Mat4& matrix) const
{
  Vec3 d = std::get<Vec3>(Get("light_direction", Vec3{0, 0, 1}));
  Vec4 mapped = Mat4MulVec4(matrix, Vec4{d.x, d.y, d.z, 0.0f});
  Vec3 direction = Vec3Normalize(Vec3{mapped.x, mapped.y, mapped.z});
  return {{"light_direction", direction}};
}
