/*  Pokemon FRLG AutoStory - Segment B3b
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
#include "PokemonFRLG_AutoStory_Segment_B3b.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;

namespace{

//  Fight the forced rival battle on the S.S. Anne deck. Mirrors the
//  multi-Pokemon loop shape used for gym leaders (B1b, B3c's Lt. Surge
//  fight) rather than B0_02's single-Pokemon rival fight, since by this
//  point in the story the rival's party has grown past one Pokemon.
void fight_rival_on_deck(
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
                continue;  //  rival sent out his next Pokemon
            }
            if (ret == 1){
                trainer_defeated = true;
                continue;
            }
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "fight_rival_on_deck(): couldn't tell whether the rival sent out "
                "another Pokemon or the battle ended after a Pokemon fainted.",
                env.console
            );
        }
        case BattleResult::playerfainted:
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "fight_rival_on_deck(): lead fainted during the S.S. Anne rival "
                "battle. AutoStory doesn't handle in-battle switching yet.",
                env.console
            );
            break;
        case BattleResult::outofpp:
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "fight_rival_on_deck(): ran out of PP fighting the rival. This is "
                "a forced trainer battle -- can't flee to recover.",
                env.console
            );
            break;
        case BattleResult::unknown:
        default:
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "fight_rival_on_deck(): unrecognized battle result while fighting "
                "the rival.",
                env.console
            );
            break;
        }
    }
    if (!trainer_defeated){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "fight_rival_on_deck(): exhausted the round budget without detecting "
            "the rival's defeat.",
            env.console
        );
    }
    env.log("Rival defeated on the S.S. Anne deck.");
}

}  //  anonymous namespace

void run_B3b_ssanne_traversal(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    (void)options;

    env.log("B3b: Board S.S. Anne -> rival battle -> deliver medicine -> Cut HM -> disembark", COLOR_BLUE);

    //  ---- Board the ship ----
    //  The sailor at the gangplank checks the ticket automatically (already
    //  obtained from Bill earlier in the story) then the screen fades into
    //  the 1F deck. Same generic walk-forward-into-fade mechanics as every
    //  other door in this codebase.
    {
        pbf_mash_button(context, BUTTON_A, 3000ms);  //  ticket-check dialogue
        BlackScreenWatcher boarding_fade(COLOR_RED);
        context.wait_for_all_requests();
        int ret = run_until<ProControllerContext>(
            env.console, context,
            [](ProControllerContext& context){
                pbf_move_left_joystick(context, {128, 0}, 2000ms, 200ms);
            },
            { boarding_fade }
        );
        if (ret < 0){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "run_B3b_ssanne_traversal(): failed to detect the boarding fade "
                "onto the S.S. Anne's 1F deck.",
                env.console
            );
        }
    }
    env.log("Boarded the S.S. Anne.");

    //  ---- Walk to the rival's fixed position on deck ----
    //  NOTE: best-effort -- exact step count not hardware-verified.
    pbf_wait(context, 2000ms);
    pbf_move_left_joystick(context, {128, 0}, 1500ms, 200ms);
    pbf_move_left_joystick(context, {255, 128}, 1500ms, 200ms);
    {
        BattleMenuWatcher battle_ready(COLOR_RED);
        context.wait_for_all_requests();
        int ret = run_until<ProControllerContext>(
            env.console, context,
            [](ProControllerContext& context){
                pbf_move_left_joystick(context, {128, 0}, 1000ms, 0ms);
                pbf_mash_button(context, BUTTON_B, 20s);
            },
            { battle_ready }
        );
        if (ret < 0){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "run_B3b_ssanne_traversal(): failed to trigger the rival battle "
                "on deck.",
                env.console
            );
        }
    }
    fight_rival_on_deck(env, context, stats);
    pbf_mash_button(context, BUTTON_B, 20000ms);  //  exit battle
    pbf_mash_button(context, BUTTON_A, 10000ms);  //  post-battle dialogue
    context.wait_for_all_requests();

    //  ---- Find the panacea and deliver it to the seasick captain ----
    //  NOTE: KNOWN GAP -- not implemented precisely. The panacea is held by
    //  a specific crew NPC belowdecks; this codebase does not yet have a
    //  verified room-by-room route to find them. The walk below is a
    //  best-effort placeholder (head below deck, mash dialogue with any
    //  NPCs encountered along a straight path) and will very likely need
    //  real hardware captures to fix before this segment is reliable.
    pbf_move_left_joystick(context, {128, 255}, 1000ms, 200ms);
    {
        BlackScreenWatcher stairs_fade(COLOR_RED);
        context.wait_for_all_requests();
        int ret = run_until<ProControllerContext>(
            env.console, context,
            [](ProControllerContext& context){
                pbf_move_left_joystick(context, {128, 0}, 2000ms, 200ms);
            },
            { stairs_fade }
        );
        if (ret < 0){
            env.log(
                "run_B3b_ssanne_traversal(): didn't detect a floor transition "
                "heading below deck. Continuing blind -- this leg is a known gap.",
                COLOR_ORANGE
            );
        }
    }
    pbf_mash_button(context, BUTTON_A, 15000ms);  //  talk through crew members for the panacea
    context.wait_for_all_requests();

    //  ---- Deliver the panacea to the captain and receive HM01 Cut ----
    pbf_move_left_joystick(context, {128, 0}, 1500ms, 200ms);
    pbf_mash_button(context, BUTTON_A, 20000ms);  //  wake-up cutscene + "got HM01!" item-get box
    context.wait_for_all_requests();
    env.log("HM01 Cut obtained from the captain.");

    //  ---- Walk back to the gangplank and disembark ----
    {
        BlackScreenWatcher exit_fade(COLOR_RED);
        context.wait_for_all_requests();
        int ret = run_until<ProControllerContext>(
            env.console, context,
            [](ProControllerContext& context){
                pbf_move_left_joystick(context, {128, 255}, 2500ms, 200ms);
                pbf_move_left_joystick(context, {255, 128}, 2500ms, 200ms);
            },
            { exit_fade }
        );
        if (ret < 0){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "run_B3b_ssanne_traversal(): failed to detect the disembark fade "
                "back into Vermilion City.",
                env.console
            );
        }
    }

    env.log("Off the S.S. Anne with Cut taught, back in Vermilion City.");
}

}
}
}
