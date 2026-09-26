#include "luxorengine.h"
#include "linearinterpolator.h"
#include "cubicinterpolator.h"

LuxorEngine::LuxorEngine (
               TransformPtr trf_all,
               TransformPtr trf_base,
               TransformPtr trf_haste1,
               TransformPtr trf_haste2,
               TransformPtr trf_haste3,
               TransformPtr trf_cupula,
               TransformPtr trf_lampada
              )
: m_reverse(false),
  m_head_angle(0.0f),
  m_status("up"),
  m_curr_anim(),
  m_trf_all(trf_all),
  m_trf_base(trf_base),
  m_trf_haste1(trf_haste1),
  m_trf_haste2(trf_haste2),
  m_trf_haste3(trf_haste3),
  m_trf_cupula(trf_cupula),
  m_trf_lampada(trf_lampada)
{
  CreateStandDownAnimation();
  CreateJumpForwardAnimation();
}

LuxorEnginePtr LuxorEngine::Make (
               TransformPtr trf_all,
               TransformPtr trf_base,
               TransformPtr trf_haste1,
               TransformPtr trf_haste2,
               TransformPtr trf_haste3,
               TransformPtr trf_cupula,
               TransformPtr trf_lampada
              )
{
  return LuxorEnginePtr(new LuxorEngine(trf_all,
                                        trf_base,
                                        trf_haste1,
                                        trf_haste2,
                                        trf_haste3,
                                        trf_cupula,
                                        trf_lampada
                                       )
                        );
}

LuxorEngine::~LuxorEngine ()
{
}

void LuxorEngine::CreateStandDownAnimation ()
{
  MovementPtr move = Movement::Make(0.5f);
  move->AddRotation(m_trf_haste1,
                    LinearInterpolator::Make(Vec3{0.0f,0.0f,0.0f},
                                             Vec3{-30.0f,0.0f,0.0f}
                                            )
                   );
  move->AddRotation(m_trf_haste2,
                    LinearInterpolator::Make(Vec3{0.0f,0.0f,0.0f},
                                             Vec3{120.0f,0.0f,0.0f}
                                            )
                   );
  move->AddRotation(m_trf_haste3,
                    LinearInterpolator::Make(Vec3{0.0f,0.0f,0.0f},
                                             Vec3{-120.0f,0.0f,0.0f}
                                            )
                   );
  move->AddRotation(m_trf_cupula,
                    LinearInterpolator::Make(Vec3{0.0f,0.0f,0.0f},
                                             Vec3{30.0f,0.0f,0.0f}
                                            )
                   );
  m_stand_down_anim = Animation::Make({move});
}

