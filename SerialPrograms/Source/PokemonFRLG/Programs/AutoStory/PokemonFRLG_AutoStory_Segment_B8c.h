/*  Pokemon FRLG AutoStory - Segment B8c
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Viridian Gym -> Route 22 -> Route 23 -> Victory Road entrance.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B8c_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B8c_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: outside the Viridian Gym, all 8 badges in hand.
//  End:   standing at the entrance to Victory Road on Route 23.
//
//  Pure overworld travel: Route 22 and Route 23 are both covered by the
//  combined-map A* navigator (KantoGoals::Route22NorthEntranceDoor* and the
//  Route23 region), so this segment is delegated almost entirely to
//  navigate_to() -- no hand-authored choreography needed.
void run_B8c_route_22_23_to_victory_road(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
