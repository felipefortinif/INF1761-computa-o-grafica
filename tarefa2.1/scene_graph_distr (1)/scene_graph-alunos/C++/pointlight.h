#include <memory>
class PointLight;
using PointLightPtr = std::shared_ptr<PointLight>;

#ifndef POINTLIGHT_H
#define POINTLIGHT_H

#include "light.h"

// Point light whose position is translated by coordinate mappings.
class PointLight : public Light {
protected:
  PointLight (float x, float y, float z, const std::string& space);

public:
  static PointLightPtr Make (float x, float y, float z, const std::string& space = "world");
  void SetPosition (float x, float y, float z);
  std::map<std::string, UniformValue> Map (const Mat4& matrix) const override;
};

#endif
