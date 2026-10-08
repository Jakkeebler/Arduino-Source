/*  Pokemon FRLG AutoStory - Segment B5c
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
#include "PokemonFRLG_AutoStory_Segment_B5c.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;

namespace{

//  Multi-Pokemon trainer battle loop (Koga fields 4 in FRLG). Keeps using
//  the lead's first move via spam_first_move() each time the battle menu
//  comes back; forces in the next party slot on a player faint; stops when
//  the battle menu doesn't return after an opponent faint (trainer fully
//  defeated) or the party runs out of healthy Pokemon.
//
//  Best-effort / not hardware-verified: FRLG gym trainer AI can switch or
//  stall, and this always just re-presses the first move slot, matching
//  the same simplification already used for the B0 rival battle and the
//  B5a/B5b single-Pokemon battles. Good enough to not soft-lock; not
//  guaranteed to be move-type-optimal.
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
                    "win_gym_battle(): the whole party fainted against the gym leader.",
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

        //  opponentfainted: either the gym leader sends out their next
        //  Pokemon (battle menu returns) or the whole battle is over.
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
            //  No further battle menu -- gym leader is out of Pokemon.
            env.log("Gym leader defeated.");
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


void run_B5c_koga_soul_badge(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    env.log("B5c: Routes 12-15 -> Koga -> Soul Badge -> Surf HM", COLOR_BLUE);

    //  ---- Wake the Route 12 Snorlax with the Poke Flute ----
    //  Best-effort / not hardware-verified: navigate_to() gets us to the
    //  general Route 12 area via the overworld A*, but the exact "stand
    //  here and use the flute" tile is not captured, so this just opens
    //  the bag and uses the flute as soon as we're in the vicinity, then
    //  fights the Snorlax that wakes up blocking the path.
    env.log("Approaching the sleeping Snorlax on Route 12...");
    open_bag_from_overworld(env.console, context);
    pbf_mash_button(context, BUTTON_A, 2s);  //  select the Poke Flute (first/only key item of its kind)
    pbf_mash_button(context, BUTTON_A, 3s);  //  confirm "use"
    context.wait_for_all_requests();

    env.log("Snorlax woke up. Battling it...");
    {
        BattleMenuWatcher battle_ready(COLOR_RED);
        context.wait_for_all_requests();
        int ret = run_until<ProControllerContext>(
            env.console, context,
            [](ProControllerContext& context){
                pbf_move_left_joystick(context, {128, 0}, 1500ms, 300ms);
                pbf_mash_button(context, BUTTON_A, 10s);
            },
            { battle_ready }
        );
        if (ret < 0){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "run_B5c_koga_soul_badge(): the Snorlax encounter never started.",
                env.console
            );
        }
    }
    BattleResult snorlax_result = spam_first_move(env.console, context);
    if (snorlax_result == BattleResult::playerfainted){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "run_B5c_koga_soul_badge(): lost to the Route 12 Snorlax.",
            env.console
        );
    }
    track_battle_won(stats);
    exit_wild_battle(env.console, context, false, true);

    //  ---- Routes 12-15 (or Cycling Road) to Fuchsia City ----
    //  Delegated entirely to the overworld A* navigator; no hand-specified
    //  waypoints needed for the long hop itself.
    env.log("Heading to Fuchsia City...");
    navigate_to(env, context, stats, KantoGoals::FuchsiaGymEntrance, &KantoGoals::LavenderPokeCenterEntrance, 400);

    //  ---- Koga: the invisible-wall gym maze ----
    //  NOT SOLVED HERE. Koga's gym floor is covered in invisible walls
    //  forming a maze that must be walked blind (or via a memorized path);
    //  no such path has been captured against real hardware for this
    //  project yet. The placeholder below just mashes forward -- on real
    //  hardware this will very likely stall against a wall rather than
    //  softlock (bumping a wall is a no-op), so this is flagged as a
    //  correctness gap rather than a crash risk. Replace with a captured
    //  step sequence (or an invisible-wall-aware solver) before relying on
    //  this in a real run.
    //  Koga's gym has an invisible-wall maze (see note below); walk in
    //  first via the generic Gym door-fade helper.
    enter_building(env, context, BuildingType::GYM);
    for (int i = 0; i < 6; i++){
        pbf_move_left_joystick(context, {128, 0}, 1200ms, 200ms);
    }

    env.log("Battling Koga...");
    win_gym_battle(env, context, stats);

    //  ---- Soul Badge + TM06 (Toxic) ----
    pbf_mash_button(context, BUTTON_B, 15s);   //  trainer-defeat dialogue
    pbf_mash_button(context, BUTTON_A, 15s);   //  Soul Badge + TM06 handover
    context.wait_for_all_requests();
    env.log("Soul Badge won. TM06 (Toxic) received.");

    exit_building(env, context, BuildingType::POKECENTER);

    //  Segment ends here, matching this phase's registered end_text
    //  ("Soul Badge won. Outside the Fuchsia Gym."). The Safari Zone /
    //  HM03 Surf side-quest is Phase::B6a's job (see PokemonFRLG_AutoStory
    //  .cpp's phase table and FuchsiaSafariZoneEntrance in
    //  PokemonFRLG_KantoMapNavigator.h, added here for B6a to use), not
    //  this one -- it is intentionally not attempted in this segment.
}

}
}
}
