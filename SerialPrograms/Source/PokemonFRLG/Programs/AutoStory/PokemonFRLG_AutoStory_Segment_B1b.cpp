/*  Pokemon FRLG AutoStory - Segment B1b
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
#include "PokemonFRLG_AutoStory_Segment_B1b.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;

void run_B1b_pewter_gym_brock_battle(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    (void)options;

    env.log("B1b: Pewter Gym -> Brock battle -> Boulder Badge", COLOR_BLUE);

    //  ---- Walk in ----
    enter_building(env, context, BuildingType::GYM);

    //  ---- Walk up to Brock ----
    //  NOTE: best-effort -- the exact step count to reach Brock's sightline
    //  has not been hardware-verified. Pewter Gym's floor is open (no
    //  boulder puzzle in FRLG), so a straight walk north is the right shape;
    //  retune the step count after the first real run.
    for (int i = 0; i < 10; i++){
        pbf_move_left_joystick(context, {128, 0}, 500ms, 200ms);
    }

    //  ---- Battle loop ----
    //  Brock fields two Pokemon (Geodude, Onix in FRLG). spam_first_move()
    //  only plays out a single Pokemon's worth of battle (it returns as soon
    //  as either side's current Pokemon faints), so loop it and, after each
    //  opponent faint, check whether Brock sent out his next Pokemon (battle
    //  continues) or the battle actually ended (Brock fully defeated).
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
                //  Brock sent out his next Pokemon; keep fighting.
                continue;
            }
            if (ret == 1){
                trainer_defeated = true;
                continue;
            }
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "run_B1b_pewter_gym_brock_battle(): couldn't tell whether Brock sent "
                "out another Pokemon or the battle ended after a Pokemon fainted.",
                env.console
            );
        }
        case BattleResult::playerfainted:
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "run_B1b_pewter_gym_brock_battle(): lead fainted during the Brock "
                "battle. AutoStory doesn't handle in-battle switching yet.",
                env.console
            );
            break;
        case BattleResult::outofpp:
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "run_B1b_pewter_gym_brock_battle(): ran out of PP fighting Brock. "
                "This is a forced trainer battle -- can't flee to recover.",
                env.console
            );
            break;
        case BattleResult::unknown:
        default:
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "run_B1b_pewter_gym_brock_battle(): unrecognized battle result "
                "while fighting Brock.",
                env.console
            );
            break;
        }
    }
    if (!trainer_defeated){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "run_B1b_pewter_gym_brock_battle(): exhausted the round budget without "
            "detecting Brock's defeat.",
            env.console
        );
    }
    env.log("Brock defeated.");

    //  ---- Post-battle: Boulder Badge + TM39 Rock Tomb dialogue ----
    //  NOTE: best-effort -- the exact dialogue chain length (badge jingle,
    //  "got TM39!" item-get box, bag-full check, Brock's closing line) has
    //  not been hardware-verified. Mash through it blind with a generous
    //  timeout, same approach used for the rival battle in B0_02.
    pbf_mash_button(context, BUTTON_B, 20000ms);
    context.wait_for_all_requests();
    pbf_mash_button(context, BUTTON_A, 20000ms);
    context.wait_for_all_requests();

    //  ---- Walk out ----
    exit_building(env, context, BuildingType::GYM);

    env.log("Boulder Badge won. Outside the Pewter Gym.");
}

}
}
}
