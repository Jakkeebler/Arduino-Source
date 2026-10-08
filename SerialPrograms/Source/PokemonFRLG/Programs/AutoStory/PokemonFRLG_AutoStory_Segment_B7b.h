/*  Pokemon FRLG AutoStory - Segment B7b
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Cinnabar Gym (Secret Key in hand) -> Blaine battle -> Volcano Badge +
 *  TM38 (Fire Blast).
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B7b_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B7b_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: standing outside the Cinnabar Gym, ready to challenge it (Secret
//         Key already obtained in Segment_B7a).
//  End:   Volcano Badge won, standing outside the Cinnabar Gym.
//
//  NOTE: best-effort / not hardware-verified, matching Segment_B5c's Koga
//  loop. The gym's sliding/warp-tile floor puzzle (unlocked by the Secret
//  Key) has no captured route yet, so reaching Blaine is hand-authored
//  choreography, not a solved maze -- flagged as this segment's highest-risk
//  section, same caveat as the Rocket Hideout trapdoors in Segment_B5a.
void run_B7b_blaine_volcano_badge(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
