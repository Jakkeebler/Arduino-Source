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


void kanto_navigate_to(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const KantoGoal& goal,
    int max_steps
){
    const KantoMapDetector& detector = KantoMapDetector::instance();

    int flees = 0;
    int unknown_polls = 0;
    int no_progress_count = 0;
    bool have_prev_pos = false;
    int prev_x = -999, prev_y = -999;
    Step last_step = STEP_NORTH;

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

        std::optional<KantoPosition> pos = snap
            ? (have_prev_pos
                ? detector.locate(*snap.frame, MIN_CONFIDENCE, prev_x, prev_y, HINT_RADIUS_TILES)
                : detector.locate(*snap.frame, MIN_CONFIDENCE))
            : std::nullopt;

        //  Motion gate: a detection more than MAX_TILE_JUMP tiles from the last
        //  confirmed position cannot be real -- we issue at most one step per poll.
        //  Discard it rather than letting it seed the next poll's hint window,
        //  which is how a single bad match used to lock navigation onto the wrong
        //  part of the map permanently.
        if (pos && have_prev_pos){
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
            std::optional<KantoPosition> raw = !snap ? std::nullopt
                : (have_prev_pos
                    ? detector.locate(*snap.frame, -2.0, prev_x, prev_y, HINT_RADIUS_TILES)
                    : detector.locate(*snap.frame, -2.0));
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
            if (no_progress_count >= MAX_NO_PROGRESS){
                OperationFailedException::fire(
                    ErrorReport::SEND_ERROR_REPORT,
                    std::string("Player has not moved for ") +
                    std::to_string(MAX_NO_PROGRESS) + " steps at tile (" +
                    std::to_string(pos->tile_x) + ", " +
                    std::to_string(pos->tile_y) + "). Walkable mask may be "
                    "wrong here, or NPC blocking.",
                    env.console
                );
            }
        }else{
            no_progress_count = 0;
        }
        prev_x = pos->tile_x;
        prev_y = pos->tile_y;
        have_prev_pos = true;

        //  A* pathfind to the goal.
        std::optional<KantoStep> next = kanto_pathfind_next_step(
            pos->tile_x, pos->tile_y,
            goal.tile_x, goal.tile_y
        );
        if (!next){
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
        }else{
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
