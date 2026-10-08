/*  Pokemon FRLG AutoStory - Segment B8c
 *
 *  From: https://github.com/PokemonAutomation/
 */

#include "Common/Cpp/Color.h"
#include "PokemonFRLG/Programs/PokemonFRLG_KantoMapNavigator.h"
#include "PokemonFRLG/Inference/Map/PokemonFRLG_KantoGoals_Extended.h"
#include "PokemonFRLG_AutoStory_Segment_B8c.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

void run_B8c_route_22_23_to_victory_road(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    env.log("B8c: Route 22/23 -> Victory Road entrance", COLOR_BLUE);

    //  Long cross-map hop; the overworld A* navigator handles the actual
    //  route (Route 22 -> Route 23), so no intermediate waypoints are
    //  hand-specified here. navigate_to() stops on the door tile itself
    //  (standing in front of it, not yet entered), matching this phase's
    //  registered end_text ("standing at the entrance to Victory Road").
    env.log("Heading for the Victory Road entrance...");
    navigate_to(env, context, stats, KantoGoals::Extended::VictoryRoad1fDoor, &KantoGoals::Extended::ViridianCityGymDoor, 400);

    context.wait_for_all_requests();
}

}
}
}
