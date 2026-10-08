/*  Pokemon FRLG AutoStory - Segment B6b
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Outside Silph Co. (Lapras obtained) -> navigate to the Saffron Gym ->
 *  battle through Sabrina's teleport-tile puzzle gym -> defeat Sabrina ->
 *  receive the Marsh Badge and TM46 (Psywave).
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B6b_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B6b_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: standing outside Silph Co. in Saffron City, Lapras in the party.
//  End:   Marsh Badge won, TM46 (Psywave) obtained. Standing outside the
//         Saffron Gym.
//
//  NOTE: best-effort first pass, not hardware-verified.
//   - The short walk from Silph Co. to the Saffron Gym is delegated to the
//     overworld A* navigator (navigate_to), which is the part of this
//     segment on the firmest footing.
//   - Sabrina's gym floor is covered in warp/teleport tiles forming a
//     puzzle that must be solved to reach her battle tile; that puzzle is
//     NOT solved here (not captured against real hardware for this
//     project yet) -- see the big caveat comment in the .cpp. This is
//     documented the same way Koga's invisible-wall gym maze is flagged
//     in Segment_B5c: a blind best-effort placeholder, not a crash risk
//     (stepping on the wrong warp tile just relocates the player
//     elsewhere on the same floor rather than soft-locking), but flagged
//     as a correctness gap. This is the segment's highest-risk section.
//   - The Saffron Gym door tile (KantoGoals::SaffronGymEntrance) is itself
//     an estimated position -- see the comment above that constant.
//   - Sabrina's battle reuses the same multi-Pokemon gym-battle loop
//     pattern as Koga in B5c (win_gym_battle-style looping).
void run_B6b_sabrina_marsh_badge(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
