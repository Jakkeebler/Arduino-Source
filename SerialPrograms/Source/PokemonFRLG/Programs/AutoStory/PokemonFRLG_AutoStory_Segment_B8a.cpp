/*  Pokemon FRLG AutoStory - Segment B8a
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
#include "PokemonFRLG/Inference/Map/PokemonFRLG_KantoGoals_Extended.h"
#include "PokemonFRLG_AutoStory_Segment_B8a.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;

namespace{

//  Walk a single Seafoam Islands floor toward the (estimated) exit
//  direction, fighting off any wild encounter met along the way.
//  Best-effort / not hardware-verified -- see the header comment.
void cross_one_seafoam_floor(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    AutoStoryStats& stats,
    std::chrono::milliseconds walk_duration
){
    bool reached_next = false;
    for (int attempt = 0; attempt < 3 && !reached_next; attempt++){
        BlackScreenWatcher fade;
        context.wait_for_all_requests();
        int ret = run_until<ProControllerContext>(
            env.console, context,
            [&](ProControllerContext& context){
                pbf_move_left_joystick(context, {128, 0}, walk_duration, 200ms);
            },
            { fade }
        );
        if (ret == 0){
            reached_next = true;
            break;
        }

        //  No fade seen: most likely a wild encounter (Golbat/Seel/Slowpoke
        //  on these floors) interrupted the walk.
        env.log("cross_one_seafoam_floor(): no fade seen, checking for a wild encounter.", COLOR_ORANGE);
        bool encounter_shiny = handle_encounter(env.console, context, false);
        (void)encounter_shiny;
        BattleResult result = spam_first_move(env.console, context);
        if (result == BattleResult::playerfainted){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "cross_one_seafoam_floor(): the player's Pokemon fainted to a wild encounter.",
                env.console
            );
        }
        track_battle_won(stats);
        exit_wild_battle(env.console, context, false, true);
    }
    if (!reached_next){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "cross_one_seafoam_floor(): failed to reach the next floor/exit after 3 attempts. "
            "Possible boulder/ice puzzle blocking the hard-coded path (not solved by this segment).",
            env.console
        );
    }
}

}  //  anonymous namespace


void run_B8a_seafoam_islands(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    env.log("B8a: Seafoam Islands traversal (Cinnabar -> Route 20)", COLOR_BLUE);

    //  ---- Travel from the Cinnabar Gym to the Seafoam Islands entrance ----
    env.log("Heading for Seafoam Islands...");
    navigate_to(env, context, stats, KantoGoals::Extended::SeafoamIslands1fDoor1, &KantoGoals::CinnabarPokeCenterEntrance, 300);

    //  ---- Enter the islands ----
    //  No CAVE case exists in BuildingType (see PokemonFRLG_AutoStoryTools.h)
    //  -- enter_building()/exit_building() with POKECENTER is reused here
    //  intentionally, matching B5a's Rocket Hideout precedent: the
    //  underlying helper is just a generic "walk through the door, wait for
    //  the fade" routine with no PokeCenter-specific logic.
    enter_building(env, context, BuildingType::POKECENTER);

    //  ---- Cross the islands' several floors (ice slides, Strength
    //  boulders over water, a Surf crossing to reach Route 20) ----
    //  NOT SOLVED HERE -- see the header comment. Four floor-crossings is a
    //  rough guess at the island's depth; this is expected to get stuck on
    //  a real cartridge run until replaced with a captured route.
    for (int floor = 0; floor < 4; floor++){
        env.log("Crossing Seafoam Islands floor " + std::to_string(floor + 1) + "...");
        cross_one_seafoam_floor(env, context, stats, 2000ms);
    }

    //  ---- Exit onto Route 20 ----
    {
        BlackScreenWatcher exit_fade;
        context.wait_for_all_requests();
        int ret = wait_until(env.console, context, 10s, {exit_fade});
        if (ret < 0){
            env.log("run_B8a_seafoam_islands(): exit fade not detected. Continuing blind.", COLOR_ORANGE);
        }
    }
    exit_building(env, context, BuildingType::POKECENTER);

    env.log("Exited Seafoam Islands onto Route 20, heading for Viridian City.");
    context.wait_for_all_requests();
}

}
}
}
