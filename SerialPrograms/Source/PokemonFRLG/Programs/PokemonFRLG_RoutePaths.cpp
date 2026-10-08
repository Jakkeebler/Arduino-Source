/*  Route Paths
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <vector>
#include "Common/Cpp/Color.h"
#include "Common/Cpp/Filesystem.h"
#include "CommonFramework/Exceptions/OperationFailedException.h"
#include "CommonFramework/ImageTypes/ImageRGB32.h"
#include "CommonFramework/VideoPipeline/VideoFeed.h"
#include "CommonTools/Async/InferenceRoutines.h"
#include "CommonTools/VisualDetectors/BlackScreenDetector.h"
#include "CommonTools/VisualDetectors/FrozenImageDetector.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_PushButtons.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_BattleDialogs.h"
#include "PokemonFRLG/PokemonFRLG_Navigation.h"
#include "PokemonFRLG_KantoMapNavigator.h"
#include "PokemonFRLG_RoutePaths.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

namespace{

enum class WalkSuccess{
    StepsComplete,            // success = all steps finished.
    StepsCompleteOrDoorFade,  // success = all steps finished OR a door fade fires at any point.
};

struct WalkStep{
    double x;
    double y;
    std::chrono::milliseconds hold;
    std::chrono::milliseconds release;
    const char* label;
};

//  Save a screenshot at the end of a successfully-completed walk step. Used
//  for capturing landmark reference frames - run a successful heal trip and
//  the resulting PNGs in TempFiles/landmarks/ are candidates for ImageMatch
//  reference images. Picks a sanitized filename from the step's label.
//  Failures here are non-fatal; we just log and continue.
void capture_landmark_frame(
    SingleSwitchProgramEnvironment& env,
    const char* phase_label,
    const WalkStep& step,
    size_t step_number
){
    try{
        VideoSnapshot snap = env.console.video().snapshot();
        if (!snap){
            return;
        }
        static const std::string dir = "TempFiles/landmarks/";
        Filesystem::create_directories(Filesystem::Path(dir));

        std::string sanitized(step.label);
        for (char& c : sanitized){
            if (c == ' ' || c == '/' || c == '\\' || c == '\'' ||
                c == '(' || c == ')' || c == ',' || c == ':')
            {
                c = '_';
            }
        }
        char index_str[8];
        std::snprintf(index_str, sizeof(index_str), "%02zu", step_number);

        std::string path = dir + std::string(phase_label) + "_step" + index_str + "_" + sanitized + ".png";
        if (snap.frame->save(path)){
            env.log("Saved landmark frame: " + path);
        }
    }catch (...){
        //  Capture is best-effort; never let it abort the walk.
    }
}

//  Run a list of joystick steps with a watcher per step. Each step gets
//  retried independently if a wild encounter triggers during it - we flee
//  the battle, then redo the same step, and only advance to the next step
//  on a clean finish. This prevents the walk from compounding drift after
//  a flee (the bug we hit on Route 1: a mid-walk encounter would over-walk
//  the X-alignment and end the walk against a tree, far from the PC door).
//
//  Failure modes:
//    - encounter (AdvanceBattleDialogWatcher): flee and retry the current step.
//      Up to MAX_FLEES total flees across the whole walk, then throw.
//    - frozen sprite for 2.5s (FrozenImageDetector): throw with the step label.
//    - black-screen fade (BlackScreenOverWatcher), only if success_signal ==
//      StepsCompleteOrDoorFade: probe with BattleDialogWatcher to disambiguate
//      door fade from encounter intro flash. If it's a door, return success.
//      If it's an encounter, fall through to the encounter flee+retry path.
void guarded_walk(
    SingleSwitchProgramEnvironment& env, ProControllerContext& context,
    const char* phase_label,
    const std::vector<WalkStep>& steps,
    WalkSuccess success_signal = WalkSuccess::StepsComplete
){
    constexpr int MAX_FLEES = 3;
    int flees = 0;

    auto handle_encounter = [&](size_t step_index){
        flees++;
        if (flees > MAX_FLEES){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                std::string("Hit ") + std::to_string(MAX_FLEES) +
                    " wild encounters during " + phase_label +
                    " without completing the walk. Aborting.",
                env.console
            );
        }
        env.log(
            std::string("Encounter during ") + phase_label +
            " step " + std::to_string(step_index + 1) +
            "/" + std::to_string(steps.size()) +
            " ('" + steps[step_index].label + "'). Fleeing (attempt " +
            std::to_string(flees) + "/" + std::to_string(MAX_FLEES) +
            ") and retrying just that step.",
            COLOR_YELLOW
        );
        flee_battle(env.console, context);
        context.wait_for_all_requests();
    };

    for (size_t i = 0; i < steps.size();){
        const WalkStep& step = steps[i];
        env.log(
            std::string(phase_label) + " step " + std::to_string(i + 1) +
            "/" + std::to_string(steps.size()) + ": " + step.label
        );

        AdvanceBattleDialogWatcher encounter(COLOR_RED);
        FrozenImageDetector frozen(std::chrono::milliseconds(2500), 10.0);
        BlackScreenOverWatcher door_fade(COLOR_RED);

        std::vector<PeriodicInferenceCallback> watchers = {encounter, frozen};
        if (success_signal == WalkSuccess::StepsCompleteOrDoorFade){
            watchers.push_back(door_fade);
        }

        int ret = run_until<ProControllerContext>(
            env.console, context,
            [&](ProControllerContext& subcontext){
                pbf_move_left_joystick(subcontext, {step.x, step.y}, step.hold, step.release);
            },
            watchers
        );
        switch (ret){
        case 0:
            handle_encounter(i);
            //  Don't advance i: re-run the same step.
            continue;
        case 1:
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                std::string("Sprite frozen during ") + phase_label +
                    " step " + std::to_string(i + 1) + " ('" + step.label +
                    "'): no camera motion for 2.5s. Likely an NPC, ledge, or wall blocking the path.",
                env.console
            );
            break;
        case 2: {
            //  Black-screen fade. Probe for a battle dialog to tell whether
            //  this was a door (success) or an encounter intro (false positive).
            BattleDialogWatcher battle(COLOR_RED);
            int probe = wait_until(
                env.console, context,
                std::chrono::milliseconds(2500),
                {battle}
            );
            if (probe == 0){
                env.log(
                    std::string("Black-screen fade during ") + phase_label +
                    " step " + std::to_string(i + 1) +
                    " was an encounter intro, not a door. Treating as encounter.",
                    COLOR_YELLOW
                );
                handle_encounter(i);
                continue;
            }
            env.log(
                std::string(phase_label) + " complete: door fade detected at step " +
                std::to_string(i + 1) + "/" + std::to_string(steps.size()) + ".",
                COLOR_BLUE
            );
            return;
        }
        default:
            //  Step finished cleanly. Capture a frame for landmark calibration
            //  and advance to the next step.
            capture_landmark_frame(env, phase_label, step, i + 1);
            ++i;
            break;
        }
    }
}

}  // namespace

void walk_to_route1(SingleSwitchProgramEnvironment& env, ProControllerContext& context){
    env.log("Walking to Route 1.");
    //  Long segments broken into ~1.3s chunks so a single mid-step encounter
    //  only over-walks ~1.3s rather than the full segment after the flee+retry.
    static const std::vector<WalkStep> steps = {
        {-1.0,  0.0,  800ms, 100ms, "left out of PC entrance"},
        { 0.0, -1.0, 1300ms, 100ms, "down 1/4 (to grass)"},
        { 0.0, -1.0, 1300ms, 100ms, "down 2/4"},
        { 0.0, -1.0, 1300ms, 100ms, "down 3/4"},
        { 0.0, -1.0, 1300ms, 100ms, "down 4/4"},
        {-1.0,  0.0,  900ms, 100ms, "left to grass corner"},
        { 0.0, +1.0,  900ms, 900ms, "up into grass corner"},
    };
    guarded_walk(env, context, "walk_to_route1", steps);
}

void walk_from_route1_to_pokecenter(SingleSwitchProgramEnvironment& env, ProControllerContext& context){
    env.log("Walking from Route 1 to the Viridian City Pokemon Center via map-driven navigation.");
    //  Drive the player toward the Viridian PC entrance using the position
    //  detector + greedy direction-based stepping. The door-fade watcher
    //  inside kanto_navigate_to short-circuits the moment we cross the
    //  building threshold, so the goal tile only needs to be approximately
    //  in front of the door.
    kanto_navigate_to(env, context, KantoGoals::ViridianPokeCenterEntrance);
}

void walk_to_route22(SingleSwitchProgramEnvironment& env, ProControllerContext& context){
    env.log("Walking to Route 22.");
    // left a few steps
    pbf_move_left_joystick(context, {-1, 0}, 900ms, 200ms);
    // up to the bush
    pbf_move_left_joystick(context, {0, +1}, 2300ms, 200ms);
    // left to the trees
    pbf_move_left_joystick(context, {-1, 0}, 7800ms, 200ms);
    // down and over the ledge
    pbf_move_left_joystick(context, {0, -1}, 3000ms, 200ms);
    // left a couple of steps
    pbf_move_left_joystick(context, {-1, 0}, 600ms, 200ms);
    // up to into the grass
    pbf_move_left_joystick(context, {0, +1}, 1500ms, 500ms);
    context.wait_for_all_requests();
}

}
}
}
