/*  Kanto Map Navigator
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <cmath>
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

    env.log(
        std::string("Kanto navigator: target tile (") +
        std::to_string(goal.tile_x) + ", " +
        std::to_string(goal.tile_y) + ") on the combined map.",
        COLOR_BLUE
    );

    for (int step = 0; step < max_steps; ++step){
        VideoSnapshot snap = env.console.video().snapshot();
        std::optional<KantoPosition> pos = snap
            ? (have_prev_pos
                ? detector.locate(*snap.frame, MIN_CONFIDENCE, prev_x, prev_y, 60)
                : detector.locate(*snap.frame, MIN_CONFIDENCE))
            : std::nullopt;

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
                    "Position unknown but battle dialog showing. Fleeing (" +
                    std::to_string(flees) + "/" + std::to_string(MAX_FLEES) + ").",
                    COLOR_YELLOW
                );
                flee_battle(env.console, context);
                context.wait_for_all_requests();
                unknown_polls = 0;
                continue;
            }

            unknown_polls++;
            std::optional<KantoPosition> raw =
                snap ? detector.locate(*snap.frame, -2.0) : std::nullopt;
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
        if (didnt_move){
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
            //  Pathfinder failed (start tile masked as blocked, or no path).
            //  Probably a mask error at this tile. Try a sane fallback:
            //  step toward the goal in the dominant axis.
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
