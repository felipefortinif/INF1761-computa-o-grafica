#ifndef CLIPPLANE_H
#define CLIPPLANE_H

#include "shader.h"

// Emulates GLSL's gl_ClipDistance (no WGSL equivalent) via a per-fragment
// discard in the shader. Sets "clip_plane" (ax+by+cz+d=0) and
// "clip_plane_color" (cut cross-section color) directly on `shader` - a
// "global" group field, fatal if the shader's "global" struct doesn't
// declare them (see Shader::SetValue/CommitGlobal).
class ClipPlane {
  ShaderPtr m_shader;
public:
  ClipPlane (ShaderPtr shader, float a, float b, float c, float d);
  void SetPlane (float a, float b, float c, float d);
  void SetColor (float r, float g, float b);
};

#endif
