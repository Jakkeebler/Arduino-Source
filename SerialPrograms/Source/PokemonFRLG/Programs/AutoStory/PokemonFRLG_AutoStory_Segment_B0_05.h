/*  Pokemon FRLG AutoStory - Segment B0_05
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Navigate through Viridian Forest to its northern exit onto Route 2.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B0_05_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B0_05_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: just inside the Viridian Forest entrance (B0_04's end state).
//  End:   just exited Viridian Forest onto Route 2 (north side), heading
//         toward Pewter City.
//
//  NOTE: Viridian Forest's real layout has several zigzag corridors around
//  patches of tall grass and a couple of trainers, none of which this
//  codebase has ever automated or captured map data for. This segment does
//  NOT attempt to reproduce that exact path (doing so would mean guessing
//  turn-by-turn coordinates with no way to verify them here). Instead it
//  repeatedly walks north in bounded bursts, auto-battles any wild
//  encounter that triggers along the way, and watches for the black-screen
//  fade that signals leaving the forest. If the step budget is exhausted
//  without seeing that fade, it fails loudly (OperationFailedException)
//  rather than reporting a false success -- this is the segment most
//  likely to need a real path capture on hardware before it works
//  end-to-end.
void run_B0_05_viridian_forest_exit(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
