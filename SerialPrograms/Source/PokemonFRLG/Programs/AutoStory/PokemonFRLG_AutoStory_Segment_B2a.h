/*  Pokemon FRLG AutoStory - Segment B2a
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Route 3 -> Mt. Moon entrance.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B2a_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B2a_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: Outside the Pewter Gym, Boulder Badge in hand (B1b's end state).
//  End:   Standing at the entrance to Mt. Moon, on the Route 3 side.
//
//  Heals at the Pewter Pokemon Center first (no healing is available again
//  until Route 4, on the far side of Mt. Moon), then uses KantoMapNavigator
//  to walk Route 3 to the Mt. Moon 1F door. Route 3's wild encounters are
//  handled by kanto_navigate_to()'s built-in flee/retry logic -- no manual
//  battle handling needed here.
void run_B2a_route3_to_mtmoon(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
