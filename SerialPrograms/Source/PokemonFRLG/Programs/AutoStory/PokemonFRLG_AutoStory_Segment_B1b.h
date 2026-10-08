/*  Pokemon FRLG AutoStory - Segment B1b
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Enter the Pewter Gym, battle and defeat Brock, and walk back out with
 *  the Boulder Badge.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B1b_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B1b_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: standing in front of the Pewter Gym door (B1a's end state).
//  End:   Boulder Badge won, standing back outside the Pewter Gym door.
//
//  Pewter Gym in FRLG has no puzzle/obstacles (unlike several later gyms) --
//  just an open floor leading up to Brock, who is Kanto's first Gym Leader
//  (Geodude x2, Onix). Walking into his line of sight triggers the battle
//  automatically, same as any other trainer.
//
//  NOTE: this segment has not been hardware-verified. Specifically
//  unverified:
//    - the exact number of steps from the door to Brock's sightline
//      (walked blind, bounded -- see run_segment body)
//    - the post-battle dialogue chain length (badge get + TM39 Rock Tomb
//      item get), mashed through blind with generous fixed timeouts
//  The battle loop itself reuses spam_first_move(), the same
//  hardware-tested move-selection logic used elsewhere in AutoStory, looped
//  to handle Brock's two Pokemon.
void run_B1b_pewter_gym_brock_battle(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
