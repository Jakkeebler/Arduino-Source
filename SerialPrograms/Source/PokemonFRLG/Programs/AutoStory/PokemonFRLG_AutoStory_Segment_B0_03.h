/*  Pokemon FRLG AutoStory - Segment B0_03
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Walk from Route 1 to Viridian City and heal at the Pokemon Center.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B0_03_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B0_03_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: standing on Route 1 just outside Pallet Town (B0_02's end state).
//  End:   just healed at the Viridian Pokemon Center, standing outside it.
//
//  Uses the map-based KantoMapNavigator (via AutoStoryTools::ensure_healed)
//  since Pallet/Route 1/Viridian are one contiguous walkable region on the
//  combined map -- this is the kind of travel the map navigator is built
//  for, unlike the scripted/blind-sequence indoor segments (B0_00-B0_02).
void run_B0_03_viridian_pc_heal(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
