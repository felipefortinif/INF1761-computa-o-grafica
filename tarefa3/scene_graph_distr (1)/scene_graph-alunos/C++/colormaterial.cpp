#include "colormaterial.h"

ColorMaterial::ColorMaterial (float r, float g, float b, float opacity)
{
  Set("color", Vec3{r, g, b});
  Set("opacity", opacity);
}

ColorMaterialPtr ColorMaterial::Make (float r, float g, float b, float opacity)
{
  return ColorMaterialPtr(new ColorMaterial(r, g, b, opacity));
}

void ColorMaterial::SetColor (float r, float g, float b) { Set("color", Vec3{r, g, b}); }
void ColorMaterial::SetOpacity (float opacity) { Set("opacity", opacity); }
