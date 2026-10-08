/*  Pokemon FRLG AutoStory - Segment B0_05
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
#include "PokemonFRLG_AutoStory_Segment_B0_05.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;

void run_B0_05_viridian_forest_exit(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    env.log("B0_05: Viridian Forest -> Route 2 exit", COLOR_BLUE);

    constexpr int MAX_BURSTS = 60;
    bool leftright = true;

    for (int burst = 0; burst < MAX_BURSTS; burst++){
        //  Walk north for a bit, bailing out early into a wild battle if one
        //  starts, or out to the exit fade if the forest ends here.
        BattleMenuWatcher battle_ready(COLOR_RED);
        BlackScreenWatcher exit_fade(COLOR_RED);
        context.wait_for_all_requests();
        int ret = run_until<ProControllerContext>(
            env.console, context,
            [](ProControllerContext& context){
                pbf_move_left_joystick(context, {128, 0}, 2000ms, 200ms);
            },
            { battle_ready, exit_fade }
        );

        if (ret == 1){
            //  Exited the forest.
            pbf_wait(context, 1000ms);
            context.wait_for_all_requests();
            env.log("Exited Viridian Forest onto Route 2.");
            return;
        }
        if (ret == 0){
            //  Wild encounter triggered mid-walk.
            bool shiny = handle_encounter(env.console, context, true);
            (void)shiny;  //  AutoStory doesn't special-case shinies here.
            BattleResult result = spam_first_move(env.console, context);
            switch (result){
            case BattleResult::opponentfainted:
                track_battle_won(stats);
                exit_wild_battle(env.console, context, false, true);
                break;
            case BattleResult::playerfainted:
                OperationFailedException::fire(
                    ErrorReport::SEND_ERROR_REPORT,
                    "run_B0_05_viridian_forest_exit(): the lead Pokemon fainted in "
                    "Viridian Forest. AutoStory does not yet handle in-battle "
                    "switching for this segment -- stopping for manual recovery.",
                    env.console
                );
                break;
            case BattleResult::outofpp:
            case BattleResult::unknown:
            default:
                OperationFailedException::fire(
                    ErrorReport::SEND_ERROR_REPORT,
                    "run_B0_05_viridian_forest_exit(): unrecognized battle result "
                    "while fighting through Viridian Forest.",
                    env.console
                );
                break;
            }
            leftright = !leftright;
            continue;
        }

        //  Neither watcher fired within the burst -- no encounter, forest
        //  continues. If movement is blocked by a wall/corridor turn, nudge
        //  sideways once before trying north again.
        pbf_move_left_joystick(context, leftright ? JoystickPosition{255, 128} : JoystickPosition{0, 128}, 500ms, 200ms);
    }

    OperationFailedException::fire(
        ErrorReport::SEND_ERROR_REPORT,
        "run_B0_05_viridian_forest_exit(): exhausted the step budget without finding "
        "the Viridian Forest exit. This segment's traversal is a generic north-walk "
        "first pass -- Viridian Forest's real zigzag layout has not been captured on "
        "hardware yet. Needs a real path (or map-based navigation once the combined "
        "map asset covers the forest interior) before this can run unattended.",
        env.console
    );
}

}
}
}
