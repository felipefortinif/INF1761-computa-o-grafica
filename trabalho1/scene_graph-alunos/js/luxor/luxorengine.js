// Drives a Luxor node hierarchy's Transforms through its built-in
// animations, implementing Engine's update(dt). Owns two Animations
// (stand, jump), each reused in both directions via the reverse flag;
// self.status tracks the current pose and self.currAnim the in-progress
// transition. Ported from python/luxor/luxorengine.py.

import * as gm from "../graphicsmath.js";
import { Engine } from "../engine.js";
import { LinearInterpolator } from "./linearinterpolator.js";
import { CubicInterpolator } from "./cubicinterpolator.js";
import { Animation } from "./animation.js";
import { Movement } from "./movement.js";

export class LuxorEngine extends Engine {
  // Stores the Luxor hierarchy's Transforms, inits pose state (status
  // "up"), and builds the stand and jump Animations.
  constructor (trfAll, trfBase, trfHaste1, trfHaste2, trfHaste3, trfCupula, trfLampada) {
    super();
    this.reverse = false;
    this.headAngle = 0.0;
    this.status = "up";
    this.currAnim = null;
    this.trfAll = trfAll;
    this.trfBase = trfBase;
    this.trfHaste1 = trfHaste1;
    this.trfHaste2 = trfHaste2;
    this.trfHaste3 = trfHaste3;
    this.trfCupula = trfCupula;
    this.trfLampada = trfLampada;
    this.createStandDownAnimation();
    this.createJumpForwardAnimation();
  }

  // Builds the up<->down pose transition (this.standDownAnim), played
  // forward for standDown and reversed for standUp.
  createStandDownAnimation () {
    const move = new Movement(0.5);
    move.addRotation(this.trfHaste1, new LinearInterpolator(gm.vec3(0.0, 0.0, 0.0), gm.vec3(-30.0, 0.0, 0.0)));
    move.addRotation(this.trfHaste2, new LinearInterpolator(gm.vec3(0.0, 0.0, 0.0), gm.vec3(120.0, 0.0, 0.0)));
    move.addRotation(this.trfHaste3, new LinearInterpolator(gm.vec3(0.0, 0.0, 0.0), gm.vec3(-120.0, 0.0, 0.0)));
    move.addRotation(this.trfCupula, new LinearInterpolator(gm.vec3(0.0, 0.0, 0.0), gm.vec3(30.0, 0.0, 0.0)));
    this.standDownAnim = new Animation([move]);
  }

