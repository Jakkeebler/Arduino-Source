/*  Pokemon FRLG AutoStory - Segment B2b
 *
 *  From: https://github.com/PokemonAutomation/
 */

#include "Common/Cpp/Color.h"
#include "CommonFramework/Exceptions/OperationFailedException.h"
#include "CommonTools/Async/InferenceRoutines.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_PushButtons.h"
#include "PokemonFRLG/PokemonFRLG_Navigation.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_DialogDetector.h"
#include "PokemonFRLG/Inference/PokemonFRLG_BattlePokemonDetector.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_BattleDialogs.h"
#include "PokemonFRLG_AutoStory_Segment_B2b.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;

namespace{

//  Fight a single wild/trainer encounter to completion (one spam_first_move
//  pass) and record the win. Stops the program on a loss or unrecognized
//  result instead of guessing at recovery -- same policy as B0_05.
void fight_one_battle(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    AutoStoryStats& stats,
    const char* caller_name
){
    bool shiny = handle_encounter(env.console, context, true);
    (void)shiny;
    BattleResult result = spam_first_move(env.console, context);
    switch (result){
    case BattleResult::opponentfainted:
        track_battle_won(stats);
        exit_wild_battle(env.console, context, false, true);
        break;
    case BattleResult::playerfainted:
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            std::string(caller_name) + "(): the lead Pokemon fainted inside Mt. Moon. "
            "AutoStory does not yet handle in-battle switching for this segment -- "
            "stopping for manual recovery.",
            env.console
        );
        break;
    case BattleResult::outofpp:
    case BattleResult::unknown:
    default:
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            std::string(caller_name) + "(): unrecognized battle result while fighting "
            "through Mt. Moon.",
            env.console
        );
        break;
    }
}

//  Walk forward in bursts (zigzagging slightly to work around corridor
//  turns) until either a wild/trainer battle interrupts (fought to
//  completion, then the walk resumes) or a black-screen fade is seen
//  (floor transition or the mountain's exit), which ends the walk.
//  Throws if `max_bursts` is exhausted without seeing a fade.
void walk_until_fade(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    AutoStoryStats& stats,
    JoystickPosition direction,
    int max_bursts,
    const char* caller_name
){
    bool leftright = true;
    for (int burst = 0; burst < max_bursts; burst++){
        BattleMenuWatcher battle_ready(COLOR_RED);
        BlackScreenWatcher fade(COLOR_RED);
        context.wait_for_all_requests();
        int ret = run_until<ProControllerContext>(
            env.console, context,
            [&](ProControllerContext& context){
                pbf_move_left_joystick(context, direction, 2000ms, 200ms);
            },
            { battle_ready, fade }
        );

        if (ret == 1){
            pbf_wait(context, 1000ms);
            context.wait_for_all_requests();
            return;
        }
        if (ret == 0){
            fight_one_battle(env, context, stats, caller_name);
            leftright = !leftright;
            continue;
        }

        //  Neither watcher fired -- nudge sideways once to work around a
        //  corridor turn, then keep trying the main direction.
        pbf_move_left_joystick(context, leftright ? JoystickPosition{255, 128} : JoystickPosition{0, 128}, 500ms, 200ms);
    }

    OperationFailedException::fire(
        ErrorReport::SEND_ERROR_REPORT,
        std::string(caller_name) + "(): exhausted the step budget without finding the "
        "expected fade. This segment's traversal is a generic forward-walk first pass "
        "-- Mt. Moon's real layout has not been captured on hardware yet. Needs a real "
        "path before this can run unattended.",
        env.console
    );
}

}  //  anonymous namespace


void run_B2b_mtmoon_traversal(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    (void)options;
    env.log("B2b: Mt. Moon traversal", COLOR_BLUE);

    //  Step onto the 1F door to enter the cave, then push north across 1F
    //  to the stairs down to B1F.
    env.log("Entering Mt. Moon 1F.");
    walk_until_fade(env, context, stats, {128, 0}, 40, "run_B2b_mtmoon_traversal");

    //  On B1F now. Push toward the fossil room first.
    env.log("On Mt. Moon B1F, heading for the fossil room.");
    walk_until_fade(env, context, stats, {128, 0}, 20, "run_B2b_mtmoon_traversal");

    //  Fossil pickup: walk up to the table and interact. The choice menu
    //  defaults to the top option; mash A to accept it. Fixed choice
    //  (Helix Fossil) -- not currently user-configurable.
    env.log("Picking up a fossil (defaulting to Helix Fossil).");
    pbf_move_left_joystick(context, {128, 0}, 1000ms, 200ms);
    pbf_press_button(context, BUTTON_A, 200ms, 1000ms);
    pbf_mash_button(context, BUTTON_A, 5s);

    //  Continue across B1F to the Route 4 exit.
    env.log("Heading for the Route 4 exit.");
    walk_until_fade(env, context, stats, {255, 128}, 40, "run_B2b_mtmoon_traversal");

    pbf_wait(context, 1000ms);
    context.wait_for_all_requests();
    env.log("Exited Mt. Moon onto Route 4.");
}

}
}
}
