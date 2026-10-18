#include <memory>
class Animation;
using AnimationPtr = std::shared_ptr<Animation>;

#ifndef ANIMATION_H
#define ANIMATION_H

#include "movement.h"

// Plays a list of Movements in sequence, never blended.
class Animation {
  int m_curr;   // current movement
  std::vector<MovementPtr> m_moves;
protected:
  Animation (std::initializer_list<MovementPtr> moves);
public:
  static AnimationPtr Make (std::initializer_list<MovementPtr> moves);
  virtual ~Animation ();

  // Returns true once the whole sequence has completed. A single large
  // dt can cascade through several Movements in one call (each
  // Movement's leftover time feeds into the next).
  bool Advance (float dt, bool reverse=false);
};

#endif
