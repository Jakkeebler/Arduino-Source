/*  Pokemon FRLG AutoStory - Segment B8b
 *
 *  From: https://github.com/PokemonAutomation/
 */

#include "Common/Cpp/Color.h"
#include "CommonFramework/Exceptions/OperationFailedException.h"
#include "CommonTools/Async/InferenceRoutines.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_PushButtons.h"
#include "PokemonFRLG/PokemonFRLG_Navigation.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_DialogDetector.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_BattleDialogs.h"
#include "PokemonFRLG/Programs/PokemonFRLG_KantoMapNavigator.h"
#include "PokemonFRLG/Inference/Map/PokemonFRLG_KantoGoals_Extended.h"
#include "PokemonFRLG_AutoStory_Segment_B8b.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;

namespace{

//  Multi-Pokemon trainer battle loop (Giovanni fields multiple Pokemon in
//  FRLG). Same simplification as win_gym_battle() in Segment_B5c -- always
//  re-presses the first move slot; good enough to not soft-lock, not
//  guaranteed to be move-type-optimal.
void win_gym_battle(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    AutoStoryStats& stats
){
    int next_switch_slot = 2;
    for (int round = 0; round < 12; round++){  //  hard cap: never spin forever
        BattleResult result = spam_first_move(env.console, context);

        if (result == BattleResult::playerfainted){
            if (next_switch_slot > 6){
                OperationFailedException::fire(
                    ErrorReport::SEND_ERROR_REPORT,
                    "win_gym_battle(): the whole party fainted against Giovanni.",
                    env.console
                );
            }
            env.log("Player's Pokemon fainted. Switching in party slot " + std::to_string(next_switch_slot) + "...");
            select_forced_switch_slot(env.console, context, next_switch_slot);
            next_switch_slot++;
            continue;
        }

        if (result == BattleResult::outofpp){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "win_gym_battle(): lead Pokemon ran out of PP on every priority move.",
                env.console
            );
        }

        track_battle_won(stats);
        BattleMenuWatcher next_mon_ready(COLOR_RED);
        context.wait_for_all_requests();
        int ret = run_until<ProControllerContext>(
            env.console, context,
            [](ProControllerContext& context){
                pbf_mash_button(context, BUTTON_B, 10s);
            },
            { next_mon_ready }
        );
        if (ret < 0){
            env.log("Giovanni defeated.");
            return;
        }
    }
    OperationFailedException::fire(
        ErrorReport::SEND_ERROR_REPORT,
        "win_gym_battle(): exceeded the round cap without a battle result. Possible soft-lock -- stopping instead of looping forever.",
        env.console
    );
}

}  //  anonymous namespace


void run_B8b_viridian_gym(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    env.log("B8b: Viridian Gym -> Giovanni -> Earth Badge", COLOR_BLUE);

    //  ---- Travel to the Viridian Gym ----
    env.log("Heading for the Viridian Gym...");
    navigate_to(env, context, stats, KantoGoals::Extended::ViridianCityGymDoor, &KantoGoals::ViridianPokeCenterEntrance, 300);

    //  ---- Walk the hidden-warp-tile maze to Giovanni ----
    //  NOT SOLVED HERE -- see the header comment. Stepping on a wrong warp
    //  tile relocates the player rather than soft-locking, so this is a
    //  correctness gap, not a crash risk.
    enter_building(env, context, BuildingType::GYM);
    for (int i = 0; i < 6; i++){
        pbf_move_left_joystick(context, {128, 0}, 1200ms, 200ms);
    }

    env.log("Battling Giovanni...");
    win_gym_battle(env, context, stats);

    //  ---- Earth Badge + TM27 (Fissure) ----
    pbf_mash_button(context, BUTTON_B, 15s);   //  trainer-defeat dialogue
    pbf_mash_button(context, BUTTON_A, 15s);   //  Earth Badge + TM27 handover
    context.wait_for_all_requests();
    env.log("Earth Badge won. TM27 (Fissure) received. All 8 badges obtained.");

    exit_building(env, context, BuildingType::POKECENTER);

    context.wait_for_all_requests();
}

}
}
}
