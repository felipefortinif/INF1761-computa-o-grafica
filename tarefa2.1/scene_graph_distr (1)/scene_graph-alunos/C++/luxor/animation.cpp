#include "animation.h"
#include "error.h"

Animation::Animation (std::initializer_list<MovementPtr> moves)
: m_curr(0), m_moves(moves)
{
  if (m_moves.empty()) Error::Fatal("Animation needs at least one Movement");
}
AnimationPtr Animation::Make (std::initializer_list<MovementPtr> moves)
{
  return AnimationPtr(new Animation(moves));
}
Animation::~Animation ()
{
}

bool Animation::Advance (float dt, bool reverse)
{
  while (dt > 0.0f) {
    int idx = reverse ? (int) m_moves.size() - 1 - m_curr : m_curr;
    std::optional<float> leftover = m_moves[idx]->Advance(dt, reverse);
    if (!leftover) return false;
    dt = *leftover;
    if (++m_curr == (int) m_moves.size()) {
      m_curr = 0;
      return true;
    }
  }
  return false;
}
