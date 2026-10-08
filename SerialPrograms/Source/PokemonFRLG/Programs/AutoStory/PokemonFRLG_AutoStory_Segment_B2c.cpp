/*  Pokemon FRLG AutoStory - Segment B2c
 *
 *  From: https://github.com/PokemonAutomation/
 */

#include "Common/Cpp/Color.h"
#include "CommonFramework/Exceptions/OperationFailedException.h"
#include "CommonTools/Async/InferenceRoutines.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_PushButtons.h"
#include "PokemonFRLG/PokemonFRLG_Navigation.h"
#include "PokemonFRLG/Programs/PokemonFRLG_KantoMapNavigator.h"
#include "PokemonFRLG/Inference/Map/PokemonFRLG_KantoGoals_Extended.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_DialogDetector.h"
#include "PokemonFRLG/Inference/PokemonFRLG_BattlePokemonDetector.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_BattleDialogs.h"
#include "PokemonFRLG_AutoStory_Segment_B2c.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;

namespace{

//  Fight one trainer/wild battle to completion. Declines any move-learn
//  offer (stop_on_move_learn=false, prevent_evolution=true) so the badge
//  hand-off dialog that follows a gym win isn't blocked behind an
//  unattended evolution/learn prompt.
void fight_one_battle(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    AutoStoryStats& stats
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
            "run_B2c_cerulean_gym(): the lead Pokemon fainted in the Cerulean Gym. "
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
            "run_B2c_cerulean_gym(): unrecognized battle result while fighting "
            "through the Cerulean Gym.",
            env.console
        );
        break;
    }
}

}  //  anonymous namespace


void run_B2c_cerulean_gym(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    (void)options;
    env.log("B2c: Mt. Moon exit -> Cerulean Gym -> Cascade Badge", COLOR_BLUE);

    //  Heal right after the Mt. Moon push, before the Gym.
    navigate_to(env, context, stats, KantoGoals::Route4PokeCenterEntrance);
    enter_building(env, context, BuildingType::POKECENTER);
    heal_at_pokecenter(env.console, context);
    exit_building(env, context, BuildingType::POKECENTER);

    //  Walk to the Cerulean Gym door.
    navigate_to(
        env, context, stats,
        KantoGoals::Extended::CeruleanCityGymDoor,
        &KantoGoals::Route4PokeCenterEntrance
    );

    //  Step inside. The Gym's interior is not part of the combined-map
    //  walkability image, so entry/exit and the trainer gauntlet are
    //  scripted -- first pass, not hardware-verified.
    env.log("Entering the Cerulean Gym.");
    {
        BlackScreenWatcher entry_fade(COLOR_RED);
        context.wait_for_all_requests();
        int ret = run_until<ProControllerContext>(
            env.console, context,
            [](ProControllerContext& context){
                for (int i = 0; i < 6; i++){
                    pbf_move_left_joystick(context, {128, 0}, 1000ms, 200ms);
                }
            },
            { entry_fade }
        );
        if (ret < 0){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "run_B2c_cerulean_gym(): did not detect the fade into the Cerulean Gym "
                "after walking north from the door. The gym entrance tile has not been "
                "hardware-verified -- check positioning.",
                env.console
            );
        }
        pbf_wait(context, 1000ms);
        context.wait_for_all_requests();
    }

    //  Fight across the gym. Cerulean Gym has one trainer (Picnicker) plus
    //  Misty -- walk forward in bursts, fighting whatever battle starts,
    //  for up to 2 encounters.
    constexpr int MAX_GYM_TRAINERS = 2;
    int trainers_beaten = 0;
    constexpr int MAX_BURSTS = 30;
    for (int burst = 0; burst < MAX_BURSTS && trainers_beaten < MAX_GYM_TRAINERS; burst++){
        BattleMenuWatcher battle_ready(COLOR_RED);
        context.wait_for_all_requests();
        int ret = run_until<ProControllerContext>(
            env.console, context,
            [](ProControllerContext& context){
                pbf_move_left_joystick(context, {128, 0}, 1500ms, 200ms);
            },
            { battle_ready }
        );
        if (ret == 0){
            fight_one_battle(env, context, stats);
            trainers_beaten++;
        }
    }
    if (trainers_beaten < MAX_GYM_TRAINERS){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "run_B2c_cerulean_gym(): did not find both expected Gym battles (Picnicker + "
            "Misty) within the step budget. The Gym's interior layout has not been "
            "hardware-verified -- check positioning/walk directions.",
            env.console
        );
    }

    //  Misty's win dialog hands over the Cascade Badge and TM11
    //  (BubbleBeam) automatically -- mash through it.
    env.log("Cascade Badge won -- clearing the TM11 hand-off dialog.");
    pbf_mash_button(context, BUTTON_A, 15s);

    //  Walk back out of the Gym.
    env.log("Exiting the Cerulean Gym.");
    {
        BlackScreenWatcher exit_fade(COLOR_RED);
        context.wait_for_all_requests();
        int ret = run_until<ProControllerContext>(
            env.console, context,
            [](ProControllerContext& context){
                for (int i = 0; i < 6; i++){
                    pbf_move_left_joystick(context, {128, 255}, 1000ms, 200ms);
                }
            },
            { exit_fade }
        );
        if (ret < 0){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "run_B2c_cerulean_gym(): did not detect the fade out of the Cerulean Gym "
                "after walking south. Check positioning.",
                env.console
            );
        }
        pbf_wait(context, 1000ms);
        context.wait_for_all_requests();
    }

    env.log("Cascade Badge won. Outside the Cerulean Gym.");
}

}
}
}
