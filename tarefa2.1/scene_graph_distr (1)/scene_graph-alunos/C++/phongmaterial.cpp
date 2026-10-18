#include "phongmaterial.h"

PhongMaterial::PhongMaterial (float r, float g, float b, float opacity)
{
  Set("base_color", Vec3{r, g, b});
  Set("opacity", opacity);
  Set("specular_color", Vec3{1.0f, 1.0f, 1.0f});
  Set("shininess", 32.0f);
}

PhongMaterialPtr PhongMaterial::Make (float r, float g, float b, float opacity)
{
  return PhongMaterialPtr(new PhongMaterial(r, g, b, opacity));
}

void PhongMaterial::SetBaseColor (float r, float g, float b) { Set("base_color", Vec3{r, g, b}); }
void PhongMaterial::SetSpecular (float r, float g, float b) { Set("specular_color", Vec3{r, g, b}); }
void PhongMaterial::SetShininess (float shininess) { Set("shininess", shininess); }
void PhongMaterial::SetOpacity (float opacity) { Set("opacity", opacity); }
