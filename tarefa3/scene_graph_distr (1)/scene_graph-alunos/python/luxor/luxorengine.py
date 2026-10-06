from __future__ import annotations

from typing import TYPE_CHECKING

from engine import *
from luxor.linearinterpolator import *
from luxor.cubicinterpolator import *
from luxor.animation import *
from luxor.movement import *
import graphics_math as gm

if TYPE_CHECKING:
  from transform import Transform

class LuxorEngine (Engine):
  """Drives a Luxor node hierarchy's Transforms through its built-in
  animations, implementing Engine's update(dt). Owns two Animations
  (stand, jump), each reused in both directions via the reverse flag;
  self.status tracks the current pose and self.curr_anim the in-progress
  transition."""

  def __init__ (self, trf_all: Transform, trf_base: Transform, trf_haste1: Transform, trf_haste2: Transform, trf_haste3: Transform, trf_cupula: Transform, trf_lampada: Transform) -> None:
    """Store the Luxor hierarchy's Transforms, init pose state (status
    "up"), and build the stand and jump Animations. Engine holds no
    state, so no super().__init__() call is needed."""
    self.reverse: bool = False
    self.head_angle: float = 0.0
    self.status: str = "up"
    self.curr_anim: Animation | None = None
    self.trf_all = trf_all
    self.trf_base = trf_base
    self.trf_haste1 = trf_haste1
    self.trf_haste2 = trf_haste2
    self.trf_haste3 = trf_haste3
    self.trf_cupula = trf_cupula
    self.trf_lampada = trf_lampada
    self.create_stand_down_animation()
    self.create_jump_forward_animation()

  def create_stand_down_animation (self) -> None:
    """Build the up<->down pose transition (self.stand_down_anim), played
    forward for stand_down and reversed for stand_up."""
    move = Movement(0.5)
    move.add_rotation(self.trf_haste1,
                     LinearInterpolator(gm.vec3(0.0,0.0,0.0),
                                        gm.vec3(-30.0,0.0,0.0)
                                       )
                   )
    move.add_rotation(self.trf_haste2,
                      LinearInterpolator(gm.vec3(0.0,0.0,0.0),
                                         gm.vec3(120.0,0.0,0.0)
                                        )
                    )
    move.add_rotation(self.trf_haste3,
                      LinearInterpolator(gm.vec3(0.0,0.0,0.0),
                                         gm.vec3(-120.0,0.0,0.0)
                                        )
                    )
    move.add_rotation(self.trf_cupula,
                      LinearInterpolator(gm.vec3(0.0,0.0,0.0),
                                         gm.vec3(30.0,0.0,0.0)
                                        )
                    )
    self.stand_down_anim = Animation([move])

  def create_jump_forward_animation (self) -> None:
    """Build the 4-part jump animation (crouch, launch, land, settle) as
    self.jump_forward_anim. Starts/ends in the "down" pose so it can also
    play reversed for jump_backward."""
    # first move: take position to jump
    move1 = Movement(0.3)
    move1.add_rotation(self.trf_haste1, 
                      LinearInterpolator(gm.vec3(-30.0,0.0,0.0),
                                         gm.vec3(-40.0,0.0,0.0)
                                        )
                      )
    move1.add_rotation(self.trf_haste2, 
                      LinearInterpolator(gm.vec3(120.0,0.0,0.0),
                                         gm.vec3(150.0,0.0,0.0)
                                        )
                    )
    move1.add_rotation(self.trf_haste3, 
                      LinearInterpolator(gm.vec3(-120.0,0.0,0.0),
                                         gm.vec3(-145.0,0.0,0.0)
                                        )
                    )
    move1.add_rotation(self.trf_cupula, 
                      LinearInterpolator(gm.vec3(30.0,0.0,0.0),
                                         gm.vec3(60.0,0.0,0.0)
                                        )
                    )
    # second move: jump to the top
    move2 = Movement(0.5)
    move2.add_translation(self.trf_all, 
                          CubicInterpolator(gm.vec3(0.0,0.0,0.0),
                                            gm.vec3(0.0,1.0,1.0),
                                            gm.vec3(0.0,30.0,50.0),
                                            gm.vec3(0.0,0.0,100.0)
                                           )
                          )
    move2.add_rotation(self.trf_base, 
                      LinearInterpolator(gm.vec3(0.0,0.0,0.0),
                                         gm.vec3(-30.0,0.0,0.0)
                                        )
                      )
    move2.add_rotation(self.trf_haste1, 
                      LinearInterpolator(gm.vec3(-40.0,0.0,0.0),
                                         gm.vec3(10.0,0.0,0.0)
                                        )
                      )
    move2.add_rotation(self.trf_haste2, 
                      LinearInterpolator(gm.vec3(150.0,0.0,0.0),
                                         gm.vec3(50.0,0.0,0.0)
                                        )
                    )
    move2.add_rotation(self.trf_haste3, 
                      LinearInterpolator(gm.vec3(-145.0,0.0,0.0),
                                         gm.vec3(-50.0,0.0,0.0)
                                        )
                    )
    move2.add_rotation(self.trf_cupula, 
                      LinearInterpolator(gm.vec3(60.0,0.0,0.0),
                                         gm.vec3(65.0,0.0,0.0)
                                        )
                    )
    # third move: from top to landing
    move3 = Movement(0.5)
    move3.add_translation(self.trf_all, 
                          CubicInterpolator(gm.vec3(0.0,30.0,50.0),
                                            gm.vec3(0.0,0.0,100.0),
                                            gm.vec3(0.0,0.0,90.0),
                                            gm.vec3(0.0,-1.0,1.0)
                                           )
                          )
    move3.add_rotation(self.trf_base, 
                      LinearInterpolator(gm.vec3(-30.0,0.0,0.0),
                                         gm.vec3(0.0,0.0,0.0)
                                        )
                      )
    move3.add_rotation(self.trf_haste1, 
                      LinearInterpolator(gm.vec3(10.0,0.0,0.0),
                                         gm.vec3(-60.0,0.0,0.0)
                                        )
                      )
    move3.add_rotation(self.trf_haste2, 
                      LinearInterpolator(gm.vec3(50.0,0.0,0.0),
                                         gm.vec3(160.0,0.0,0.0)
                                        )
                    )
    move3.add_rotation(self.trf_haste3, 
                      LinearInterpolator(gm.vec3(-50.0,0.0,0.0),
                                         gm.vec3(-165.0,0.0,0.0)
                                        )
                    )
    move3.add_rotation(self.trf_cupula, 
                      LinearInterpolator(gm.vec3(65.0,0.0,0.0),
                                         gm.vec3(60.0,0.0,0.0)
                                        )
                    )
    # forth move: from landing to resting
    move4 = Movement(0.3)
    move4.add_rotation(self.trf_haste1, 
                      LinearInterpolator(gm.vec3(-60.0,0.0,0.0),
                                         gm.vec3(-30.0,0.0,0.0)
                                        )
                      )
    move4.add_rotation(self.trf_haste2, 
                      LinearInterpolator(gm.vec3(160.0,0.0,0.0),
                                         gm.vec3(120.0,0.0,0.0)
                                        )
                    )
    move4.add_rotation(self.trf_haste3, 
                      LinearInterpolator(gm.vec3(-165.0,0.0,0.0),
                                         gm.vec3(-120.0,0.0,0.0)
                                        )
                    )
    move4.add_rotation(self.trf_cupula, 
                      LinearInterpolator(gm.vec3(60.0,0.0,0.0),
                                         gm.vec3(30.0,0.0,0.0)
                                        )
                    )
    self.jump_forward_anim = Animation([move1,move2,move3,move4])

  def stand_up (self) -> bool:
    """Play the stand-down animation in reverse to raise the lamp. No-op
    (False) unless the lamp is "down" and idle."""
    if self.curr_anim or self.status != "down":
      return False
    self.curr_anim = self.stand_down_anim
    self.reverse = True
    self.status = "up"   # next status
    return True

  def stand_down (self) -> bool:
    """Play the stand-down animation forward to lower the lamp. No-op
    (False) unless the lamp is "up" and idle."""
    if self.curr_anim or self.status != "up":
      return False
    self.curr_anim = self.stand_down_anim
    self.reverse = False
    self.status = "down"     # next status
    return True

  def jump_forward (self) -> bool:
    """Play the jump animation forward. No-op (False) unless the lamp is
    "down" and idle; stays "down" after completing."""
    if self.curr_anim or self.status != "down":
      return False
    self.curr_anim = self.jump_forward_anim
    self.reverse = False
    self.status = "down"   # next status
    return True

  def jump_backward (self) -> bool:
    """Same as jump_forward but reversed."""
    if self.curr_anim or self.status != "down":
      return False
    self.curr_anim = self.jump_forward_anim
    self.reverse = True
    self.status = "down"   # next status
    return True

  def turn_head (self, angle: float) -> None:
    """Rotate the head by angle degrees around Y, accumulating into
    self.head_angle."""
    self.trf_cupula.rotate(angle,0.0,1.0,0.0)
    self.head_angle += angle

  def update (self, dt: float) -> None:
    """Advance the in-progress animation, if any, by dt seconds; clear it
    on completion."""
    if (self.curr_anim):
      if (self.curr_anim.advance(dt,self.reverse)):
        self.curr_anim = None
