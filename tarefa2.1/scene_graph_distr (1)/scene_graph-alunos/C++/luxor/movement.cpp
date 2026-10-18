#include "movement.h"
#include "error.h"

Movement::Movement (float T)
: m_t(0.0f), m_T(T)
{
  if (T <= 0.0f) Error::Fatal("Movement duration T must be > 0");
}

MovementPtr Movement::Make (float T)
{
  return MovementPtr(new Movement(T));
}

Movement::~Movement ()
{
}

void Movement::AddTranslation (TransformPtr trf, InterpolatorPtr interp)
{
  m_trl_trf.push_back(trf);
  m_trl_interp.push_back(interp);
}

void Movement::AddRotation (TransformPtr trf, InterpolatorPtr interp)
{
  m_rot_trf.push_back(trf);
  m_rot_interp.push_back(interp);
}

std::optional<float> Movement::Advance (float dt, bool reverse)
{
  float t = m_t + dt;
  bool finished = t >= m_T;
  float leftover = 0.0f;
  if (finished) {
    leftover = t - m_T;
    t = m_T;
  }
  float t0 = reverse ? (m_T-m_t)/m_T : m_t/m_T;
  float t1 = reverse ? (m_T-t)/m_T : t/m_T;
  // perform translations
  for (size_t i=0; i<m_trl_trf.size(); ++i) {
    Vec3 v0 = m_trl_interp[i]->Interpolate(t0);
    Vec3 v1 = m_trl_interp[i]->Interpolate(t1);
    m_trl_trf[i]->Translate(v1.x-v0.x,v1.y-v0.y,v1.z-v0.z);
  }
  // perform rotations
  for (size_t i=0; i<m_rot_trf.size(); ++i) {
    Vec3 v0 = m_rot_interp[i]->Interpolate(t0);
    Vec3 v1 = m_rot_interp[i]->Interpolate(t1);
    m_rot_trf[i]->Rotate(v1.x-v0.x,1.0f,0.0f,0.0f);
    m_rot_trf[i]->Rotate(v1.y-v0.y,0.0f,1.0f,0.0f);
    m_rot_trf[i]->Rotate(v1.z-v0.z,0.0f,0.0f,1.0f);
  }
  if (finished) {  // check if movement ended
    m_t = 0.0f;    // reset internal clock
    return leftover;
  }
  else {
    m_t = t;
    return std::nullopt;
  }
}
