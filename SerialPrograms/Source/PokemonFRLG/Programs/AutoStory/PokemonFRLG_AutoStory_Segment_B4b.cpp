/*  Pokemon FRLG AutoStory - Segment B4b
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
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_BattleDialogs.h"
#include "PokemonFRLG_AutoStory_Segment_B4b.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;

namespace{

//  Walk straight onto a door tile and wait for the black-screen fade that
//  signals the interior transition. direction_up=true walks north (entering
//  a building from its south-facing door); false walks south (leaving,
//  starting from just inside/above the door). Mirrors the shape of
//  enter_leave_pokecenter() in PokemonFRLG_Navigation.cpp, but generalized
//  here since no generic gym-entry primitive exists yet in
//  PokemonFRLG_AutoStoryTools' BuildingType enum (POKECENTER only).
void walk_through_door(SingleSwitchProgramEnvironment& env, ProControllerContext& context, bool direction_up){
    BlackScreenWatcher transition(COLOR_RED);
    context.wait_for_all_requests();
    int ret = run_until<ProControllerContext>(
        env.console, context,
        [direction_up](ProControllerContext& context){
            pbf_move_left_joystick(context, direction_up ? JoystickPosition{128, 0} : JoystickPosition{128, 255}, 10000ms, 0ms);
        },
        { transition }
    );
    if (ret < 0){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "run_B4b_lavender_to_celadon_erika(): failed to detect the door transition "
            "fade while walking through the Celadon Gym door.",
            env.console
        );
    }
    pbf_wait(context, 2500ms);
    context.wait_for_all_requests();
}

}  //  anonymous namespace


void run_B4b_lavender_to_celadon_erika(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    (void)options;

    env.log("B4b: Lavender Town -> Celadon City -> Erika -> Rainbow Badge", COLOR_BLUE);

    //  ---- Walk from Lavender Town to the Celadon Gym door ----
    //  Route 7 and Celadon City are both covered by the combined Kanto map
    //  (FRO-176), so this is a single A*-pathed walk, same shape as B4a.
    //  Dept Store / Game Corner are intentionally skipped here -- see the
    //  scope note in PokemonFRLG_AutoStory_Segment_B4b.h.
    navigate_to(
        env, context, stats,
        KantoGoals::Extended::CeladonCityGymDoor,
        &KantoGoals::LavenderPokeCenterEntrance,
        400
    );

    //  ---- Walk in ----
    walk_through_door(env, context, true);

    //  ---- Walk up to Erika ----
    //  NOTE: best-effort -- the exact step count to reach Erika's sightline
    //  has not been hardware-verified. Celadon Gym in FRLG is a grass-maze
    //  floor (cuttable trees block some paths pre-Cut, but the direct north
    //  lane to the gym leader does not require Cut), so a straight walk
    //  north is the right shape; retune after the first real run, same
    //  caveat as B1b's walk to Brock.
    for (int i = 0; i < 12; i++){
        pbf_move_left_joystick(context, {128, 0}, 500ms, 200ms);
    }

    //  ---- Battle loop ----
    //  Erika fields three Pokemon in FRLG (Victreebel, Tangela, Vileplume).
    //  spam_first_move() only plays out a single Pokemon's worth of battle,
    //  so loop it and, after each opponent faint, check whether Erika sent
    //  out her next Pokemon (battle continues) or the battle actually ended
    //  (Erika fully defeated). Same pattern as B1b's Brock battle.
    bool trainer_defeated = false;
    for (int round = 0; round < 10 && !trainer_defeated; round++){
        BattleResult result = spam_first_move(env.console, context);
        switch (result){
        case BattleResult::opponentfainted:
        {
            track_battle_won(stats);
            BattleMenuWatcher next_mon_menu(COLOR_RED);
            BlackScreenWatcher battle_over(COLOR_RED);
            context.wait_for_all_requests();
            int ret = run_until<ProControllerContext>(
                env.console, context,
                [](ProControllerContext& context){
                    pbf_mash_button(context, BUTTON_B, 20000ms);
                },
                { next_mon_menu, battle_over }
            );
            if (ret == 0){
                //  Erika sent out her next Pokemon; keep fighting.
                continue;
            }
            if (ret == 1){
                trainer_defeated = true;
                continue;
            }
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "run_B4b_lavender_to_celadon_erika(): couldn't tell whether Erika sent "
                "out another Pokemon or the battle ended after a Pokemon fainted.",
                env.console
            );
        }
        case BattleResult::playerfainted:
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "run_B4b_lavender_to_celadon_erika(): lead fainted during the Erika "
                "battle. AutoStory doesn't handle in-battle switching yet.",
                env.console
            );
            break;
        case BattleResult::outofpp:
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "run_B4b_lavender_to_celadon_erika(): ran out of PP fighting Erika. "
                "This is a forced trainer battle -- can't flee to recover.",
                env.console
            );
            break;
        case BattleResult::unknown:
        default:
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "run_B4b_lavender_to_celadon_erika(): unrecognized battle result "
                "while fighting Erika.",
                env.console
            );
            break;
        }
    }
    if (!trainer_defeated){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "run_B4b_lavender_to_celadon_erika(): exhausted the round budget without "
            "detecting Erika's defeat.",
            env.console
        );
    }
    env.log("Erika defeated.");

    //  ---- Post-battle: Rainbow Badge + TM19 Giga Drain dialogue ----
    //  NOTE: best-effort -- the exact dialogue chain length (badge jingle,
    //  "got TM19!" item-get box, bag-full check, Erika's closing line) has
    //  not been hardware-verified. Mash through it blind with a generous
    //  timeout, same approach used for the Boulder Badge + TM39 chain in
    //  B1b.
    pbf_mash_button(context, BUTTON_B, 20000ms);
    context.wait_for_all_requests();
    pbf_mash_button(context, BUTTON_A, 20000ms);
    context.wait_for_all_requests();

    //  ---- Walk out ----
    walk_through_door(env, context, false);

    env.log("Rainbow Badge won. Outside the Celadon Gym.");
}

}
}
}
