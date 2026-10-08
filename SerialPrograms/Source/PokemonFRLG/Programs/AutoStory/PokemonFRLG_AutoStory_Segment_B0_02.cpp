/*  Pokemon FRLG AutoStory - Segment B0_02
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
#include "PokemonFRLG_AutoStory_Segment_B0_02.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;

namespace{

//  Mirrors StarterRng::walk_to_rival_battle()'s per-starter step count --
//  the rival's blocking position relative to the lab doorway shifts with
//  which starter table slot the player approached.
int steps_to_the_left_for_rival(StarterChoice starter){
    switch (starter){
    case StarterChoice::Bulbasaur:  return 2;
    case StarterChoice::Squirtle:   return 3;
    case StarterChoice::Charmander: return 4;
    }
    return 2;
}

}  //  anonymous namespace

void run_B0_02_rival_battle_1(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    env.log("B0_02: Rival Battle 1", COLOR_BLUE);

    //  ---- Exit the lab and walk into the rival ----
    pbf_mash_button(context, BUTTON_B, 5000ms);  //  back to the overworld

    pbf_move_left_joystick(context, {128, 255}, 40ms, 460ms);
    pbf_move_left_joystick(context, {128, 255}, 100ms, 400ms);

    pbf_move_left_joystick(context, {0, 128}, 40ms, 460ms);  //  pivot left
    int left_steps = steps_to_the_left_for_rival(options.starter_choice);
    for (int i = 0; i < left_steps; i++){
        pbf_move_left_joystick(context, {0, 128}, 100ms, 400ms);
    }

    //  Walking toward the rival's fixed position triggers his dialogue and
    //  then the battle automatically; mash B through both.
    BattleMenuWatcher battle_ready(COLOR_RED);
    {
        context.wait_for_all_requests();
        int ret = run_until<ProControllerContext>(
            env.console, context,
            [](ProControllerContext& context){
                pbf_move_left_joystick(context, {128, 255}, 1000ms, 0ms);
                pbf_mash_button(context, BUTTON_B, 20s);
            },
            { battle_ready }
        );
        if (ret < 0){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "run_B0_02_rival_battle_1(): failed to initiate the rival battle.",
                env.console
            );
        }
    }

    env.log("Battle started. Using first move...");
    pbf_mash_button(context, BUTTON_A, 1000ms);
    context.wait_for_all_requests();
    {
        //  Oak's tutorial dialogue interrupts after the first move; wait for
        //  the battle menu to come back before mashing through to a result.
        int ret = run_until<ProControllerContext>(
            env.console, context,
            [](ProControllerContext& context){
                pbf_mash_button(context, BUTTON_B, 30s);
            },
            { battle_ready }
        );
        if (ret < 0){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "run_B0_02_rival_battle_1(): battle menu not seen after first move.",
                env.console
            );
        }
    }

    BattleOpponentFaintWatcher player_won(COLOR_RED);
    BattleFaintWatcher player_lost(COLOR_RED);
    context.wait_for_all_requests();
    int ret = run_until<ProControllerContext>(
        env.console, context,
        [](ProControllerContext& context){
            pbf_mash_button(context, BUTTON_A, 300s);
        },
        { player_won, player_lost }
    );
    if (ret == 1){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "run_B0_02_rival_battle_1(): lost the first rival battle. AutoStory cannot "
            "continue -- the player's starter should always win this battle.",
            env.console
        );
    }
    if (ret != 0){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "run_B0_02_rival_battle_1(): no battle result detected within 5 minutes.",
            env.console
        );
    }
    env.log("Won rival battle 1.");
    track_battle_won(stats);

    //  ---- Mash through the post-battle dialogue back into the lab ----
    pbf_mash_button(context, BUTTON_B, 20s);  //  exit battle
    pbf_mash_button(context, BUTTON_A, 10s);  //  dialogue outside the lab

    //  NOTE: best-effort -- walk back into the lab for the Pokedex/Balls.
    //  Not hardware-verified; retune after the first real run.
    pbf_move_left_joystick(context, {128, 255}, 1000ms, 500ms);
    pbf_move_left_joystick(context, {128, 255}, 800ms, 500ms);
    {
        BlackScreenWatcher lab_fade;
        context.wait_for_all_requests();
        int ret_fade = wait_until(env.console, context, 10s, {lab_fade});
        if (ret_fade < 0){
            env.log("run_B0_02_rival_battle_1(): re-entry lab fade not detected within 10s. Continuing blind.", COLOR_ORANGE);
        }
    }

    //  Oak hands over the Pokedex and 5 Poke Balls; no choices needed.
    pbf_mash_button(context, BUTTON_A, 12s);

    //  ---- Exit the lab for good and walk to Route 1 ----
    //  Reuses the hardware-tuned StarterRng::walk_to_route1_from_lab() path.
    env.log("Exiting the lab...");
    {
        BlackScreenWatcher black_screen(COLOR_RED);
        context.wait_for_all_requests();
        int ret_exit = run_until<ProControllerContext>(
            env.console, context,
            [](ProControllerContext& context){
                pbf_move_left_joystick(context, {128, 0}, 10s, 0ms);
            },
            { black_screen }
        );
        if (ret_exit < 0){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "run_B0_02_rival_battle_1(): failed to exit the lab a second time.",
                env.console
            );
        }
    }

    env.log("Lab exited. Walking to Route 1...");
    pbf_wait(context, 5000ms);
    pbf_move_left_joystick(context, {0, 128}, 1280ms, 300ms);
    pbf_move_left_joystick(context, {128, 255}, 3150ms, 300ms);
    pbf_move_left_joystick(context, {255, 128}, 330ms, 300ms);
    pbf_move_left_joystick(context, {128, 255}, 720ms, 300ms);
    context.wait_for_all_requests();
}

}
}
}
