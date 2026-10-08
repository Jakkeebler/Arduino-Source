/*  Pokemon FRLG AutoStory - Segment B3c
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Challenge the Vermilion Gym (Lt. Surge, Thunder Badge + TM24
 *  Thunderbolt), then detour to Bill's house on Route 25 for HM02 Fly
 *  before returning to stand outside the Vermilion Gym.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B3c_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B3c_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: in Vermilion City with Cut taught (B3b's end state), heading for
//         the Vermilion Gym.
//  End:   Thunder Badge won, TM24 (Thunderbolt) and HM02 (Fly) obtained.
//         Standing outside the Vermilion Gym door again (matching B4a's
//         documented start state).
//
//  NOTE: best-effort / not hardware-verified. Two pieces of this segment
//  are scripted rather than map-driven, same caveat as other first-pass
//  segments in this codebase:
//    1. The Vermilion Gym's trash-can switch puzzle (3 of ~9 cans are
//       hidden switches gating the path to Surge) is interior and not part
//       of the combined map asset. FRLG uses the same fixed (non-random)
//       switch layout as the original games, but the exact can order coded
//       here has not been confirmed against real hardware -- retune after
//       the first real run.
//    2. Bill's Sea Cottage interior (the "rescue Bill" cutscene that hands
//       over HM02 Fly) is likewise interior/unmapped, so that leg is also
//       scripted dialogue-mashing.
void run_B3c_vermilion_gym_thunder_badge(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
