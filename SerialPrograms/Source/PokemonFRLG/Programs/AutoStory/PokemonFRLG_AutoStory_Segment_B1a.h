/*  Pokemon FRLG AutoStory - Segment B1a
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Navigate from Route 2 (just north of Viridian Forest) through Pewter
 *  City to the Pewter Gym door.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B1a_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B1a_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: on Route 2, just north of Viridian Forest's north exit (B0_05's
//         end state).
//  End:   standing in front of the Pewter Gym door (KantoGoals::PewterGymEntrance),
//         about to walk in.
//
//  This is a single map-covered walk: Route 2 (north half) and Pewter City
//  are both part of the combined Kanto map asset, so kanto_navigate_to()'s
//  A* pathing (via navigate_to()) handles the whole trip, including any
//  wild encounters triggered along the way (it auto-flees/retries) and the
//  Viridian Forest north-exit area.
//
//  NOTE: the optional Route 22 detour mentioned in the FRO-162 plan (catching
//  Pokemon / an early trainer battle) is intentionally NOT implemented here.
//  It is not required to reach Pewter City or win the Boulder Badge, and the
//  core Route 2 -> Pewter path has not been hardware-verified yet either --
//  adding an optional branch on top of an unverified path would only add
//  risk for no story-critical benefit. Revisit if/when a future milestone
//  wants Route 22 grinding as part of AutoStory.
void run_B1a_route2_pewter_gym_entrance(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
