/*  Kanto Map Navigator
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
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

constexpr int MAX_FLEES = 5;
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
constexpr int MAX_LEARNED_BLOCKS_PER_NAVIGATION = 8;

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
    int unknown_polls = 0;
    int no_progress_count = 0;
    int learned_blocks = 0;
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
    //  Consecutive steps spent escaping a start tile the mask calls blocked.
    //  Reset as soon as we are standing somewhere A* can plan from.
    int blocked_start_escapes = 0;
    //  Log the unrendered-map compensation once per navigation run, not per poll.
    bool void_reported = false;

    //  Flee an encounter found mid-navigation and mark the step interrupted.
    //  Shared by the pre-locate battle gate and the unknown-position probe.
    auto flee_encounter = [&](const std::string& why){
        flees++;
        if (flees > MAX_FLEES){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "Too many wild encounters (>" + std::to_string(MAX_FLEES) +
                    ") during navigation. Aborting.",
                env.console
            );
        }
        env.log(
            why + " Fleeing (" + std::to_string(flees) + "/" +
                std::to_string(MAX_FLEES) + ").",
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

        std::optional<KantoPosition> pos = snap
            ? (have_prev_pos
                ? detector.locate(*snap.frame, hinted_min_conf, prev_x, prev_y, HINT_RADIUS_TILES)
                : detector.locate(*snap.frame, MIN_CONFIDENCE))
            : std::nullopt;

        //  Motion gate: a detection more than MAX_TILE_JUMP tiles from the last
        //  confirmed position cannot be real -- we issue at most one step per poll.
        //  Discard it rather than letting it seed the next poll's hint window,
        //  which is how a single bad match used to lock navigation onto the wrong
        //  part of the map permanently.
        if (pos && have_prev_pos && !seeded_hint_unconfirmed){
            int jump = std::max(
                std::abs(pos->tile_x - prev_x),
                std::abs(pos->tile_y - prev_y)
            );
            if (jump > MAX_TILE_JUMP){
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
                //  jy is joystick-up-positive; tile rows increase southward.
                const int blocked_x = pos->tile_x + (int)last_step.jx;
                const int blocked_y = pos->tile_y - (int)last_step.jy;
                const bool is_goal = blocked_x == goal.tile_x && blocked_y == goal.tile_y;
                if (is_goal){
                    //  Never blacklist the destination -- that turns a reachable
                    //  goal into a permanently unplannable one.
                    env.log(
                        "Cannot enter the goal tile itself. Not blacklisting it.",
                        COLOR_RED
                    );
                }else if (learned_blocks < MAX_LEARNED_BLOCKS_PER_NAVIGATION){
                    kanto_mark_tile_blocked(blocked_x, blocked_y);
                    learned_blocks++;
                    env.log(
                        "Blocked going " + std::string(last_step.name) + " from (" +
                            std::to_string(pos->tile_x) + "," + std::to_string(pos->tile_y) +
                            "). Marking (" + std::to_string(blocked_x) + "," +
                            std::to_string(blocked_y) + ") unwalkable and re-routing. " +
                            std::to_string(learned_blocks) + "/" +
                            std::to_string(MAX_LEARNED_BLOCKS_PER_NAVIGATION) +
                            " this trip, " + std::to_string(kanto_learned_block_count()) +
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

        //  A* pathfind to the goal.
        std::optional<KantoStep> next = step_chosen
            ? std::nullopt
            : kanto_pathfind_next_step(
                  pos->tile_x, pos->tile_y,
                  goal.tile_x, goal.tile_y
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

            //  Repeated failures from a position we keep re-confirming: treat the
            //  mask as wrong here and nudge toward the goal on the dominant axis.
            int dx = goal.tile_x - pos->tile_x;
            int dy = goal.tile_y - pos->tile_y;
            Step fb;
            if (std::abs(dx) > std::abs(dy)){
                fb = (dx > 0) ? STEP_EAST : STEP_WEST;
            }else if (dy != 0){
                fb = (dy > 0) ? STEP_SOUTH : STEP_NORTH;
            }else{
                fb = STEP_NORTH;
            }
            env.log(
                std::string("A* failed at (") +
                std::to_string(pos->tile_x) + "," +
                std::to_string(pos->tile_y) + "). Falling back to greedy " +
                fb.name + ".",
                COLOR_YELLOW
            );
            last_step = fb;
        }else if (next){
            astar_failures = 0;
            last_step = kanto_step_to_joystick(*next);
        }

        const char* region = kanto_region_name(kanto_region_at(pos->tile_x, pos->tile_y));
        char log_buf[200];
        std::snprintf(
            log_buf, sizeof(log_buf),
            "Step %d: %s (%d,%d) conf=%.3f, going %s",
            step + 1, region, pos->tile_x, pos->tile_y,
            pos->confidence, last_step.name
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

        int ret = run_until<ProControllerContext>(
            env.console, context,
            [&](ProControllerContext& sub){
                pbf_move_left_joystick(sub, {last_step.jx, last_step.jy}, STEP_HOLD, STEP_RELEASE);
            },
            {encounter, frozen, door_fade}
        );

        switch (ret){
        case 0: {
            flees++;
            if (flees > MAX_FLEES){
                OperationFailedException::fire(
                    ErrorReport::SEND_ERROR_REPORT,
                    "Too many wild encounters (>" + std::to_string(MAX_FLEES) +
                        ") during navigation. Aborting.",
                    env.console
                );
            }
            env.log(
                std::string("Encounter mid-step. Fleeing (") +
                std::to_string(flees) + "/" + std::to_string(MAX_FLEES) + ").",
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
                if (flees > MAX_FLEES){
                    OperationFailedException::fire(
                        ErrorReport::SEND_ERROR_REPORT,
                        "Too many wild encounters during navigation. Aborting.",
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
