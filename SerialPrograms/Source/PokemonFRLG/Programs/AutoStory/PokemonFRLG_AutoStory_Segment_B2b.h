/*  Pokemon FRLG AutoStory - Segment B2b
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Mt. Moon traversal (1F -> B1F -> Route 4 exit), including the fossil
 *  choice.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B2b_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B2b_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: Standing at the entrance to Mt. Moon, on the Route 3 side
//         (B2a's end state).
//  End:   At the far (Route 4) exit of Mt. Moon.
//
//  Mt. Moon's interior is NOT part of the combined-map walkability image
//  (only the two exterior door tiles -- KantoGoals::Extended::MtMoon1fDoor1
//  on the Route 3 side and MtMoonB1fDoor1 on the Route 4 side -- are
//  tracked goals; everything between them is un-mapped cave floor). This
//  segment is therefore a scripted burst-walk traversal (same idiom as
//  B0_05's Viridian Forest exit): walk forward in short bursts, bail into
//  any wild encounter that starts, and bail out cleanly on each floor's
//  exit fade. It also makes a fixed, non-configurable fossil choice
//  (Helix Fossil) in the fossil room.
//
//  This is a first-pass/generic implementation, NOT hardware-verified:
//  Mt. Moon's real 1F and B1F layouts (turns, the Rocket grunt encounter,
//  the exact fossil-table tile) have not been captured on hardware. Expect
//  to retune the walk directions/burst counts once this runs on real
//  hardware.
void run_B2b_mtmoon_traversal(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
