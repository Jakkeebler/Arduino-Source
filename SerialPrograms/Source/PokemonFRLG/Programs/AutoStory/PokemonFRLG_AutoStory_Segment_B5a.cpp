/*  Pokemon FRLG AutoStory - Segment B5a
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
#include "PokemonFRLG/Inference/PokemonFRLG_BattlePokemonDetector.h"
#include "PokemonFRLG/Programs/PokemonFRLG_KantoMapNavigator.h"
#include "PokemonFRLG_AutoStory_Segment_B5a.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;

namespace{

//  Mash through a single-opponent trainer battle (the Rocket grunt guarding
//  the Silph Scope has one Pokemon). Not a general gym-battle loop -- see
//  Segment_B5c for the multi-Pokemon Koga loop.
void win_one_pokemon_trainer_battle(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    AutoStoryStats& stats
){
    BattleMenuWatcher battle_ready(COLOR_RED);
    context.wait_for_all_requests();
    int ret = run_until<ProControllerContext>(
        env.console, context,
        [](ProControllerContext& context){
            pbf_mash_button(context, BUTTON_A, 20s);
        },
        { battle_ready }
    );
    if (ret < 0){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "win_one_pokemon_trainer_battle(): battle menu never appeared.",
            env.console
        );
    }

    BattleOpponentFaintWatcher opponent_fainted(COLOR_RED);
    BattleFaintWatcher player_fainted(COLOR_RED);
    context.wait_for_all_requests();
    int outcome = run_until<ProControllerContext>(
        env.console, context,
        [](ProControllerContext& context){
            pbf_mash_button(context, BUTTON_A, 120s);
        },
        { opponent_fainted, player_fainted }
    );
    if (outcome == 1){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "win_one_pokemon_trainer_battle(): the player's Pokemon fainted. "
            "AutoStory does not expect to lose this battle.",
            env.console
        );
    }
    if (outcome != 0){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "win_one_pokemon_trainer_battle(): no battle result detected within 2 minutes.",
            env.console
        );
    }
    track_battle_won(stats);

    //  Mash through the trainer-defeat dialogue / prize money text back to
    //  the overworld.
    pbf_mash_button(context, BUTTON_B, 15s);
    context.wait_for_all_requests();
}

}  //  anonymous namespace


void run_B5a_celadon_hideout_to_tower(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    env.log("B5a: Celadon Rocket Hideout (Silph Scope) -> Pokemon Tower entrance", COLOR_BLUE);

    //  ---- Walk into the Game Corner and down the hidden stairs ----
    navigate_to(env, context, stats, KantoGoals::CeladonGameCornerEntrance, &KantoGoals::CeladonPokeCenterEntrance);
    //  enter_building()/BuildingType only has a POKECENTER case wired up
    //  today (see PokemonFRLG_AutoStoryTools.h), but enter_pokecenter() /
    //  leave_pokecenter() are themselves just generic "walk through the
    //  door, wait for the fade" helpers with no PokeCenter-specific logic,
    //  so reusing them here for the Game Corner's door is intentional, not
    //  a copy-paste error.
    enter_building(env, context, BuildingType::POKECENTER);

    //  NOTE: best-effort / not hardware-verified. The vending-machine door
    //  to the Hideout is in the back-left corner of the Game Corner; this
    //  walks toward it and mashes through the "strange noise" flavor text.
    //  The three trapdoor floors below are a fixed, memorized maze in the
    //  original game, but the exact tile path has not been captured here --
    //  this just walks generally south/west each floor and mashes dialog,
    //  which is unlikely to reliably reach the Scope room without a real
    //  captured route. Flagging as the segment's highest-risk section.
    pbf_move_left_joystick(context, {0, 128}, 1000ms, 300ms);
    pbf_move_left_joystick(context, {0, 255}, 2000ms, 300ms);
    pbf_mash_button(context, BUTTON_A, 3s);
    {
        BlackScreenWatcher trapdoor_fade;
        context.wait_for_all_requests();
        int ret = wait_until(env.console, context, 10s, {trapdoor_fade});
        if (ret < 0){
            env.log("run_B5a_celadon_hideout_to_tower(): hideout trapdoor fade not detected. Continuing blind.", COLOR_ORANGE);
        }
    }

    //  Rocket grunt guarding the Silph Scope.
    env.log("Engaging the Rocket grunt guarding the Silph Scope...");
    win_one_pokemon_trainer_battle(env, context, stats);

    //  Pick up the Silph Scope (simple "found item" dialog, no choices).
    pbf_mash_button(context, BUTTON_A, 8s);
    context.wait_for_all_requests();

    //  ---- Leave the Hideout / Game Corner the way we came ----
    //  Best-effort retrace: mash B (cancel any menu) then walk back toward
    //  the entrance stairs/door. Like the walk-in above, this is not
    //  hardware-verified.
    pbf_mash_button(context, BUTTON_B, 3s);
    pbf_move_left_joystick(context, {128, 0}, 2000ms, 300ms);
    {
        BlackScreenWatcher exit_fade;
        context.wait_for_all_requests();
        int ret = wait_until(env.console, context, 10s, {exit_fade});
        if (ret < 0){
            env.log("run_B5a_celadon_hideout_to_tower(): hideout exit fade not detected. Continuing blind.", COLOR_ORANGE);
        }
    }
    exit_building(env, context, BuildingType::POKECENTER);

    //  ---- Travel to Lavender Town and the Pokemon Tower entrance ----
    //  Long cross-map hop; the overworld A* navigator handles the actual
    //  route (Route 7/Underground Path/Route 8), so no intermediate
    //  waypoints are hand-specified here.
    env.log("Heading for the Pokemon Tower entrance in Lavender Town...");
    navigate_to(env, context, stats, KantoGoals::PokemonTowerEntrance, &KantoGoals::CeladonPokeCenterEntrance, 400);

    context.wait_for_all_requests();
}

}
}
}
