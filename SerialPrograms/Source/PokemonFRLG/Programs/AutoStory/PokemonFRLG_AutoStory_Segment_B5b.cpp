/*  Pokemon FRLG AutoStory - Segment B5b
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
#include "PokemonFRLG_AutoStory_Segment_B5b.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;

namespace{

//  Walk toward the up-stairs on the current floor and climb them, fighting
//  off any wild Ghost-type encounter met along the way. `walk_duration` is
//  an estimated (not hardware-tuned) joystick-hold time toward the stairs
//  for this particular floor.
void climb_one_tower_floor(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    AutoStoryStats& stats,
    std::chrono::milliseconds walk_duration
){
    bool reached_stairs = false;
    for (int attempt = 0; attempt < 3 && !reached_stairs; attempt++){
        BlackScreenWatcher stairs_fade;
        context.wait_for_all_requests();
        int ret = run_until<ProControllerContext>(
            env.console, context,
            [&](ProControllerContext& context){
                pbf_move_left_joystick(context, {128, 0}, walk_duration, 200ms);
            },
            { stairs_fade }
        );
        if (ret == 0){
            reached_stairs = true;
            break;
        }

        //  No fade seen: most likely a wild encounter interrupted the walk.
        //  Fight it off (Ghost-types here are low level; no need to flee)
        //  and try walking to the stairs again.
        env.log("climb_one_tower_floor(): no stair fade seen, checking for a wild encounter.", COLOR_ORANGE);
        bool encounter_shiny = handle_encounter(env.console, context, false);
        (void)encounter_shiny;
        BattleResult result = spam_first_move(env.console, context);
        if (result == BattleResult::playerfainted){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "climb_one_tower_floor(): the player's Pokemon fainted to a wild encounter.",
                env.console
            );
        }
        track_battle_won(stats);
        exit_wild_battle(env.console, context, false, true);
    }
    if (!reached_stairs){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "climb_one_tower_floor(): failed to reach the next floor's stairs after 3 attempts.",
            env.console
        );
    }
}

}  //  anonymous namespace


void run_B5b_pokemon_tower(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    env.log("B5b: Pokemon Tower traversal -> Mr. Fuji rescued", COLOR_BLUE);

    //  Enter the tower.
    {
        BlackScreenWatcher entrance_fade;
        context.wait_for_all_requests();
        int ret = run_until<ProControllerContext>(
            env.console, context,
            [](ProControllerContext& context){
                pbf_move_left_joystick(context, {128, 0}, 1500ms, 300ms);
            },
            { entrance_fade }
        );
        if (ret < 0){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "run_B5b_pokemon_tower(): failed to walk into the tower entrance.",
                env.console
            );
        }
    }

    //  ---- Climb floors 1F through 6F ----
    //  Estimated per-floor walk durations toward the stairs -- NOT
    //  hardware-verified. All 6 are the same placeholder value for now;
    //  retune individually once captured against real hardware.
    for (int floor = 1; floor <= 6; floor++){
        env.log("Climbing to floor " + std::to_string(floor + 1) + "...");
        climb_one_tower_floor(env, context, stats, 2500ms);
    }

    //  ---- 7F: the "ghost" (disguised Rocket grunt) blocking Mr. Fuji ----
    //  Walking up to the ghost triggers dialogue; the Silph Scope reveals
    //  him automatically (no menu choice needed in FRLG) and the battle
    //  starts. He fields a single Pokemon similar in difficulty to the
    //  Hideout grunt in B5a.
    env.log("Confronting the disguised Rocket grunt on 7F...");
    pbf_move_left_joystick(context, {128, 0}, 1500ms, 300ms);
    pbf_mash_button(context, BUTTON_A, 10s);
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
                "run_B5b_pokemon_tower(): the 7F ghost battle never started.",
                env.console
            );
        }
    }
    BattleResult result = spam_first_move(env.console, context);
    if (result == BattleResult::playerfainted){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "run_B5b_pokemon_tower(): lost the 7F Rocket grunt battle.",
            env.console
        );
    }
    track_battle_won(stats);
    exit_wild_battle(env.console, context, false, true);
    pbf_mash_button(context, BUTTON_B, 15s);  //  trainer-defeat dialogue

    //  ---- Walk to Mr. Fuji and escort him out ----
    //  Best-effort / not hardware-verified: walk further into 7F to find
    //  Mr. Fuji, mash through the rescue dialogue, then retrace down all 7
    //  floors (door-fade gated, same mechanism as the entrance) back
    //  outside.
    env.log("Rescuing Mr. Fuji...");
    pbf_move_left_joystick(context, {128, 0}, 1000ms, 300ms);
    pbf_mash_button(context, BUTTON_A, 15s);
    context.wait_for_all_requests();

    env.log("Escorting Mr. Fuji out of the tower...");
    for (int floor = 0; floor < 7; floor++){
        BlackScreenWatcher floor_fade;
        context.wait_for_all_requests();
        int ret = run_until<ProControllerContext>(
            env.console, context,
            [](ProControllerContext& context){
                pbf_move_left_joystick(context, {128, 255}, 2500ms, 200ms);
            },
            { floor_fade }
        );
        if (ret < 0){
            env.log("run_B5b_pokemon_tower(): floor-down fade not detected on the way out. Continuing blind.", COLOR_ORANGE);
        }
    }

    //  Mr. Fuji's house: rescue dialogue concludes, he gives the Poke Flute.
    pbf_mash_button(context, BUTTON_A, 15s);
    context.wait_for_all_requests();

    env.log("Mr. Fuji rescued. Poke Flute received.");
}

}
}
}
