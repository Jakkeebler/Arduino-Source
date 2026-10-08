/*  Pokemon FRLG AutoStory - Segment B0_01
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Walk from the player's bedroom to Professor Oak's lab and pick the
 *  configured starter Pokemon.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B0_01_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B0_01_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: in control, standing in the player's bedroom (B0_00's end state).
//  End:   starter received (no nickname given), standing in Oak's lab,
//         dialog finished, about to walk out toward the rival battle.
//
//  NOTE: the bedroom->lab walk and the pokeball-table cursor movement below
//  are a best-effort first pass (no existing AutoStory/RNG program in this
//  codebase does a from-scratch bedroom-to-lab walk -- StarterRng's reset
//  loop resumes an existing save already positioned at the table). Table
//  layout is assumed left-to-right Bulbasaur / Squirtle / Charmander,
//  matching vanilla FRLG. Expect to retune step counts/delays after the
//  first real run.
void run_B0_01_starter_selection(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
