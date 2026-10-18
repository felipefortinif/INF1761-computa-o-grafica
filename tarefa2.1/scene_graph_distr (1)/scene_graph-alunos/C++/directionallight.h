#include <memory>
class DirectionalLight;
using DirectionalLightPtr = std::shared_ptr<DirectionalLight>;

#ifndef DIRECTIONALLIGHT_H
#define DIRECTIONALLIGHT_H

#include "light.h"

// Directional light whose direction ignores translation and is normalized.
class DirectionalLight : public Light {
protected:
  DirectionalLight (float x, float y, float z, const std::string& space);

public:
  static DirectionalLightPtr Make (float x, float y, float z, const std::string& space = "world");
  void SetDirection (float x, float y, float z);
  std::map<std::string, UniformValue> Map (const Mat4& matrix) const override;
};

#endif
