#include <memory>
class PhongMaterial;
using PhongMaterialPtr = std::shared_ptr<PhongMaterial>;

#ifndef PHONGMATERIAL_H
#define PHONGMATERIAL_H

#include "material.h"

// Blinn-Phong surface appearance: a diffuse base color, a specular
// color, shininess exponent, and opacity. The ambient term isn't a
// material field - shaders source it straight from the "global" group's
// light.
class PhongMaterial : public Material {
protected:
  PhongMaterial (float r, float g, float b, float opacity);

public:
  static PhongMaterialPtr Make (float r, float g, float b, float opacity = 1.0f);
  void SetBaseColor (float r, float g, float b);
  void SetSpecular (float r, float g, float b);
  void SetShininess (float shininess);
  void SetOpacity (float opacity);
};

#endif
