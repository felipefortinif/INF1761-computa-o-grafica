from __future__ import annotations

from typing import TYPE_CHECKING

if TYPE_CHECKING:
  from luxor.movement import Movement

class Animation:
  """A sequence of Movements played one after another, never blended.
  reverse plays the sequence back to front, letting one Animation serve
  both directions of a transition."""

  def __init__ (self, moves: list[Movement]) -> None:
    """Copy the Movement sequence to play, starting at index 0. Raises
    if `moves` is empty - there would be nothing for advance() to play."""
    if not moves:
      raise ValueError("Animation needs at least one Movement")
    self.curr: int = 0
    self.moves = moves.copy()

  def advance (self, dt: float, reverse: bool = False) -> bool:
    """Advance the current Movement by dt seconds, moving to the next
    one when it finishes - looping through as many subsequent Movements
    as needed so a large dt (e.g. after a stall) is never silently
    dropped, only fully consumed. Returns True if the whole sequence
    completed at least once during this call (index wraps to 0)."""
    completed = False
    while dt > 0:
      idx = (len(self.moves)-1) - self.curr if reverse else self.curr
      leftover = self.moves[idx].advance(dt, reverse)
      if leftover is None:
        break
      dt = leftover
      self.curr += 1
      if self.curr == len(self.moves):
        self.curr = 0
        completed = True
    return completed
