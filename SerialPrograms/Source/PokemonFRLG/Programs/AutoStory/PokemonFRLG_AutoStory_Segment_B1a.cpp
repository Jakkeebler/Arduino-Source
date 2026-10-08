/*  Pokemon FRLG AutoStory - Segment B1a
 *
 *  From: https://github.com/PokemonAutomation/
 */

#include "Common/Cpp/Color.h"
#include "PokemonFRLG/Programs/PokemonFRLG_KantoMapNavigator.h"
#include "PokemonFRLG_AutoStory_Segment_B1a.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

void run_B1a_route2_pewter_gym_entrance(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    (void)options;

    env.log("B1a: Route 2 -> Pewter City -> Pewter Gym entrance", COLOR_BLUE);

    //  No start_hint: B0_05 left the player at the north edge of Viridian
    //  Forest without a tracked goal (the forest interior isn't covered by
    //  the combined map asset), so the first fix here has to be a cold
    //  match. max_steps is generous (this is a long multi-screen walk --
    //  Route 2 north half into Pewter City proper).
    navigate_to(env, context, stats, KantoGoals::PewterGymEntrance, nullptr, 400);

    env.log("Arrived at the Pewter Gym door.");
}

}
}
}
