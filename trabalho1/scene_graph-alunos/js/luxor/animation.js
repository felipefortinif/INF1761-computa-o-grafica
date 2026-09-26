// A sequence of Movements played one after another, never blended.
// reverse plays the sequence back to front, letting one Animation serve
// both directions of a transition. Ported from python/luxor/animation.py.

export class Animation {
  // Copies the Movement sequence to play, starting at index 0. Throws if
  // `moves` is empty - there would be nothing for advance() to play.
  constructor (moves) {
    if (moves.length === 0) throw new Error("Animation needs at least one Movement");
    this.curr = 0;
    this.moves = [...moves];
  }

  // Advances the current Movement by dt seconds, moving to the next one
  // when it finishes - looping through as many subsequent Movements as
  // needed so a large dt (e.g. after a stall) is never silently dropped,
  // only fully consumed. Returns true if the whole sequence completed at
  // least once during this call (index wraps to 0).
  advance (dt, reverse = false) {
    let completed = false;
    while (dt > 0) {
      const idx = reverse ? (this.moves.length - 1) - this.curr : this.curr;
      const leftover = this.moves[idx].advance(dt, reverse);
      if (leftover === null) break;
      dt = leftover;
      this.curr += 1;
      if (this.curr === this.moves.length) {
        this.curr = 0;
        completed = true;
      }
    }
    return completed;
  }
}
