/*  Pokemon FRLG AutoStory - Segment B3a
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Navigate from outside the Cerulean Gym, south through Route 5, the
 *  Underground Path (Route 5 <-> Route 6), and Route 6, into Vermilion
 *  City, ending at the S.S. Anne gangplank.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B3a_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B3a_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: outside the Cerulean Gym door, Cascade Badge in hand (B2c's end
//         state).
//  End:   standing at the S.S. Anne gangplank (Extended::SsanneExteriorDoor1)
//         in Vermilion City, about to board.
//
//  Every leg of this trip -- Route 5, the Underground Path tunnel, Route 6,
//  and Vermilion City proper -- is part of the combined Kanto map asset, so
//  this is entirely map-driven navigate_to() calls (no scripted walking).
//  Each call is seeded with the previous goal as a start_hint to avoid a
//  cold full-map match at every leg.
void run_B3a_cerulean_to_ssanne(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
