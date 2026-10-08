/*  Pokemon FRLG AutoStory - Segment B6b
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
#include "PokemonFRLG_AutoStory_Segment_B6b.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;

namespace{

//  Multi-Pokemon trainer battle loop for Sabrina. Same shape as Koga's gym
//  fight in Segment_B5c (and Giovanni's in Segment_B6a) -- see
//  win_gym_battle() there for the canonical version of this pattern and
//  its caveats. Duplicated here (rather than shared) to keep each segment
//  file self-contained, matching this codebase's existing per-segment
//  anonymous-namespace convention.
void win_sabrina_battle(
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
                    "win_sabrina_battle(): the whole party fainted against Sabrina.",
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
                "win_sabrina_battle(): lead Pokemon ran out of PP on every priority move.",
                env.console
            );
        }

        //  opponentfainted: either Sabrina sends out her next Pokemon
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
            //  No further battle menu -- Sabrina is out of Pokemon.
            env.log("Sabrina defeated.");
            return;
        }
        //  Otherwise loop around and fight the next Pokemon.
    }
    OperationFailedException::fire(
        ErrorReport::SEND_ERROR_REPORT,
        "win_sabrina_battle(): exceeded the round cap without a battle result. Possible soft-lock -- stopping instead of looping forever.",
        env.console
    );
}

}  //  anonymous namespace


void run_B6b_sabrina_marsh_badge(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    env.log("B6b: Saffron Gym -> Sabrina -> Marsh Badge won", COLOR_BLUE);

    //  ---- Silph Co. to the Saffron Gym ----
    //  Short in-city hop; the overworld A* navigator handles the actual
    //  route, so no intermediate waypoints are hand-specified here.
    env.log("Heading to the Saffron Gym...");
    navigate_to(env, context, stats, KantoGoals::SaffronGymEntrance, &KantoGoals::SilphCoEntrance, 100);

    //  ---- Sabrina: the teleport-tile gym puzzle ----
    //  NOT SOLVED HERE. The Saffron Gym's floor is covered in warp tiles
    //  that teleport the player to other tiles on the same floor, forming
    //  a puzzle that must be solved (or walked via a memorized path) to
    //  reach Sabrina's battle tile; no such path has been captured against
    //  real hardware for this project yet, directly analogous to Koga's
    //  invisible-wall gym maze in Segment_B5c. The placeholder below just
    //  mashes forward -- on real hardware this will very likely bounce
    //  the player around the warp-tile network rather than soft-lock
    //  (stepping on a warp tile just relocates the player elsewhere on the
    //  same floor, it is not a crash risk), so this is flagged as a
    //  correctness gap rather than a crash risk. Replace with a captured
    //  step sequence (or a warp-tile-aware solver) before relying on this
    //  in a real run. Flagging as the segment's highest-risk section.
    enter_building(env, context, BuildingType::GYM);
    env.log("Saffron Gym: best-effort blind warp-tile walk (puzzle not solved)...", COLOR_ORANGE);
    for (int i = 0; i < 6; i++){
        pbf_move_left_joystick(context, {128, 0}, 1200ms, 200ms);
    }

    env.log("Battling Sabrina...");
    win_sabrina_battle(env, context, stats);

    //  ---- Marsh Badge + TM46 (Psywave) ----
    pbf_mash_button(context, BUTTON_B, 15s);   //  trainer-defeat dialogue
    pbf_mash_button(context, BUTTON_A, 15s);   //  Marsh Badge + TM46 handover
    context.wait_for_all_requests();
    env.log("Marsh Badge won. TM46 (Psywave) received.");

    exit_building(env, context, BuildingType::GYM);

    //  Segment ends here, matching this phase's registered end_text
    //  ("Marsh Badge won. Standing outside the Saffron Gym."). Cinnabar
    //  Island (Pokemon Mansion / Volcano Badge) is Phase::B7a/B7b's job
    //  (see PokemonFRLG_AutoStory.cpp's phase table), not this one.
}

}
}
}