void LuxorEngine::CreateJumpForwardAnimation ()
{
  // first move: take position to jump
  MovementPtr move1 = Movement::Make(0.3f);
  move1->AddRotation(m_trf_haste1,
                     LinearInterpolator::Make(Vec3{-30.0f,0.0f,0.0f},
                                              Vec3{-40.0f,0.0f,0.0f}
                                             )
                    );
  move1->AddRotation(m_trf_haste2,
                     LinearInterpolator::Make(Vec3{120.0f,0.0f,0.0f},
                                              Vec3{150.0f,0.0f,0.0f}
                                             )
                   );
  move1->AddRotation(m_trf_haste3,
                     LinearInterpolator::Make(Vec3{-120.0f,0.0f,0.0f},
                                              Vec3{-145.0f,0.0f,0.0f}
                                             )
                   );
  move1->AddRotation(m_trf_cupula,
                     LinearInterpolator::Make(Vec3{30.0f,0.0f,0.0f},
                                              Vec3{60.0f,0.0f,0.0f}
                                             )
                   );
  // second move: jump to the top
  MovementPtr move2 = Movement::Make(0.5f);
  move2->AddTranslation(m_trf_all,
                        CubicInterpolator::Make(Vec3{0.0f,0.0f,0.0f},
                                                Vec3{0.0f,1.0f,1.0f},
                                                Vec3{0.0f,30.0f,50.0f},
                                                Vec3{0.0f,0.0f,100.0f}
                                                )
                        );
  move2->AddRotation(m_trf_base,
                     LinearInterpolator::Make(Vec3{0.0f,0.0f,0.0f},
                                              Vec3{-30.0f,0.0f,0.0f}
                                             )
                    );
  move2->AddRotation(m_trf_haste1,
                     LinearInterpolator::Make(Vec3{-40.0f,0.0f,0.0f},
                                              Vec3{10.0f,0.0f,0.0f}
                                             )
                    );
  move2->AddRotation(m_trf_haste2,
                     LinearInterpolator::Make(Vec3{150.0f,0.0f,0.0f},
                                              Vec3{50.0f,0.0f,0.0f}
                                             )
                   );
  move2->AddRotation(m_trf_haste3,
                     LinearInterpolator::Make(Vec3{-145.0f,0.0f,0.0f},
                                              Vec3{-50.0f,0.0f,0.0f}
                                             )
                   );
  move2->AddRotation(m_trf_cupula,
                     LinearInterpolator::Make(Vec3{60.0f,0.0f,0.0f},
                                              Vec3{65.0f,0.0f,0.0f}
                                             )
                   );
  // third move: from top to landing
  MovementPtr move3 = Movement::Make(0.5f);
  move3->AddTranslation(m_trf_all,
                        CubicInterpolator::Make(Vec3{0.0f,30.0f,50.0f},
                                                Vec3{0.0f,0.0f,100.0f},
                                                Vec3{0.0f,0.0f,90.0f},
                                                Vec3{0.0f,-1.0f,1.0f}
                                                )
                        );
  move3->AddRotation(m_trf_base,
                     LinearInterpolator::Make(Vec3{-30.0f,0.0f,0.0f},
                                              Vec3{0.0f,0.0f,0.0f}
                                             )
                    );
  move3->AddRotation(m_trf_haste1,
                     LinearInterpolator::Make(Vec3{10.0f,0.0f,0.0f},
                                              Vec3{-60.0f,0.0f,0.0f}
                                             )
                    );
  move3->AddRotation(m_trf_haste2,
                     LinearInterpolator::Make(Vec3{50.0f,0.0f,0.0f},
                                              Vec3{160.0f,0.0f,0.0f}
                                             )
                   );
  move3->AddRotation(m_trf_haste3,
                     LinearInterpolator::Make(Vec3{-50.0f,0.0f,0.0f},
                                              Vec3{-165.0f,0.0f,0.0f}
                                             )
                   );
  move3->AddRotation(m_trf_cupula,
                     LinearInterpolator::Make(Vec3{65.0f,0.0f,0.0f},
                                              Vec3{60.0f,0.0f,0.0f}
                                             )
                   );
  // forth move: from landing to resting
  MovementPtr move4 = Movement::Make(0.3f);
  move4->AddRotation(m_trf_haste1,
                     LinearInterpolator::Make(Vec3{-60.0f,0.0f,0.0f},
                                              Vec3{-30.0f,0.0f,0.0f}
                                             )
                    );
  move4->AddRotation(m_trf_haste2,
                     LinearInterpolator::Make(Vec3{160.0f,0.0f,0.0f},
                                              Vec3{120.0f,0.0f,0.0f}
                                             )
                   );
  move4->AddRotation(m_trf_haste3,
                     LinearInterpolator::Make(Vec3{-165.0f,0.0f,0.0f},
                                              Vec3{-120.0f,0.0f,0.0f}
                                             )
                   );
  move4->AddRotation(m_trf_cupula,
                     LinearInterpolator::Make(Vec3{60.0f,0.0f,0.0f},
                                              Vec3{30.0f,0.0f,0.0f}
                                             )
                   );
  m_jump_forward_anim = Animation::Make({move1,move2,move3,move4});
}

bool LuxorEngine::StandUp ()
{
  if (m_curr_anim || m_status != "down")
    return false;
  m_curr_anim = m_stand_down_anim;
  m_reverse = true;
  m_status = "up";   // next status
  return true;
}

bool LuxorEngine::StandDown ()
{
  if (m_curr_anim || m_status != "up")
    return false;
  m_curr_anim = m_stand_down_anim;
  m_reverse = false;
  m_status = "down";     // next status
  return true;
}

bool LuxorEngine::JumpForward ()
{
  if (m_curr_anim || m_status != "down")
    return false;
  m_curr_anim = m_jump_forward_anim;
  m_reverse = false;
  m_status = "down";   // next status
  return true;
}

bool LuxorEngine::JumpBackward ()
{
  if (m_curr_anim || m_status != "down")
    return false;
  m_curr_anim = m_jump_forward_anim;
  m_reverse = true;
  m_status = "down";   // next status
  return true;
}

void LuxorEngine::TurnHead (float angle)
{
  m_trf_cupula->Rotate(angle,0.0f,1.0f,0.0f);
  m_head_angle += angle;
}

void LuxorEngine::Update (float dt)
{
  if (m_curr_anim) {
    if (m_curr_anim->Advance(dt,m_reverse)) {
      m_curr_anim = nullptr;
    }
  }
}
