/*  Pokemon FRLG AutoStory - Segment B5b
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Pokemon Tower entrance -> climb to the top, use the Silph Scope to
 *  unmask the "ghost" (a Rocket grunt disguised as a Marowak), battle him,
 *  rescue Mr. Fuji, and receive the Poke Flute.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B5b_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B5b_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: standing at the entrance to Pokemon Tower, Silph Scope in the bag.
//  End:   back outside Pokemon Tower, Mr. Fuji rescued, Poke Flute in hand.
//
//  NOTE: best-effort first pass, not hardware-verified. Pokemon Tower's 7
//  floors are straight-up staircase climbs with wandering wild Ghost-type
//  encounters and no branching puzzle (unlike the Rocket Hideout in B5a or
//  Koga's gym in B5c), so this is tractable as "walk to the stairs, mash
//  through any encounter, repeat" -- but the per-floor stair positions
//  below are estimated/typical, not captured from this project's own
//  Kanto-Combined.png interior art (Pokemon Tower floors are not in that
//  asset's supported interior set; see PokemonFRLG_KantoMapNavigator.h).
//  Expect to retune step counts after the first real run.
void run_B5b_pokemon_tower(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
