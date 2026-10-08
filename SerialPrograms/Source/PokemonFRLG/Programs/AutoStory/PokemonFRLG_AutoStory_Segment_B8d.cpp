/*  Pokemon FRLG AutoStory - Segment B8d
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
#include "PokemonFRLG_AutoStory_Segment_B8d.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;

namespace{

//  Walk a single Victory Road floor toward the (estimated) stairs/exit
//  direction, fighting off any wild encounter met along the way, and
//  mashing through a Strength HM-use prompt as a best guess at clearing a
//  boulder in the path. NOT a real puzzle solver -- see the header comment.
void cross_one_victory_road_floor(
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

        //  No fade: either a wild encounter (Geodude/Onix/Graveler/Zubat
        //  are common on these floors) or a boulder/dark-room obstruction
        //  that this segment cannot distinguish or solve. Handle it as a
        //  wild encounter first (the common case); if nothing is actually
        //  fought, handle_encounter()/spam_first_move() are no-ops-ish and
        //  this just retries the walk, same as cross_one_seafoam_floor().
        env.log("cross_one_victory_road_floor(): no fade seen, checking for a wild encounter.", COLOR_ORANGE);
        bool encounter_shiny = handle_encounter(env.console, context, false);
        (void)encounter_shiny;
        BattleResult result = spam_first_move(env.console, context);
        if (result == BattleResult::playerfainted){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "cross_one_victory_road_floor(): the player's Pokemon fainted to a wild encounter.",
                env.console
            );
        }
        track_battle_won(stats);
        exit_wild_battle(env.console, context, false, true);
    }
    if (!reached_next){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "cross_one_victory_road_floor(): failed to reach the next floor/exit after 3 attempts. "
            "Likely a Strength boulder puzzle this segment cannot solve -- see the header comment.",
            env.console
        );
    }
}

}  //  anonymous namespace


void run_B8d_victory_road(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    env.log("B8d: Victory Road traversal -> Pokemon League gate", COLOR_BLUE);

    //  ---- Enter Victory Road ----
    //  No CAVE case exists in BuildingType (see PokemonFRLG_AutoStoryTools.h)
    //  -- enter_building()/exit_building() with POKECENTER is reused here
    //  intentionally, matching B5a/B8a's precedent.
    enter_building(env, context, BuildingType::POKECENTER);

    //  ---- Cross the known floors (1F, 2F) ----
    //  Only KantoGoals::VictoryRoad1fDoor/2fDoor are cataloged today (see
    //  PokemonFRLG_KantoGoals_Extended.h), so 2 floor-crossings is used as
    //  a floor-count guess. NOT a real boulder-puzzle solve -- see the
    //  header comment. Expect this to get stuck on a real cartridge run.
    for (int floor = 0; floor < 2; floor++){
        env.log("Crossing Victory Road floor " + std::to_string(floor + 1) + "...");
        cross_one_victory_road_floor(env, context, stats, 2000ms);
    }

    //  ---- Exit toward the Pokemon League gate ----
    {
        BlackScreenWatcher exit_fade;
        context.wait_for_all_requests();
        int ret = wait_until(env.console, context, 10s, {exit_fade});
        if (ret < 0){
            env.log("run_B8d_victory_road(): exit fade not detected. Continuing blind.", COLOR_ORANGE);
        }
    }
    exit_building(env, context, BuildingType::POKECENTER);

    //  No KantoGoal exists yet for the Pokemon League gate door itself
    //  (see PokemonFRLG_KantoGoals_Extended.h -- only the Indigo Plateau
    //  Pokemon Center door and its Fly destination are cataloged, no
    //  League building entrance). KantoGoals::FlyIndigoPlateau's overworld
    //  tile is used here as the closest known stand-in; a follow-up ticket
    //  should add a real PokemonLeagueGateDoor constant (template-matched
    //  off Kanto-Combined.png, same process as every other door in that
    //  file) and swap it in here.
    env.log("Heading for the Pokemon League gate (best-effort landing point -- see code comment)...");
    navigate_to(env, context, stats, KantoGoals::Extended::FlyIndigoPlateau, nullptr, 300);

    env.log("At the Pokemon League gate, ready to enter the Elite Four. "
             "NOTE: the Elite Four, Champion rival battle, and Hall of Fame "
             "are NOT implemented by this segment -- there is no Phase enum "
             "value for them yet (Phase::B8d is the last one). See this "
             "file's header comment / the FRO-187 follow-up note.");
    context.wait_for_all_requests();
}

}
}
}
