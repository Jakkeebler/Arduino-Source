/*  Pokemon FRLG AutoStory - Segment B2a
 *
 *  From: https://github.com/PokemonAutomation/
 */

#include "Common/Cpp/Color.h"
#include "PokemonFRLG/PokemonFRLG_Navigation.h"
#include "PokemonFRLG/Programs/PokemonFRLG_KantoMapNavigator.h"
#include "PokemonFRLG/Inference/Map/PokemonFRLG_KantoGoals_Extended.h"
#include "PokemonFRLG_AutoStory_Segment_B2a.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

void run_B2a_route3_to_mtmoon(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    (void)options;
    env.log("B2a: Route 3 -> Mt. Moon entrance", COLOR_BLUE);

    //  Mandatory heal before the Mt. Moon push: no Pokemon Center is
    //  reachable again until Route 4, on the far side of the mountain.
    navigate_to(env, context, stats, KantoGoals::PewterPokeCenterEntrance);
    enter_building(env, context, BuildingType::POKECENTER);
    heal_at_pokecenter(env.console, context);
    exit_building(env, context, BuildingType::POKECENTER);

    //  Walk Route 3 to the Mt. Moon 1F door. kanto_navigate_to() handles
    //  Route 3's wild encounters internally (flee + retry), so no manual
    //  battle loop is needed for this leg.
    navigate_to(
        env, context, stats,
        KantoGoals::Extended::MtMoon1fDoor1,
        &KantoGoals::PewterPokeCenterEntrance
    );

    env.log("Standing at the entrance to Mt. Moon.");
}

}
}
}
