/*  Pokemon FRLG AutoStory - Segment B8a
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Cinnabar Gym (Volcano Badge) -> Seafoam Islands traversal -> Route 20,
 *  heading for Viridian City.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B8a_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B8a_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: standing outside the Cinnabar Gym, Volcano Badge in hand.
//  End:   exited Seafoam Islands onto Route 20, heading for Viridian City.
//
//  NOTE: best-effort first pass, not hardware-verified. Seafoam Islands is
//  not part of the overworld A* navigator's supported interior set (see
//  the dungeon list in PokemonFRLG_KantoMapNavigator.h's header comment --
//  Seafoam is absent from it, unlike Mt. Moon/Rock Tunnel/Viridian Forest),
//  and its real layout (ice-block-sliding puzzle floors, Strength boulders
//  over water, a Surf crossing) is not captured here. This just walks a
//  generally north/west path per floor and mashes through any wild
//  encounter, matching the same simplification already used for the
//  Rocket Hideout in B5a. Flagging as this segment's highest-risk section;
//  expect to retune/re-capture after the first real run.
void run_B8a_seafoam_islands(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