  // Builds the 4-part jump animation (crouch, launch, land, settle) as
  // this.jumpForwardAnim. Starts/ends in the "down" pose so it can also
  // play reversed for jumpBackward.
  createJumpForwardAnimation () {
    // first move: take position to jump
    const move1 = new Movement(0.3);
    move1.addRotation(this.trfHaste1, new LinearInterpolator(gm.vec3(-30.0, 0.0, 0.0), gm.vec3(-40.0, 0.0, 0.0)));
    move1.addRotation(this.trfHaste2, new LinearInterpolator(gm.vec3(120.0, 0.0, 0.0), gm.vec3(150.0, 0.0, 0.0)));
    move1.addRotation(this.trfHaste3, new LinearInterpolator(gm.vec3(-120.0, 0.0, 0.0), gm.vec3(-145.0, 0.0, 0.0)));
    move1.addRotation(this.trfCupula, new LinearInterpolator(gm.vec3(30.0, 0.0, 0.0), gm.vec3(60.0, 0.0, 0.0)));

    // second move: jump to the top
    const move2 = new Movement(0.5);
    move2.addTranslation(this.trfAll, new CubicInterpolator(
      gm.vec3(0.0, 0.0, 0.0), gm.vec3(0.0, 1.0, 1.0), gm.vec3(0.0, 30.0, 50.0), gm.vec3(0.0, 0.0, 100.0),
    ));
    move2.addRotation(this.trfBase, new LinearInterpolator(gm.vec3(0.0, 0.0, 0.0), gm.vec3(-30.0, 0.0, 0.0)));
    move2.addRotation(this.trfHaste1, new LinearInterpolator(gm.vec3(-40.0, 0.0, 0.0), gm.vec3(10.0, 0.0, 0.0)));
    move2.addRotation(this.trfHaste2, new LinearInterpolator(gm.vec3(150.0, 0.0, 0.0), gm.vec3(50.0, 0.0, 0.0)));
    move2.addRotation(this.trfHaste3, new LinearInterpolator(gm.vec3(-145.0, 0.0, 0.0), gm.vec3(-50.0, 0.0, 0.0)));
    move2.addRotation(this.trfCupula, new LinearInterpolator(gm.vec3(60.0, 0.0, 0.0), gm.vec3(65.0, 0.0, 0.0)));

    // third move: from top to landing
    const move3 = new Movement(0.5);
    move3.addTranslation(this.trfAll, new CubicInterpolator(
      gm.vec3(0.0, 30.0, 50.0), gm.vec3(0.0, 0.0, 100.0), gm.vec3(0.0, 0.0, 90.0), gm.vec3(0.0, -1.0, 1.0),
    ));
    move3.addRotation(this.trfBase, new LinearInterpolator(gm.vec3(-30.0, 0.0, 0.0), gm.vec3(0.0, 0.0, 0.0)));
    move3.addRotation(this.trfHaste1, new LinearInterpolator(gm.vec3(10.0, 0.0, 0.0), gm.vec3(-60.0, 0.0, 0.0)));
    move3.addRotation(this.trfHaste2, new LinearInterpolator(gm.vec3(50.0, 0.0, 0.0), gm.vec3(160.0, 0.0, 0.0)));
    move3.addRotation(this.trfHaste3, new LinearInterpolator(gm.vec3(-50.0, 0.0, 0.0), gm.vec3(-165.0, 0.0, 0.0)));
    move3.addRotation(this.trfCupula, new LinearInterpolator(gm.vec3(65.0, 0.0, 0.0), gm.vec3(60.0, 0.0, 0.0)));

    // fourth move: from landing to resting
    const move4 = new Movement(0.3);
    move4.addRotation(this.trfHaste1, new LinearInterpolator(gm.vec3(-60.0, 0.0, 0.0), gm.vec3(-30.0, 0.0, 0.0)));
    move4.addRotation(this.trfHaste2, new LinearInterpolator(gm.vec3(160.0, 0.0, 0.0), gm.vec3(120.0, 0.0, 0.0)));
    move4.addRotation(this.trfHaste3, new LinearInterpolator(gm.vec3(-165.0, 0.0, 0.0), gm.vec3(-120.0, 0.0, 0.0)));
    move4.addRotation(this.trfCupula, new LinearInterpolator(gm.vec3(60.0, 0.0, 0.0), gm.vec3(30.0, 0.0, 0.0)));

    this.jumpForwardAnim = new Animation([move1, move2, move3, move4]);
  }

  // Plays the stand-down animation in reverse to raise the lamp. No-op
  // (false) unless the lamp is "down" and idle.
  standUp () {
    if (this.currAnim || this.status !== "down") return false;
    this.currAnim = this.standDownAnim;
    this.reverse = true;
    this.status = "up"; // next status
    return true;
  }

  // Plays the stand-down animation forward to lower the lamp. No-op
  // (false) unless the lamp is "up" and idle.
  standDown () {
    if (this.currAnim || this.status !== "up") return false;
    this.currAnim = this.standDownAnim;
    this.reverse = false;
    this.status = "down"; // next status
    return true;
  }

  // Plays the jump animation forward. No-op (false) unless the lamp is
  // "down" and idle; stays "down" after completing.
  jumpForward () {
    if (this.currAnim || this.status !== "down") return false;
    this.currAnim = this.jumpForwardAnim;
    this.reverse = false;
    this.status = "down"; // next status
    return true;
  }

  // Same as jumpForward but reversed.
  jumpBackward () {
    if (this.currAnim || this.status !== "down") return false;
    this.currAnim = this.jumpForwardAnim;
    this.reverse = true;
    this.status = "down"; // next status
    return true;
  }

  // Rotates the head by angle degrees around Y, accumulating into headAngle.
  turnHead (angle) {
    this.trfCupula.rotate(angle, 0.0, 1.0, 0.0);
    this.headAngle += angle;
  }

  // Advances the in-progress animation, if any, by dt seconds; clears it
  // on completion.
  update (dt) {
    if (this.currAnim) {
      if (this.currAnim.advance(dt, this.reverse)) this.currAnim = null;
    }
  }
}
