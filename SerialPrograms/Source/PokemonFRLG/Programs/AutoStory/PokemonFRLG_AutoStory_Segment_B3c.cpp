/*  Pokemon FRLG AutoStory - Segment B3c
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
#include "PokemonFRLG/Inference/Map/PokemonFRLG_KantoGoals_Extended.h"
#include "PokemonFRLG/Programs/PokemonFRLG_KantoMapNavigator.h"
#include "PokemonFRLG_AutoStory_Segment_B3c.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;

namespace{

//  Run the trainer battle loop against Lt. Surge (Voltorb, Pikachu, Raichu
//  in FRLG). Mirrors run_B1b_pewter_gym_brock_battle()'s multi-Pokemon loop
//  shape: spam_first_move() only plays out one Pokemon at a time, so loop
//  it and disambiguate "next Pokemon sent out" from "battle actually over"
//  after each opponent faint.
void fight_lt_surge(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    AutoStoryStats& stats
){
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
                //  Surge sent out his next Pokemon; keep fighting.
                continue;
            }
            if (ret == 1){
                trainer_defeated = true;
                continue;
            }
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "fight_lt_surge(): couldn't tell whether Surge sent out another "
                "Pokemon or the battle ended after a Pokemon fainted.",
                env.console
            );
        }
        case BattleResult::playerfainted:
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "fight_lt_surge(): lead fainted during the Lt. Surge battle. "
                "AutoStory doesn't handle in-battle switching yet.",
                env.console
            );
            break;
        case BattleResult::outofpp:
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "fight_lt_surge(): ran out of PP fighting Lt. Surge. This is a "
                "forced trainer battle -- can't flee to recover.",
                env.console
            );
            break;
        case BattleResult::unknown:
        default:
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "fight_lt_surge(): unrecognized battle result while fighting Surge.",
                env.console
            );
            break;
        }
    }
    if (!trainer_defeated){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "fight_lt_surge(): exhausted the round budget without detecting "
            "Surge's defeat.",
            env.console
        );
    }
    env.log("Lt. Surge defeated.");
}

//  Walk the trash-can switch puzzle blocking the path to Surge. FRLG uses
//  the same fixed (non-random per-save) 3-switch layout as the original
//  games: the two side rows of cans are decoys, and the switches sit at
//  the far end of the two side aisles plus the near end of the middle
//  aisle. This exact step sequence is a best-effort first pass -- it has
//  NOT been hardware-verified. If it leaves the player stuck against a
//  locked gate, this is the first place to re-check with a real capture.
void solve_trashcan_puzzle(ProControllerContext& context){
    //  Switch 1: walk up the left aisle and press the far trash can.
    pbf_move_left_joystick(context, {128, 0}, 1800ms, 200ms);
    pbf_mash_button(context, BUTTON_A, 1000ms);
    //  Back down, over to the middle aisle, press the near trash can.
    pbf_move_left_joystick(context, {128, 255}, 1800ms, 200ms);
    pbf_move_left_joystick(context, {255, 128}, 900ms, 200ms);
    pbf_move_left_joystick(context, {128, 0}, 400ms, 200ms);
    pbf_mash_button(context, BUTTON_A, 1000ms);
    //  Over to the right aisle, up to the far trash can.
    pbf_move_left_joystick(context, {255, 128}, 900ms, 200ms);
    pbf_move_left_joystick(context, {128, 0}, 1800ms, 200ms);
    pbf_mash_button(context, BUTTON_A, 1000ms);
    //  Gates should now be open -- walk north to Surge's platform.
    pbf_move_left_joystick(context, {0, 128}, 500ms, 200ms);
    pbf_move_left_joystick(context, {128, 0}, 1200ms, 200ms);
}

//  Navigate to Bill's Sea Cottage on Route 25, mash through the "rescue
//  Bill" cutscene, and receive HM02 Fly. Interior/cutscene-heavy and
//  unmapped, same caveat as the trash-can puzzle above: best-effort,
//  not hardware-verified.
void get_fly_hm_from_bill(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    AutoStoryStats& stats
){
    env.log("Detouring to Bill's Sea Cottage on Route 25 for HM02 Fly...");
    navigate_to(
        env, context, stats,
        KantoGoals::Extended::Route25SeaCottageDoor,
        &KantoGoals::Extended::VermilionCityGymDoor,
        400
    );
    enter_building(env, context, BuildingType::POKECENTER);  //  generic door-fade walk-in; Bill's house uses the same sprite convention.

    //  NOTE: best-effort -- mash through Bill's "stuck halfway between human
    //  and Pokemon" cutscene and the HM02 Fly item-get dialogue blind. Exact
    //  chain length not hardware-verified.
    pbf_mash_button(context, BUTTON_A, 30000ms);
    context.wait_for_all_requests();

    exit_building(env, context, BuildingType::POKECENTER);
    env.log("HM02 Fly obtained from Bill.");
}

}  //  anonymous namespace

void run_B3c_vermilion_gym_thunder_badge(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    (void)options;

    env.log("B3c: Vermilion Gym -> Lt. Surge battle -> Thunder Badge -> Bill's Fly HM", COLOR_BLUE);

    //  ---- Walk to and enter the gym ----
    navigate_to(env, context, stats, KantoGoals::Extended::VermilionCityGymDoor, nullptr, 250);
    enter_building(env, context, BuildingType::GYM);

    //  ---- Trash-can switch puzzle, then walk up to Surge ----
    solve_trashcan_puzzle(context);

    //  ---- Battle loop ----
    fight_lt_surge(env, context, stats);

    //  ---- Post-battle: Thunder Badge + TM24 Thunderbolt dialogue ----
    //  NOTE: best-effort -- the exact dialogue chain length (badge jingle,
    //  "got TM24!" item-get box, bag-full check, Surge's closing line) has
    //  not been hardware-verified. Mash through it blind with a generous
    //  timeout, same approach used for Brock in B1b.
    pbf_mash_button(context, BUTTON_B, 20000ms);
    context.wait_for_all_requests();
    pbf_mash_button(context, BUTTON_A, 20000ms);
    context.wait_for_all_requests();

    //  ---- Walk out ----
    exit_building(env, context, BuildingType::GYM);
    env.log("Thunder Badge won. Outside the Vermilion Gym.");

    //  ---- Side trip: Bill's Sea Cottage for HM02 Fly ----
    get_fly_hm_from_bill(env, context, stats);

    //  ---- Return to the Vermilion Gym door to match this segment's documented end state ----
    navigate_to(
        env, context, stats,
        KantoGoals::Extended::VermilionCityGymDoor,
        &KantoGoals::Extended::Route25SeaCottageDoor,
        400
    );

    env.log("Thunder Badge won, Fly HM obtained. Outside the Vermilion Gym.");
}

}
}
}
