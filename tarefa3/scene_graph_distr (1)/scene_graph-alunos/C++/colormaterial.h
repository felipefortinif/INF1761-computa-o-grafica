#include <memory>
class ColorMaterial;
using ColorMaterialPtr = std::shared_ptr<ColorMaterial>;

#ifndef COLORMATERIAL_H
#define COLORMATERIAL_H

#include "material.h"

// Flat, unlit color appearance: an RGB color plus opacity, written into
// the shader's "material" group (e.g. ColorBlock's color/opacity fields).
class ColorMaterial : public Material {
protected:
  ColorMaterial (float r, float g, float b, float opacity);

public:
  static ColorMaterialPtr Make (float r, float g, float b, float opacity = 1.0f);
  void SetColor (float r, float g, float b);
  void SetOpacity (float opacity);
};

#endif
