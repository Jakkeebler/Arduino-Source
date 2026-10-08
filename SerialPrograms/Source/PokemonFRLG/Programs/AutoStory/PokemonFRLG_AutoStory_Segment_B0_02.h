/*  Pokemon FRLG AutoStory - Segment B0_02
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Exit Oak's lab, battle the rival for the first time, and return to the
 *  lab to receive the Pokedex + Poke Balls.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B0_02_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B0_02_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: in Oak's lab, starter just received (no nickname), B0_01's end
//         state.
//  End:   standing on Route 1, just left Pallet Town with the Pokedex and
//         5 Poke Balls.
//
//  NOTE: the rival-battle positioning (dodge + step count by starter) and
//  the lab exit/battle-menu detection are adapted from the hardware-tuned
//  StarterRng::walk_to_rival_battle() / auto_battle_rival() (this codebase's
//  only prior automation of this exact in-game moment), stripped of the
//  RNG-specific stat reading since AutoStory just needs to win the battle.
//  The "walk back into the lab for the Pokedex" leg has NOT been automated
//  anywhere before and is a best-effort first pass needing hardware tuning.
void run_B0_02_rival_battle_1(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
