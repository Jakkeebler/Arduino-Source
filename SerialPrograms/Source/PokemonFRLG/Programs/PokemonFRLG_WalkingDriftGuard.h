/*  Pokemon FRLG Walking Drift Guard
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Pure logic, no I/O. Two independent guards used by the walking stack
 *  (kanto_navigate_to and the scripted guarded_walk route paths):
 *
 *  1. Post-move drift verification: after a burst of tile-steps, compare the
 *     predicted landing tile against the actual confirmed position fix. Any
 *     drift (> 0 tiles) should re-localize immediately rather than waiting
 *     for the no-progress / strand-guard path, which only trips after
 *     several further steps of total non-movement. A 1-tile drift never
 *     shows up as "no movement" -- the player DID move, just not exactly as
 *     predicted -- so the old no-progress counter never caught it at all.
 *
 *  2. Collision / blocked-tile detection: distinguishes "walked into an
 *     obstacle, no position change" from a legitimate, expected non-movement
 *     (a direction-change turn, or a step cut short by an encounter/fade),
 *     and decides when to stop repeating the same input and reroute.
 *
 *  See FRO-225 (FRLG Auto-Train Phase 2).
 */

#ifndef PokemonAutomation_PokemonFRLG_WalkingDriftGuard_H
#define PokemonAutomation_PokemonFRLG_WalkingDriftGuard_H

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


//  ---- Post-move drift verification ----

struct DriftCheckResult{
    //  Chebyshev (chessboard) tile distance between the predicted and actual
    //  position. 0 means the burst landed exactly where predicted.
    int drift_tiles = 0;
    //  True whenever drift_tiles > 0: per the Phase 2 acceptance criteria,
    //  ANY drift re-localizes immediately rather than being silently
    //  absorbed as "close enough" by the motion gate's jump tolerance.
    bool should_relocalize = false;
};

//  `predicted_x/y` is where the burst should have ended if every tile of the
//  intended walk actually landed. `actual_x/y` is the tile the detector just
//  confirmed. `interrupted` is true when the step was cut short by a known,
//  expected cause (encounter flee, door fade) -- the predicted tile was never
//  really targeted once the step was interrupted, so that is not drift and
//  must not be reported as such.
DriftCheckResult check_walk_drift(
    int predicted_x, int predicted_y,
    int actual_x, int actual_y,
    bool interrupted
);

//  Whether a periodic drift audit is due. Only matters when a caller wants to
//  check less often than every poll (the issue's "after every N tile-steps");
//  passing interval <= 1 means "check every time", which is the safest
//  default and what kanto_navigate_to actually uses.
bool drift_check_due(int steps_since_last_check, int interval);


//  ---- Collision / blocked-tile detection ----

enum class CollisionDecision{
    NotACollision,   //  Expected non-movement (turn or interruption). Not a stall.
    KeepTrying,      //  Unexplained non-movement, but under the reroute threshold.
    Reroute,         //  Threshold reached: treat the tile ahead as blocked and reroute.
};

class CollisionGuard{
public:
    //  attempts_before_reroute: consecutive UNEXPLAINED no-moves before
    //  Reroute. Default 2 per the Phase 2 acceptance criteria ("reroutes
    //  within 2 attempts"), intentionally tighter than the pre-Phase-2
    //  no-progress counter (which allowed 3 before learning an obstacle).
    explicit CollisionGuard(int attempts_before_reroute = 2);

    //  Report one step's outcome.
    //    moved            - whether the confirmed tile actually changed.
    //    expected_no_move - true for a turn-in-place press or a step whose
    //                       move was interrupted (encounter/fade): both
    //                       produce a legitimate "didn't move" that must
    //                       never count toward collision.
    CollisionDecision report(bool moved, bool expected_no_move);

    int consecutive_unexplained_stalls() const{ return m_count; }
    void reset(){ m_count = 0; }

private:
    int m_threshold;
    int m_count = 0;
};


}
}
}
#endif
