/*  Pokemon FRLG AutoStory - Segment B6a
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
#include "PokemonFRLG_AutoStory_Segment_B6a.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;

namespace{

//  Mash through a single-opponent Rocket grunt trainer battle (most of the
//  grunts guarding Silph Co.'s floors field one Pokemon). Not a general
//  gym-battle loop -- see win_giovanni_battle() below for Giovanni's
//  multi-Pokemon fight at the top floor.
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

//  Multi-Pokemon trainer battle loop for Giovanni (fields several Pokemon
//  at the top of Silph Co., same shape as Koga's gym fight in Segment_B5c
//  -- see win_gym_battle() there for the canonical version of this pattern
//  and its caveats). Duplicated here (rather than shared) to keep each
//  segment file self-contained, matching this codebase's existing
//  per-segment anonymous-namespace convention.
void win_giovanni_battle(
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
                    "win_giovanni_battle(): the whole party fainted against Giovanni.",
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
                "win_giovanni_battle(): lead Pokemon ran out of PP on every priority move.",
                env.console
            );
        }

        //  opponentfainted: either Giovanni sends out his next Pokemon
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
            //  No further battle menu -- Giovanni is out of Pokemon.
            env.log("Giovanni defeated.");
            return;
        }
        //  Otherwise loop around and fight the next Pokemon.
    }
    OperationFailedException::fire(
        ErrorReport::SEND_ERROR_REPORT,
        "win_giovanni_battle(): exceeded the round cap without a battle result. Possible soft-lock -- stopping instead of looping forever.",
        env.console
    );
}

}  //  anonymous namespace


void run_B6a_silph_co(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    env.log("B6a: Saffron City -> Silph Co. (Card Key, Giovanni, Lapras)", COLOR_BLUE);

    //  ---- Fuchsia City to Saffron City ----
    //  Long cross-map hop; the overworld A* navigator handles the actual
    //  route, so no intermediate waypoints are hand-specified here. Saffron
    //  is now reachable: Team Rocket's occupation of Silph Co. is what
    //  gates the city's streets in FRLG, and nothing else blocks entry.
    env.log("Heading to Saffron City...");
    navigate_to(env, context, stats, KantoGoals::SilphCoEntrance, &KantoGoals::FuchsiaGymEntrance, 400);

    //  ---- Silph Co.: the card-key floor puzzle ----
    //  NOT SOLVED HERE. Silph Co. is an 11-floor tower where most floors
    //  have at least one locked door that only opens after the Card Key
    //  (dropped by a Rocket grunt partway up) is in the bag, plus warp
    //  tiles, roamer Rocket grunts, and a Team Rocket boss Pokemon (Rocket
    //  Girl/Scientist fights are optional on most floors) blocking some
    //  routes. None of this floor-by-floor routing has been captured
    //  against real hardware for this project yet -- see
    //  PokemonFRLG_KantoMapNavigator.h's interior-dungeon caveat. The
    //  placeholder below enters the building and then repeats a simple
    //  "walk forward, mash through any dialog/battle, try the elevator"
    //  loop per floor; on real hardware this will very likely stall
    //  against a locked door or empty elevator panel rather than
    //  soft-lock (bumping a wall/door is a no-op), so this is flagged as a
    //  correctness gap rather than a crash risk. Replace with a captured
    //  step sequence (or a card-key-aware solver) before relying on this
    //  in a real run. Flagging as the segment's highest-risk section.
    enter_building(env, context, BuildingType::GYM);

    env.log("Silph Co.: best-effort blind floor climb (card key routing not captured)...", COLOR_ORANGE);
    const int NUM_FLOORS_TO_GIOVANNI = 10;  //  estimated; not hardware-verified
    for (int floor = 0; floor < NUM_FLOORS_TO_GIOVANNI; floor++){
        //  Walk toward wherever this floor's elevator/stairs/Card-Key door
        //  is estimated to be, mashing through any grunt dialog or
        //  encounter along the way.
        pbf_move_left_joystick(context, {128, 0}, 1500ms, 300ms);
        pbf_mash_button(context, BUTTON_A, 3s);
        context.wait_for_all_requests();

        //  A roamer Rocket grunt may intercept and force a battle; if the
        //  battle menu comes up, fight it off the same way B5a's Hideout
        //  grunt is handled.
        BattleMenuWatcher battle_ready(COLOR_RED);
        context.wait_for_all_requests();
        int ret = wait_until(env.console, context, 3s, {battle_ready});
        if (ret == 0){
            env.log("Floor " + std::to_string(floor + 1) + ": engaging a Rocket grunt...");
            win_one_pokemon_trainer_battle(env, context, stats);
        }

        //  Take the elevator/stairs up. Best-effort / not hardware
        //  -verified: the exact elevator panel tile and floor-select menu
        //  entries are not captured, so this just walks forward into
        //  wherever the next floor's transition is estimated to be and
        //  mashes through any menu that appears.
        BlackScreenWatcher floor_fade;
        context.wait_for_all_requests();
        int fade_ret = run_until<ProControllerContext>(
            env.console, context,
            [](ProControllerContext& context){
                pbf_move_left_joystick(context, {128, 0}, 1000ms, 300ms);
                pbf_mash_button(context, BUTTON_A, 3s);
            },
            { floor_fade }
        );
        if (fade_ret < 0){
            env.log("run_B6a_silph_co(): no floor-transition fade detected on floor " + std::to_string(floor + 1) + ". Continuing blind.", COLOR_ORANGE);
        }
    }

    //  ---- Giovanni, top floor ----
    env.log("Confronting Giovanni at the top of Silph Co....");
    pbf_mash_button(context, BUTTON_A, 10s);  //  pre-battle dialogue
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
                "run_B6a_silph_co(): the Giovanni battle never started.",
                env.console
            );
        }
    }
    win_giovanni_battle(env, context, stats);
    pbf_mash_button(context, BUTTON_B, 15s);  //  Giovanni-flees dialogue

    //  ---- The grateful president's Lapras gift ----
    //  The Silph Co. president (rescued along with the building) gives a
    //  free Lapras as thanks. Simple "receive Pokemon" dialog, no menu
    //  choices, same shape as the Soul Badge handover in B5c.
    env.log("Receiving the Lapras gift from the Silph Co. president...");
    pbf_mash_button(context, BUTTON_A, 15s);
    context.wait_for_all_requests();
    env.log("Lapras obtained.");

    //  ---- Leave Silph Co. ----
    //  Best-effort retrace: mash B (cancel any menu) then walk back toward
    //  the ground floor and out the front door. Like the climb above, this
    //  is not hardware-verified.
    pbf_mash_button(context, BUTTON_B, 3s);
    exit_building(env, context, BuildingType::GYM);

    //  Segment ends here, standing outside Silph Co. in Saffron City,
    //  matching this phase's registered end_text ("Lapras obtained,
    //  standing outside Silph Co. in Saffron City."). The Saffron Gym /
    //  Marsh Badge is Phase::B6b's job (see PokemonFRLG_AutoStory.cpp's
    //  phase table and KantoGoals::SaffronGymEntrance in
    //  PokemonFRLG_KantoMapNavigator.h), not this one.
}

}
}
}
