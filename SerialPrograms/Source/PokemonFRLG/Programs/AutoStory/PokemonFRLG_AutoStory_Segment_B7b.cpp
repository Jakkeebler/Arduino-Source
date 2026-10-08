/*  Pokemon FRLG AutoStory - Segment B7b
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
#include "PokemonFRLG_AutoStory_Segment_B7b.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;

namespace{

//  Multi-Pokemon trainer battle loop (Blaine fields 4 in FRLG: Growlithe,
//  Ponyta, Rapidash, Arcanine). Same shape as Segment_B5c's win_gym_battle()
//  for Koga: keeps using the lead's first move via spam_first_move() each
//  time the battle menu comes back; forces in the next party slot on a
//  player faint; stops when the battle menu doesn't return after an
//  opponent faint (gym leader fully defeated) or the party runs out of
//  healthy Pokemon.
//
//  Best-effort / not hardware-verified: Blaine's team is all Fire-type and
//  can do heavy damage to an unprepared party; this always just re-presses
//  the first move slot, matching the same simplification already used for
//  Koga. Good enough to not soft-lock; not guaranteed to be
//  move-type-optimal (bring a Water/Rock/Ground move on the lead for a real
//  run).
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
                    "win_gym_battle(): the whole party fainted against Blaine.",
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

        //  opponentfainted: either Blaine sends out their next Pokemon
        //  (battle menu returns) or the whole battle is over.
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
            //  No further battle menu -- Blaine is out of Pokemon.
            env.log("Blaine defeated.");
            return;
        }
        //  Otherwise loop around and fight the next Pokemon.
    }
    OperationFailedException::fire(
        ErrorReport::SEND_ERROR_REPORT,
        "win_gym_battle(): exceeded the round cap without a battle result. Possible soft-lock -- stopping instead of looping forever.",
        env.console
    );
}

}  //  anonymous namespace


void run_B7b_blaine_volcano_badge(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    (void)options;
    env.log("B7b: Cinnabar Gym -> Blaine -> Volcano Badge + TM38", COLOR_BLUE);

    //  ---- Walk in ----
    enter_building(env, context, BuildingType::GYM);

    //  ---- Cross the sliding-floor puzzle to Blaine ----
    //  NOT SOLVED HERE, matching Segment_B5c's explicit call-out for Koga's
    //  invisible-wall maze. Cinnabar Gym's floor is a grid of one-way
    //  sliding warp tiles (unlocked by the Secret Key from Segment_B7a) that
    //  must be crossed via a specific tile path; no such path has been
    //  captured against real hardware for this project yet. The placeholder
    //  below just mashes forward -- sliding tiles auto-walk the player once
    //  stepped on, so repeated forward input is at least directionally
    //  correct, but this is flagged as a correctness gap rather than a
    //  crash risk. Replace with a captured step sequence (or a sliding-tile
    //  -aware solver) before relying on this in a real run.
    for (int i = 0; i < 8; i++){
        pbf_move_left_joystick(context, {128, 0}, 1200ms, 200ms);
    }

    env.log("Battling Blaine...");
    {
        BattleMenuWatcher battle_ready(COLOR_RED);
        context.wait_for_all_requests();
        int ret = run_until<ProControllerContext>(
            env.console, context,
            [](ProControllerContext& context){
                pbf_mash_button(context, BUTTON_A, 15s);
            },
            { battle_ready }
        );
        if (ret < 0){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "run_B7b_blaine_volcano_badge(): the Blaine battle never started.",
                env.console
            );
        }
    }
    win_gym_battle(env, context, stats);

    //  ---- Volcano Badge + TM38 (Fire Blast) ----
    pbf_mash_button(context, BUTTON_B, 15s);   //  trainer-defeat dialogue
    pbf_mash_button(context, BUTTON_A, 15s);   //  Volcano Badge + TM38 handover
    context.wait_for_all_requests();
    env.log("Volcano Badge won. TM38 (Fire Blast) received.");

    //  ---- Walk out ----
    exit_building(env, context, BuildingType::GYM);

    env.log("Volcano Badge won. Outside the Cinnabar Gym.");
}

}
}
}
