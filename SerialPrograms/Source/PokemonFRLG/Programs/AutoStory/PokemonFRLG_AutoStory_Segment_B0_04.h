/*  Pokemon FRLG AutoStory - Segment B0_04
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Walk from Viridian City to the Viridian Forest entrance on Route 2.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B0_04_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B0_04_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: just outside the Viridian Pokemon Center, healed (B0_03's end
//         state).
//  End:   just stepped into the Viridian Forest building/interior.
//
//  Uses KantoMapNavigator to reach the known Route2SouthGrass goal (the
//  verified tile just south of the forest), then a short scripted walk
//  north through the forest's door sprite. The combined map's generated
//  walkability mask does NOT reliably distinguish the forest interior as
//  its own connected component from unrelated regions of the stitched
//  image (checked during implementation -- a flood fill from the forest
//  bbox connects to far-away, geographically unrelated map sections), so
//  this segment deliberately does NOT add new KantoGoal tile constants for
//  the forest door: that would be guessing precise coordinates against an
//  asset (Kanto-Combined.png) that isn't available on this build machine.
//  The scripted walk below is a best-effort first pass; retune on hardware.
void run_B0_04_route1_viridian_forest(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
