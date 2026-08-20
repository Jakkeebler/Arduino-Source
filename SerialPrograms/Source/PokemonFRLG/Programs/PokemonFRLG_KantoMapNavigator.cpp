/*  Kanto Map Navigator
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <set>
#include "Common/Cpp/Color.h"
#include "CommonFramework/Exceptions/OperationFailedException.h"
#include "CommonFramework/VideoPipeline/VideoFeed.h"
#include "CommonTools/Async/InferenceRoutines.h"
#include "CommonTools/VisualDetectors/BlackScreenDetector.h"
#include "CommonTools/VisualDetectors/FrozenImageDetector.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_PushButtons.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_BattleDialogs.h"
#include "PokemonFRLG/Inference/Map/PokemonFRLG_KantoMapDetector.h"
#include "PokemonFRLG/Inference/Map/PokemonFRLG_KantoMapPathfinder.h"
#include "PokemonFRLG/PokemonFRLG_Navigation.h"
#include "PokemonFRLG_KantoMapNavigator.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

namespace{

struct Step{
    double jx;
    double jy;
    const char* name;
};
constexpr Step STEP_NORTH{0.0, +1.0, "north"};
constexpr Step STEP_SOUTH{0.0, -1.0, "south"};
constexpr Step STEP_EAST {+1.0,  0.0, "east"};
constexpr Step STEP_WEST {-1.0,  0.0, "west"};

//  Wild encounters during travel are routine, not a failure. Routes run through
//  grass, and a heal trip across Route 1 or Route 22 will legitimately trigger
//  a dozen of them -- the old cap of 5 killed runs for doing exactly the right
//  thing.
//
//  What actually matters is whether we are still getting closer to the goal. So
//  the counter resets on every new closest approach (see best_distance below)
//  and this cap only trips when we have fled that many times WITHOUT making any
//  progress, which is a real stall: a battle we cannot escape, or an encounter
//  loop on a tile we cannot leave.
constexpr int MAX_FLEES_WITHOUT_PROGRESS = 12;
constexpr int MAX_UNKNOWN_POLLS = 15;
constexpr int MAX_NO_PROGRESS = 8;
constexpr auto STEP_HOLD = std::chrono::milliseconds(150);
constexpr auto STEP_RELEASE = std::chrono::milliseconds(80);
constexpr double MIN_CONFIDENCE = 0.40;

//  Hard floor for the void-compensated confidence threshold. Even a viewport
//  that is mostly blank must clear this, or a fade or a menu would start
//  matching. See MIN_CONFIDENCE compensation in the poll loop below.
constexpr double MIN_CONFIDENCE_FLOOR = 0.20;

//  Failed hinted polls before abandoning the hint for a cold full-map fix.
//  Small on purpose: a correct hint resolves on the first or second poll, so
//  anything beyond that is a hint we should stop trusting. A cold fix costs a
//  couple of seconds; refusing to take one costs the whole run.
constexpr int HINT_GIVEUP_POLLS = 3;

//  A Gen-3 walk cycle is ~16 frames (~267 ms) and the camera keeps scrolling
//  briefly after the stick is released. Capturing before the scroll settles
//  leaves the viewport offset by up to half a tile, which is exactly the error
//  the detector's quantization has the least tolerance for. Wait this long
//  after every step before trusting the next frame.
constexpr auto STEP_SETTLE = std::chrono::milliseconds(260);

//  Search window around the previous position, in tiles. One step moves at most
//  one tile, so this only has to cover a step plus a little slack. The old value
//  of 60 opened a 120x120-tile window, which is enough repetitive terrain
//  (grass, trees, fences) for matchTemplate to find a confident look-alike far
//  from the player -- and because the result then became the next poll's hint,
//  a single bad match locked navigation onto the wrong part of the map for good.
constexpr int HINT_RADIUS_TILES = 4;

//  Reject any detection further than this from the previous confirmed position.
//  Physically impossible: we issue one step per poll. Rejecting instead of
//  accepting is what stops a bad match from poisoning the hint chain.
constexpr int MAX_TILE_JUMP = 2;

//  How many implausible detections in a row before we stop trusting the hint and
//  fall back to a cold full-map fix. Not 1: a cold fix is the only kind subject to
//  the detector's ambiguity gate, so in repetitive terrain it is *less* likely to
//  succeed than a hinted one. Give the hint a couple of retries first -- a single
//  frame captured mid-scroll should not throw away a good position.
constexpr int MAX_REJECTED_JUMPS = 3;

//  How many consecutive A* failures to treat as "we are mis-localized" before
//  accepting that the walkable mask is simply wrong here and walking greedily.
constexpr int MAX_ASTAR_FAILURES_BEFORE_GREEDY = 2;

//  How many steps to spend escaping a start tile the mask calls blocked before
//  giving up. One step should normally be enough -- a doorway is adjacent to
//  open ground by construction. More than a couple means we are inside a region
//  the mask has wrong wholesale, and pressing on just burns the step budget
//  somewhere the map cannot describe.
constexpr int MAX_BLOCKED_START_ESCAPES = 4;

//  Consecutive no-progress polls before we conclude the tile ahead is genuinely
//  not walkable and record it. Well under MAX_NO_PROGRESS on purpose: the point
//  is to re-route while there is still budget left to walk the alternative.
//  Three presses is already past any plausible dropped input.
constexpr int NO_PROGRESS_BEFORE_LEARNING = 3;

//  Cap on obstacles one navigation may learn. A handful means the mask is wrong
//  about a wall or two and routing around them is right. Many more means we are
//  either mis-localized or somewhere the map describes wholesale incorrectly,
//  and blacklisting our way across it would carve real holes in the map for the
//  rest of the session -- better to fail and say so.
constexpr int MAX_LEARNED_BLOCKS_PER_NAVIGATION = 16;

//  How many distinct tiles escape mode may explore before giving up. A ledge
//  pocket on Route 22 is a handful of tiles; thirty means we are wandering.
constexpr int MAX_ESCAPE_TILES = 30;

inline uint32_t tile_key(int x, int y){
    return (uint32_t)(x & 0xFFFF) | ((uint32_t)(y & 0xFFFF) << 16);
}

//  ---- Burst walking -----------------------------------------------------
//
//  Taking a position fix after every single tile cost ~515 ms per tile, of
//  which only ~20 ms was the template match: the rest was a 230 ms stick press
//  and a 260 ms settle, paid once per tile. Routes are mostly long straight
//  segments, so instead we hold the stick down for a whole segment and fix our
//  position once at the end of it.
//
//  Gen 3 walks one tile per 16 frames (~267 ms) and runs one per 8 (~133 ms).
//  Holding B runs when the player has the Running Shoes and is harmless when
//  they do not -- B does nothing else in the overworld.
constexpr int RUN_TILE_MS = 138;

//  Pressing a direction the player is not already facing spends a few frames
//  turning on the spot before they move. Pay it once per burst, not per tile.
constexpr int TURN_ALLOWANCE_MS = 100;

//  Tiles per burst. Bounded by localization, not by movement: the position fix
//  afterwards has to search a window big enough to contain both "walked the
//  whole way" and "stopped immediately", and that window grows with the burst.
//  Eight keeps it to ~16x16 tiles, well inside the range where the terrain is
//  distinctive enough to match, while still amortizing the settle 8 ways.
constexpr int MAX_BURST_TILES = 8;

//  After anything unexpected -- an encounter, a rejected fix, a tile we could
//  not enter -- drop back to single steps until a clean fix comes in. This is
//  what keeps grass safe: encounters interrupt a burst, and the next few tiles
//  are then taken one at a time with a fix after each.
constexpr int CAUTIOUS_BURST_TILES = 1;

//  Inverse of kanto_step_to_joystick(). jy is joystick-up-positive while tile
//  rows increase southward, which is the sign flip that makes this worth having
//  in one place.
KantoStep joystick_to_kanto_step(const Step& s){
    if (s.jx < 0) return KantoStep::West;
    if (s.jx > 0) return KantoStep::East;
    if (s.jy > 0) return KantoStep::North;
    return KantoStep::South;
}

Step kanto_step_to_joystick(KantoStep ks){
    switch (ks){
    case KantoStep::North: return STEP_NORTH;
    case KantoStep::South: return STEP_SOUTH;
    case KantoStep::East:  return STEP_EAST;
    case KantoStep::West:  return STEP_WEST;
    }
    return STEP_NORTH;
}

}  // namespace


static void kanto_navigate_impl(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const KantoGoal& goal,
    const KantoGoal* start_hint,
    int max_steps
);

void kanto_navigate_to(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const KantoGoal& goal,
    int max_steps
){
    kanto_navigate_impl(env, context, goal, nullptr, max_steps);
}
void kanto_navigate_to(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const KantoGoal& goal,
    const KantoGoal& start_hint,
    int max_steps
){
    kanto_navigate_impl(env, context, goal, &start_hint, max_steps);
}

static void kanto_navigate_impl(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const KantoGoal& goal,
    const KantoGoal* start_hint,
    int max_steps
){
    const KantoMapDetector& detector = KantoMapDetector::instance();

    int flees = 0;

    //  Closest we have ever been to the goal on this trip, in tiles. Every new
    //  best resets the flee counter: encounters are only a problem when they
    //  stop us getting anywhere.
    int best_distance = INT_MAX;
    int unknown_polls = 0;
    int no_progress_count = 0;
    int learned_blocks = 0;

    //  Burst state. hint_x/hint_y is where we expect to be at the next poll --
    //  the midpoint of the burst we just issued, so the search window covers
    //  both ends of it. tiles_in_flight is how far the player may legitimately
    //  have travelled since the last confirmed fix, which is what the motion
    //  gate has to allow for.
    int hint_x = -999, hint_y = -999;
    int hint_radius = HINT_RADIUS_TILES;
    int tiles_in_flight = 1;
    int burst_cap = MAX_BURST_TILES;

    bool have_prev_pos = false;
    int prev_x = -999, prev_y = -999;
    Step last_step = STEP_NORTH;

    //  Seed the search window from the caller's belief about where we are.
    //
    //  Without this the first fix is a cold full-map match, which is the only
    //  kind the detector's ambiguity gate applies to. Standing in the middle of
    //  Route 1's grass -- a field of identical tiles -- the gate refuses every
    //  candidate and the run stalls before taking a step, burning ~10 s per poll
    //  on an unconstrained 6528x6400 match. Observed 2026-08-18 16:27: sixteen
    //  consecutive "Position unknown", best tile (303,67) conf=0.56, nowhere
    //  near Route 1, until the run aborted.
    //
    //  The hint is only a starting guess. It is not trusted: the very first
    //  detection bypasses the motion gate (we cannot know how far off the hint
    //  was), and from then on the normal gate applies.
    bool seeded_hint_unconfirmed = false;
    if (start_hint != nullptr){
        prev_x = start_hint->tile_x;
        prev_y = start_hint->tile_y;
        have_prev_pos = true;
        seeded_hint_unconfirmed = true;
        env.log(
            std::string("Kanto navigator: seeding the search window at (") +
            std::to_string(prev_x) + "," + std::to_string(prev_y) +
            ") -- where the program believes it is.",
            COLOR_BLUE
        );
    }

    //  Direction the player is currently facing. In Gen 3 the first tap in a new
    //  direction turns the player in place without moving them, so a "didn't
    //  move" poll right after a direction change is expected, not an obstacle.
    bool have_facing = false;
    double facing_jx = 0.0, facing_jy = 0.0;
    bool prev_step_was_turn = false;

    //  True when the previous step was cut short by an encounter or a fade. The
    //  player ends up back on the tile they started from, so "didn't move" on the
    //  next poll is expected and must not count against the no-progress budget.
    bool prev_step_interrupted = false;

    //  Consecutive motion-gate rejections. Reset on any accepted detection.
    int rejected_jumps = 0;
    //  Consecutive A* failures. Reset whenever A* returns a step.
    int astar_failures = 0;
    //  Tiles stood on since A* last failed. Cleared the moment A* can plan
    //  again -- it exists only to stop escape mode retreading its own steps.
    std::set<uint32_t> escape_visited;
    //  Consecutive steps spent escaping a start tile the mask calls blocked.
    //  Reset as soon as we are standing somewhere A* can plan from.
    int blocked_start_escapes = 0;
    //  Log the unrendered-map compensation once per navigation run, not per poll.
    bool void_reported = false;

    //  Flee an encounter found mid-navigation and mark the step interrupted.
    //  Shared by the pre-locate battle gate and the unknown-position probe.
    auto flee_encounter = [&](const std::string& why){
        flees++;
        if (flees > MAX_FLEES_WITHOUT_PROGRESS){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "Fled " + std::to_string(flees) + " encounters without getting any "
                "closer to the goal. Not a busy patch of grass -- something is "
                "keeping us in one place.",
                env.console
            );
        }
        env.log(
            why + " Fleeing (" + std::to_string(flees) + " since last progress).",
            COLOR_YELLOW
        );
        flee_battle(env.console, context);
        context.wait_for_all_requests();
        //  Fleeing returns the player to the tile they were already on.
        prev_step_interrupted = true;
        have_facing = false;
        unknown_polls = 0;
    };

    env.log(
        std::string("Kanto navigator: target tile (") +
        std::to_string(goal.tile_x) + ", " +
        std::to_string(goal.tile_y) + ") on the combined map.",
        COLOR_BLUE
    );

    for (int step = 0; step < max_steps; ++step){
        VideoSnapshot snap = env.console.video().snapshot();

        //  Battle gate: establish that this frame is even the overworld BEFORE
        //  asking the map detector where we are.
        //
        //  This is not a nicety. TM_CCOEFF_NORMED scores a battle screen against
        //  the Kanto map at 0.64-0.71 -- far above MIN_CONFIDENCE -- so locate()
        //  confidently returns a bogus tile, `pos` is never null, the
        //  unknown-position battle probe further down never runs, and the
        //  navigator keeps pressing the stick against a battle until the step
        //  budget or the no-progress guard kills the run.
        //
        //  Straight out of error report 20260502-213206347118: eight consecutive
        //  "off-map (36,188) conf=0.640, going east" steps while
        //  "Wild RATTATA appeared!" was on screen. The confidence threshold cannot
        //  fix this -- 0.64 is a genuinely strong correlation -- so the frame has
        //  to be classified before it is localized.
        if (snap){
            BattleDialogDetector battle_dialog(COLOR_RED);
            BattleMenuDetector battle_menu(COLOR_RED);
            if (battle_dialog.detect(*snap.frame) || battle_menu.detect(*snap.frame)){
                flee_encounter("Battle detected during navigation (frame is not the overworld).");
                continue;
            }
        }

        //  Compensate for unrendered map.
        //
        //  Kanto-Combined.png leaves the space outside its stitched sub-maps
        //  pure white, while the game renders its border block there. A viewport
        //  overlapping a void is therefore compared against a partly meaningless
        //  template, and TM_CCOEFF_NORMED drops roughly in proportion to how much
        //  of the template is blank. Holding such a viewport to the full 0.40
        //  floor asks it to clear a bar it cannot reach.
        //
        //  Route1SouthGrass{78,235} is 6 tiles from the void that starts at x=84
        //  on that row: its viewport always contained 2 white columns and peaked
        //  at ~0.76 instead of the ~0.98 seen on interior ground. Two tiles of
        //  drift east put it under 0.40 and localization died mid-run on
        //  2026-08-18 -- 16 polls, all rejecting a correct fix at 0.355.
        //
        //  Only relax for HINTED matches. The window is +-HINT_RADIUS_TILES, so a
        //  wrong accept is a few tiles out at worst and the motion gate below
        //  catches it. A cold full-map match keeps the full floor, because there a
        //  wrong accept can land anywhere in Kanto.
        double hinted_min_conf = MIN_CONFIDENCE;
        if (have_prev_pos){
            const double void_fraction = detector.viewport_void_fraction(prev_x, prev_y);
            if (void_fraction > 0.0){
                hinted_min_conf = std::max(
                    MIN_CONFIDENCE * (1.0 - void_fraction),
                    MIN_CONFIDENCE_FLOOR
                );
                if (!void_reported){
                    char vbuf[260];
                    std::snprintf(
                        vbuf, sizeof(vbuf),
                        "Map around (%d,%d) is %.0f%% unrendered -- the template is partly blank "
                        "here, so the hinted confidence floor drops %.2f -> %.2f.",
                        prev_x, prev_y, void_fraction * 100.0,
                        MIN_CONFIDENCE, hinted_min_conf
                    );
                    env.log(vbuf, COLOR_YELLOW);
                    void_reported = true;
                }
            }
        }

        //  Search around where the burst should have put us, not around where it
        //  started. For a single step the two are the same tile.
        const int search_x = (hint_x == -999) ? prev_x : hint_x;
        const int search_y = (hint_y == -999) ? prev_y : hint_y;

        std::optional<KantoPosition> pos = snap
            ? (have_prev_pos
                ? detector.locate(*snap.frame, hinted_min_conf, search_x, search_y, hint_radius)
                : detector.locate(*snap.frame, MIN_CONFIDENCE))
            : std::nullopt;

        //  Motion gate: a detection further from the last confirmed position than
        //  the burst could possibly have carried us cannot be real. Discard it
        //  rather than letting it seed the next poll's hint window, which is how
        //  a single bad match used to lock navigation onto the wrong part of the
        //  map permanently.
        //
        //  A single step keeps the original tight bound. A burst gets its own
        //  length plus that bound as slack -- the hold is timed in milliseconds
        //  against an emulated frame rate, so landing a tile past the intended
        //  one is normal and must not be mistaken for a bad match.
        const int max_jump_allowed = tiles_in_flight <= 1
            ? MAX_TILE_JUMP
            : MAX_TILE_JUMP + tiles_in_flight;
        if (pos && have_prev_pos && !seeded_hint_unconfirmed){
            int jump = std::max(
                std::abs(pos->tile_x - prev_x),
                std::abs(pos->tile_y - prev_y)
            );
            if (jump > max_jump_allowed){
                rejected_jumps++;
                //  Keep the hint for the first few rejections; only go cold once
                //  the hinted window has repeatedly disagreed with physics, which
                //  is what a genuine displacement (a warp, a whiteout mid-walk)
                //  looks like.
                const bool go_cold = rejected_jumps >= MAX_REJECTED_JUMPS;
                char buf[260];
                std::snprintf(
                    buf, sizeof(buf),
                    "Rejected implausible jump: (%d,%d) -> (%d,%d) is %d tiles in one step "
                    "(conf=%.3f, rejection %d/%d). %s",
                    prev_x, prev_y, pos->tile_x, pos->tile_y, jump, pos->confidence,
                    rejected_jumps, MAX_REJECTED_JUMPS,
                    go_cold
                        ? "Giving up on the hint; re-localizing from the full map."
                        : "Keeping previous position and retrying."
                );
                env.log(buf, COLOR_YELLOW);
                pos.reset();
                if (go_cold){
                    have_prev_pos = false;
                    rejected_jumps = 0;
                }
                unknown_polls++;
                if (unknown_polls > MAX_UNKNOWN_POLLS){
                    OperationFailedException::fire(
                        ErrorReport::SEND_ERROR_REPORT,
                        "Position could not be established for " +
                            std::to_string(MAX_UNKNOWN_POLLS) +
                            " consecutive polls (repeated implausible jumps).",
                        env.console
                    );
                }
                context.wait_for(std::chrono::milliseconds(400));
                continue;
            }
            rejected_jumps = 0;
        }

        if (!pos){
            //  Probe for an active battle dialog. Encounters between steps
            //  drop confidence below threshold; flee and resume.
            BattleDialogWatcher battle_probe(COLOR_RED);
            int battle_check = wait_until(
                env.console, context,
                std::chrono::milliseconds(1500),
                {battle_probe}
            );
            if (battle_check == 0){
                //  Backstop for the battle gate above: catches an encounter that
                //  is mid-fade (dialog borders not yet drawn) when we snapshot, so
                //  the gate misses it and locate() also fails. The gate handles the
                //  much more dangerous case where locate() *succeeds* on a battle
                //  screen.
                flee_encounter("Position unknown but battle dialog showing.");
                continue;
            }

            unknown_polls++;
            //  Diagnostic best-guess for the log. Keep it inside the hint window
            //  when we have one: an unconstrained match on the 6528x6400 map
            //  allocates a ~157 MB result Mat (plus a clone) and costs seconds,
            //  which is far too expensive to pay just to print a line.
            //  With no hint the only diagnostic available is an unconstrained
            //  match, and that is genuinely expensive: a ~157 MB result Mat plus
            //  a clone, several seconds of wall clock. Paying it on every failed
            //  poll turned a 15-poll timeout into 150 s of thrash on 8/18. Run it
            //  on the first failure and then sparsely -- enough to keep the log
            //  useful without making the failure path the slow path.
            std::optional<KantoPosition> raw;
            if (snap){
                if (have_prev_pos){
                    raw = detector.locate(*snap.frame, -2.0, prev_x, prev_y, HINT_RADIUS_TILES);
                }else if (unknown_polls == 1 || unknown_polls % 5 == 0){
                    raw = detector.locate(*snap.frame, -2.0);
                }
            }
            if (raw){
                char buf[160];
                std::snprintf(
                    buf, sizeof(buf),
                    "Position unknown (poll %d/%d). Best tile (%d,%d) conf=%.3f - below threshold.",
                    unknown_polls, MAX_UNKNOWN_POLLS,
                    raw->tile_x, raw->tile_y, raw->confidence
                );
                env.log(buf, COLOR_YELLOW);
            }else{
                env.log(
                    "Position unknown (poll " + std::to_string(unknown_polls) +
                    "/" + std::to_string(MAX_UNKNOWN_POLLS) + ").",
                    COLOR_YELLOW
                );
            }
            //  Give up on the hint and go cold.
            //
            //  The motion gate only fires on detections we ACCEPT, so a hint that
            //  is simply wrong -- the player drifted further than the search
            //  radius while we weren't looking -- produces nothing to reject and
            //  the window never widens. Before this, all 15 polls re-searched the
            //  same wrong +-4 tiles and the run died. Observed 2026-08-19: heal
            //  trip seeded (78,235); the player was actually at (81,226), nine
            //  tiles north, so every poll was doomed from the first.
            if (have_prev_pos && unknown_polls == HINT_GIVEUP_POLLS){
                env.log(
                    "Hinted search has failed " + std::to_string(HINT_GIVEUP_POLLS) +
                        " times -- the player is not near where we believed. "
                        "Dropping the hint and re-localizing from the full map.",
                    COLOR_YELLOW
                );
                have_prev_pos = false;
                seeded_hint_unconfirmed = false;
                void_reported = false;
            }
            if (unknown_polls > MAX_UNKNOWN_POLLS){
                OperationFailedException::fire(
                    ErrorReport::SEND_ERROR_REPORT,
                    "Position unknown for " + std::to_string(MAX_UNKNOWN_POLLS) +
                        " consecutive polls. Likely a dialog or off-map area.",
                    env.console
                );
            }
            context.wait_for(std::chrono::milliseconds(750));
            continue;
        }
        unknown_polls = 0;

        //  The seeded hint has now produced a real detection, so from here on the
        //  motion gate applies normally.
        if (seeded_hint_unconfirmed){
            char seed_buf[200];
            std::snprintf(
                seed_buf, sizeof(seed_buf),
                "Kanto navigator: first fix (%d,%d) conf=%.3f, %d tile(s) from the seeded hint.",
                pos->tile_x, pos->tile_y, pos->confidence,
                std::max(std::abs(pos->tile_x - prev_x), std::abs(pos->tile_y - prev_y))
            );
            env.log(seed_buf, COLOR_BLUE);
            seeded_hint_unconfirmed = false;
        }

        //  Goal check.
        if (std::abs(pos->tile_x - goal.tile_x) <= goal.tolerance &&
            std::abs(pos->tile_y - goal.tile_y) <= goal.tolerance){
            env.log(
                std::string("Reached goal: (") +
                std::to_string(pos->tile_x) + ", " +
                std::to_string(pos->tile_y) + ").",
                COLOR_BLUE
            );
            return;
        }

        //  No-progress detection: same tile two polls in a row means we hit
        //  an obstacle the path doesn't account for.
        bool didnt_move = have_prev_pos &&
                          pos->tile_x == prev_x && pos->tile_y == prev_y;
        if (didnt_move && (prev_step_was_turn || prev_step_interrupted)){
            //  Expected non-movement: either the previous press only turned the
            //  player to face a new direction, or the step was interrupted by an
            //  encounter/fade that returned them to the same tile. Neither is an
            //  obstacle -- hold the counter steady (rather than resetting it) so a
            //  genuinely blocked tile still trips it on the following press.
            env.log(
                prev_step_was_turn
                    ? "Previous step turned the player in place (expected). Re-issuing."
                    : "Previous step was interrupted (expected no movement). Re-issuing."
            );
            //  Consume the excuse. Without this the flags survive until the next
            //  step is issued, so one encounter could excuse several consecutive
            //  polls in a row and mask a genuinely blocked tile.
            prev_step_was_turn = false;
            prev_step_interrupted = false;
        }else if (didnt_move){
            no_progress_count++;

            //  Learn the obstacle rather than dying on it.
            //
            //  Pressing into the same tile several times without moving is
            //  proof that tile is not walkable, whatever the mask claims. Record
            //  it and let A* re-plan: the alternative route is usually only a
            //  few tiles longer, and the run continues.
            //
            //  This is what the two hand-written mask overrides above would have
            //  been if the program could write them itself. It matters because
            //  the generated mask's defects are systematic -- mountain interiors
            //  marked walkable, tree columns blocked on alternating rows -- and
            //  we only find them one dead run at a time.
            if (no_progress_count >= NO_PROGRESS_BEFORE_LEARNING){
                const KantoStep tried = joystick_to_kanto_step(last_step);
                if (kanto_edge_blocked(pos->tile_x, pos->tile_y, tried)){
                    //  Already knew this one. Re-recording it teaches nothing and
                    //  spends the trip's learning budget on a duplicate -- on
                    //  2026-08-19 the same edge was "learned" three times while
                    //  the greedy fallback kept steering back into it. Let the
                    //  no-progress counter run on to the hard failure instead;
                    //  something upstream is choosing a direction we know is dead.
                    env.log(
                        "Still being pushed " + std::string(last_step.name) + " from (" +
                            std::to_string(pos->tile_x) + "," + std::to_string(pos->tile_y) +
                            "), which is already a known wall.",
                        COLOR_RED
                    );
                }else if (learned_blocks < MAX_LEARNED_BLOCKS_PER_NAVIGATION){
                    //  Record the EDGE, not the destination tile.
                    //
                    //  "I could not walk north out of here" does not mean the
                    //  tile to the north is solid -- a Gen 3 ledge is one-way, so
                    //  the tile above may be one we walked through on the way
                    //  down. Blacklisting it, as the first version of this did,
                    //  carves a hole in a route that actually works.
                    kanto_mark_edge_blocked(
                        pos->tile_x, pos->tile_y, joystick_to_kanto_step(last_step)
                    );
                    learned_blocks++;
                    env.log(
                        "Cannot go " + std::string(last_step.name) + " from (" +
                            std::to_string(pos->tile_x) + "," + std::to_string(pos->tile_y) +
                            ") -- wall, or a one-way ledge. Closing that edge and re-routing. " +
                            std::to_string(learned_blocks) + "/" +
                            std::to_string(MAX_LEARNED_BLOCKS_PER_NAVIGATION) +
                            " this trip, " + std::to_string(kanto_learned_edge_count()) +
                            " this session.",
                        COLOR_BLUE
                    );
                    no_progress_count = 0;
                }
            }

            if (no_progress_count >= MAX_NO_PROGRESS){
                OperationFailedException::fire(
                    ErrorReport::SEND_ERROR_REPORT,
                    std::string("Player has not moved for ") +
                    std::to_string(MAX_NO_PROGRESS) + " steps at tile (" +
                    std::to_string(pos->tile_x) + ", " +
                    std::to_string(pos->tile_y) + "). Walkable mask may be "
                    "wrong here, or NPC blocking. Learned " +
                    std::to_string(learned_blocks) + " obstacle(s) on this trip "
                    "without finding a way through.",
                    env.console
                );
            }
        }else{
            no_progress_count = 0;
        }
        //  We are standing here, so this tile is walkable -- whatever the
        //  generated mask says about it. This is the single most reliable fact
        //  the navigator ever has, and recording it is what stops the run dying
        //  with "standing on a tile the walkable mask calls blocked": on
        //  2026-08-19 a ledge hop landed the player on a tile the mask called
        //  solid, and the escape logic burned its whole budget arguing with the
        //  map instead of believing the player.
        kanto_mark_tile_walkable(pos->tile_x, pos->tile_y);

        //  Getting closer clears the encounter budget. Ten fights on the way
        //  across a route is a normal Tuesday; ten fights without gaining a tile
        //  is a stall.
        {
            const int distance =
                std::abs(pos->tile_x - goal.tile_x) + std::abs(pos->tile_y - goal.tile_y);
            if (distance < best_distance){
                best_distance = distance;
                flees = 0;
            }
        }

        prev_x = pos->tile_x;
        prev_y = pos->tile_y;
        have_prev_pos = true;

        //  Blocked start tile.
        //
        //  kanto_pathfind_next_step() returns nullopt both when it cannot find a
        //  route and when the tile we are standing on is itself blocked in the
        //  mask -- but those need opposite responses, so separate them here
        //  rather than letting the A*-failure path guess.
        //
        //  A blocked *start* is not a localization problem: re-localizing returns
        //  the same tile, and the greedy fallback is actively harmful because it
        //  walks the dominant axis from a tile the map does not understand. On
        //  2026-08-18 that marched the bot out of the Viridian Poke Center door
        //  (74,206 -- blocked in the mask, and the player was standing on it)
        //  straight down x=74 into the fence at (74,215), where it burned the
        //  entire no-progress budget and ended the run.
        //
        //  The fix is small and local: step onto whichever neighbouring tile the
        //  mask does accept, preferring the one nearest the goal, and let A* take
        //  over from solid ground. A doorway is adjacent to open ground by
        //  construction, so this normally costs exactly one step.
        bool step_chosen = false;
        if (!kanto_tile_walkable(pos->tile_x, pos->tile_y)){
            blocked_start_escapes++;
            if (blocked_start_escapes > MAX_BLOCKED_START_ESCAPES){
                OperationFailedException::fire(
                    ErrorReport::SEND_ERROR_REPORT,
                    "Standing on a tile the walkable mask calls blocked at (" +
                        std::to_string(pos->tile_x) + "," +
                        std::to_string(pos->tile_y) + ") and could not escape it in " +
                        std::to_string(MAX_BLOCKED_START_ESCAPES) +
                        " steps. The walkable mask is wrong around here.",
                    env.console
                );
            }

            const struct{ int dx; int dy; Step step; } ESCAPES[4] = {
                { 0, -1, STEP_NORTH},
                { 0, +1, STEP_SOUTH},
                {+1,  0, STEP_EAST },
                {-1,  0, STEP_WEST },
            };
            bool have_escape = false;
            Step escape = STEP_NORTH;
            int best_dist = 0;
            for (const auto& e : ESCAPES){
                int nx = pos->tile_x + e.dx;
                int ny = pos->tile_y + e.dy;
                if (!kanto_tile_walkable(nx, ny)){
                    continue;
                }
                int dist = std::abs(nx - goal.tile_x) + std::abs(ny - goal.tile_y);
                if (!have_escape || dist < best_dist){
                    have_escape = true;
                    best_dist = dist;
                    escape = e.step;
                }
            }
            if (!have_escape){
                OperationFailedException::fire(
                    ErrorReport::SEND_ERROR_REPORT,
                    "Standing on a tile the walkable mask calls blocked at (" +
                        std::to_string(pos->tile_x) + "," +
                        std::to_string(pos->tile_y) +
                        "), and all four neighbours are blocked too. The walkable mask is "
                        "wrong around here.",
                    env.console
                );
            }

            char esc_buf[240];
            std::snprintf(
                esc_buf, sizeof(esc_buf),
                "Start tile (%d,%d) is blocked in the walkable mask -- A* cannot plan from "
                "here. Stepping %s onto walkable ground and re-planning (%d/%d).",
                pos->tile_x, pos->tile_y, escape.name,
                blocked_start_escapes, MAX_BLOCKED_START_ESCAPES
            );
            env.log(esc_buf, COLOR_YELLOW);
            last_step = escape;
            step_chosen = true;
        }else{
            blocked_start_escapes = 0;
        }

        //  How far to walk before the next position fix.
        //
        //  Full speed only from a clean state. Anything that suggests we are not
        //  where we think we are -- a blocked tile, a rejected fix, an encounter
        //  that cut the last burst short -- drops back to single steps until a
        //  clean fix comes in. In practice this is what slows the program down
        //  in grass, where encounters interrupt bursts, and lets it run flat out
        //  on roads and routes.
        const bool confident =
            no_progress_count == 0 && rejected_jumps == 0 && !prev_step_interrupted;
        burst_cap = confident ? MAX_BURST_TILES : CAUTIOUS_BURST_TILES;

        //  A* pathfind to the goal, taking the whole first straight segment.
        int run_len = 1;
        std::optional<KantoStep> next = step_chosen
            ? std::nullopt
            : kanto_pathfind_next_run(
                  pos->tile_x, pos->tile_y,
                  goal.tile_x, goal.tile_y,
                  burst_cap, &run_len
              );
        if (!next && !step_chosen){
            astar_failures++;

            //  A* failing means one of two things: the position we handed it is
            //  wrong, or the walkable mask is wrong at this tile. Assume the
            //  former first.
            //
            //  Walking the dominant axis from a position we do not trust is
            //  exactly how a single bad fix became eight steps pressed into a
            //  battle screen in error report 20260502-213206347118 -- every one
            //  of those steps logged "A* failed ... Falling back to greedy east".
            //  Re-localize instead, and only fall back to greedy movement once a
            //  position has repeatedly failed to path, which is the signature of a
            //  genuine mask error rather than a mis-fix.
            if (astar_failures <= MAX_ASTAR_FAILURES_BEFORE_GREEDY){
                char buf[220];
                std::snprintf(
                    buf, sizeof(buf),
                    "A* failed at (%d,%d) [%d/%d]. Re-localizing from the full map "
                    "rather than walking blind.",
                    pos->tile_x, pos->tile_y,
                    astar_failures, MAX_ASTAR_FAILURES_BEFORE_GREEDY
                );
                env.log(buf, COLOR_YELLOW);
                have_prev_pos = false;
                unknown_polls++;
                if (unknown_polls > MAX_UNKNOWN_POLLS){
                    OperationFailedException::fire(
                        ErrorReport::SEND_ERROR_REPORT,
                        "A* could not path from " + std::to_string(MAX_UNKNOWN_POLLS) +
                            " consecutive positions. Localization or walkable mask is wrong.",
                        env.console
                    );
                }
                context.wait_for(std::chrono::milliseconds(400));
                continue;
            }

            //  Escape mode.
            //
            //  A* has no route from here, which after a one-way ledge is the
            //  normal case: you hop down into a pocket whose real exit the mask
            //  does not know about. Hill-climbing toward the goal does not work
            //  there -- with no memory it just oscillates. Observed 2026-08-19:
            //  (45,207) greedy north, (45,206) greedy south, back and forth 180
            //  times until the step budget died, because east was walled and
            //  north/south scored identically from each tile.
            //
            //  So: explore instead of hill-climb. Remember where we have been
            //  since A* started failing and prefer somewhere new, and be willing
            //  to walk at tiles the mask calls solid -- the mask is wrong often
            //  enough that probing one is a reasonable move when the alternative
            //  is a dead end. Every tile we actually reach is recorded walkable,
            //  so a successful probe grows the map and usually lets A* re-plan.
            escape_visited.insert(tile_key(pos->tile_x, pos->tile_y));
            if ((int)escape_visited.size() > MAX_ESCAPE_TILES){
                OperationFailedException::fire(
                    ErrorReport::SEND_ERROR_REPORT,
                    "Explored " + std::to_string(escape_visited.size()) + " tiles around (" +
                        std::to_string(pos->tile_x) + "," + std::to_string(pos->tile_y) +
                        ") without finding anywhere A* can plan a route to the goal from. "
                        "Either the map is wrong over a wide area here, or this really is "
                        "a pocket with no way out.",
                    env.console
                );
            }

            const int dx = goal.tile_x - pos->tile_x;
            const int dy = goal.tile_y - pos->tile_y;
            struct Cand{ Step step; KantoStep ks; int ddx; int ddy; int gain; };
            const Cand CANDS[4] = {
                {STEP_EAST,  KantoStep::East,  +1,  0, dx > 0 ?  std::abs(dx) : -std::abs(dx)},
                {STEP_WEST,  KantoStep::West,  -1,  0, dx < 0 ?  std::abs(dx) : -std::abs(dx)},
                {STEP_SOUTH, KantoStep::South,  0, +1, dy > 0 ?  std::abs(dy) : -std::abs(dy)},
                {STEP_NORTH, KantoStep::North,  0, -1, dy < 0 ?  std::abs(dy) : -std::abs(dy)},
            };

            //  Preference order, most significant first:
            //    1. somewhere we have not already been during this escape
            //    2. somewhere the map believes is walkable
            //    3. whichever closes the distance to the goal most
            Step fb = STEP_NORTH;
            bool found_fb = false;
            bool best_fresh = false, best_walkable = false;
            int best_gain = INT_MIN;
            bool probing = false;
            for (const Cand& c : CANDS){
                if (kanto_edge_blocked(pos->tile_x, pos->tile_y, c.ks)){
                    continue;
                }
                const int nx = pos->tile_x + c.ddx;
                const int ny = pos->tile_y + c.ddy;
                if (nx < 0 || ny < 0){
                    continue;
                }
                const bool fresh = escape_visited.find(tile_key(nx, ny)) == escape_visited.end();
                const bool walkable = kanto_tile_walkable(nx, ny);

                bool better;
                if (!found_fb){
                    better = true;
                }else if (fresh != best_fresh){
                    better = fresh;
                }else if (walkable != best_walkable){
                    better = walkable;
                }else{
                    better = c.gain > best_gain;
                }
                if (better){
                    found_fb = true;
                    best_fresh = fresh;
                    best_walkable = walkable;
                    best_gain = c.gain;
                    fb = c.step;
                    probing = !walkable;
                }
            }
            if (!found_fb){
                OperationFailedException::fire(
                    ErrorReport::SEND_ERROR_REPORT,
                    "No route to the goal from (" + std::to_string(pos->tile_x) + "," +
                        std::to_string(pos->tile_y) + "), and all four directions out of "
                        "this tile are edges we have already proved impassable.",
                    env.console
                );
            }
            env.log(
                std::string("A* failed at (") +
                std::to_string(pos->tile_x) + "," +
                std::to_string(pos->tile_y) + "). Escaping " + fb.name +
                (probing ? " (probing a tile the map calls solid)" : "") +
                " -- " + std::to_string(escape_visited.size()) + " tile(s) explored.",
                COLOR_YELLOW
            );
            last_step = fb;
        }else if (next){
            astar_failures = 0;
            //  Back on a planned route. Whatever escape mode explored is history.
            if (!escape_visited.empty()){
                env.log(
                    "Escaped: A* can plan from (" + std::to_string(pos->tile_x) + "," +
                        std::to_string(pos->tile_y) + ") again after exploring " +
                        std::to_string(escape_visited.size()) + " tile(s).",
                    COLOR_BLUE
                );
                escape_visited.clear();
            }
            last_step = kanto_step_to_joystick(*next);
        }

        //  A greedy or escape step is a single tile: neither is following a
        //  planned route, so there is no straight segment to commit to.
        if (step_chosen || !next){
            run_len = 1;
        }
        run_len = std::max(1, std::min(run_len, burst_cap));

        const char* region = kanto_region_name(kanto_region_at(pos->tile_x, pos->tile_y));
        char log_buf[220];
        std::snprintf(
            log_buf, sizeof(log_buf),
            "Step %d: %s (%d,%d) conf=%.3f, going %s x%d",
            step + 1, region, pos->tile_x, pos->tile_y,
            pos->confidence, last_step.name, run_len
        );
        env.log(log_buf);

        //  Is this press a direction change? If so the game may spend it turning
        //  the player in place instead of moving them, and the next poll will
        //  legitimately report the same tile.
        prev_step_was_turn = !have_facing ||
                             facing_jx != last_step.jx ||
                             facing_jy != last_step.jy;
        have_facing = true;
        facing_jx = last_step.jx;
        facing_jy = last_step.jy;

        //  Issue the joystick step with parallel watchers for encounter,
        //  freeze, and door fade.
        AdvanceBattleDialogWatcher encounter(COLOR_RED);
        FrozenImageDetector frozen(std::chrono::milliseconds(2500), 10.0);
        BlackScreenOverWatcher door_fade(COLOR_RED);

        //  Hold the stick for the whole segment, with B down to run.
        //
        //  pbf_controller_state drives button and stick together, which is what
        //  running requires -- B alone does nothing in the overworld, and B with
        //  a direction is the Running Shoes. The watchers still run in parallel,
        //  so an encounter cuts the hold short exactly as it did a single step.
        const Milliseconds hold_ms = std::chrono::milliseconds(
            (prev_step_was_turn ? TURN_ALLOWANCE_MS : 0) + run_len * RUN_TILE_MS
        );
        int ret = run_until<ProControllerContext>(
            env.console, context,
            [&](ProControllerContext& sub){
                pbf_controller_state(
                    sub,
                    BUTTON_B,
                    DPAD_NONE,
                    {last_step.jx, last_step.jy},
                    {0.0, 0.0},
                    hold_ms
                );
                pbf_wait(sub, STEP_RELEASE);
            },
            {encounter, frozen, door_fade}
        );

        switch (ret){
        case 0: {
            flees++;
            if (flees > MAX_FLEES_WITHOUT_PROGRESS){
                OperationFailedException::fire(
                    ErrorReport::SEND_ERROR_REPORT,
                    "Fled " + std::to_string(flees) + " encounters without getting any "
                    "closer to the goal. Not a busy patch of grass -- something is "
                    "keeping us in one place.",
                    env.console
                );
            }
            env.log(
                std::string("Encounter mid-step. Fleeing (") +
                std::to_string(flees) + " since last progress).",
                COLOR_YELLOW
            );
            flee_battle(env.console, context);
            context.wait_for_all_requests();
            break;
        }
        case 1:
            env.log("Sprite frozen during step (likely obstacle).", COLOR_YELLOW);
            break;
        case 2: {
            BattleDialogWatcher battle(COLOR_RED);
            int probe = wait_until(
                env.console, context,
                std::chrono::milliseconds(2500),
                {battle}
            );
            if (probe == 0){
                flees++;
                if (flees > MAX_FLEES_WITHOUT_PROGRESS){
                    OperationFailedException::fire(
                        ErrorReport::SEND_ERROR_REPORT,
                        "Fled " + std::to_string(flees) + " encounters without getting any "
                        "closer to the goal. Not a busy patch of grass -- something is "
                        "keeping us in one place.",
                        env.console
                    );
                }
                env.log("Black-screen fade was an encounter intro. Fleeing.", COLOR_YELLOW);
                flee_battle(env.console, context);
                context.wait_for_all_requests();
            }else{
                env.log("Door fade detected. Navigation complete.", COLOR_BLUE);
                return;
            }
            break;
        }
        default:
            break;
        }

        //  Let the queued input drain and the camera scroll finish before the
        //  next iteration snapshots the screen. Without this the capture lands
        //  mid-scroll and the viewport is offset by up to half a tile, which
        //  feeds straight into a misread tile.
        context.wait_for_all_requests();
        context.wait_for(STEP_SETTLE);

        //  ret == 0 (encounter) and ret == 2 (fade that turned out to be an
        //  encounter intro) both abort the step and leave the player's facing
        //  direction unknown. ret == 1 (frozen against an obstacle) does not:
        //  the player did turn, they just could not walk, and that IS the
        //  obstacle we want the no-progress counter to catch.
        prev_step_interrupted = (ret == 0 || ret == 2);
        if (prev_step_interrupted){
            have_facing = false;
            prev_step_was_turn = false;
        }

        //  Aim the next fix's search window.
        //
        //  An uninterrupted burst should land run_len tiles along, but an
        //  interrupted one may have travelled anything from 0 to run_len. Centre
        //  the window on the midpoint so both ends are inside it, and widen the
        //  radius by half the burst. For run_len == 1 this is the previous
        //  behaviour exactly: centre on the tile ahead, radius HINT_RADIUS_TILES.
        //
        //  Interrupted bursts do not get the benefit of the doubt -- an
        //  encounter stops the player where they stood, so search from there.
        const int travelled = prev_step_interrupted ? 0 : run_len;
        const int dx_tile = (int)last_step.jx;
        const int dy_tile = -(int)last_step.jy;
        hint_x = pos->tile_x + dx_tile * (travelled / 2);
        hint_y = pos->tile_y + dy_tile * (travelled / 2);
        hint_radius = HINT_RADIUS_TILES + (travelled + 1) / 2;
        tiles_in_flight = std::max(1, travelled);
    }

    OperationFailedException::fire(
        ErrorReport::SEND_ERROR_REPORT,
        std::string("Step budget (") + std::to_string(max_steps) +
            ") exhausted in kanto_navigate_to without reaching goal.",
        env.console
    );
}


}
}
}
