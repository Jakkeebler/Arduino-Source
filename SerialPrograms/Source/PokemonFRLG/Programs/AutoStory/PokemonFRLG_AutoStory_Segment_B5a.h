/*  Pokemon FRLG AutoStory - Segment B5a
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Celadon Gym (Rainbow Badge) -> Celadon Game Corner / Rocket Hideout
 *  (Silph Scope retrieved) -> Pokemon Tower entrance in Lavender Town.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B5a_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B5a_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: standing outside the Celadon Gym, Rainbow Badge in hand.
//  End:   standing at the entrance to Pokemon Tower in Lavender Town, Silph
//         Scope in the bag.
//
//  NOTE: this is a best-effort first pass. The Rocket Hideout under the
//  Celadon Game Corner is a multi-floor dungeon (trapdoors + a short grunt
//  battle guarding the Silph Scope) that is not part of the overworld A*
//  navigator's supported interior set (see PokemonFRLG_KantoMapNavigator.h),
//  so its traversal below is hand-authored button choreography, not
//  hardware-captured. The Game Corner door tile (KantoGoals::
//  CeladonGameCornerEntrance) and the Pokemon Tower door tile (KantoGoals::
//  PokemonTowerEntrance) are themselves estimated positions -- see the
//  comment above those constants. Expect to retune/re-capture this whole
//  segment after the first real run.
void run_B5a_celadon_hideout_to_tower(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
