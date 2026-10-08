/*  Pokemon FRLG AutoStory - Segment B2c
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Route 4 exit of Mt. Moon -> Cerulean City -> Cerulean Gym -> Cascade
 *  Badge.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B2c_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B2c_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: At the Route 4 exit of Mt. Moon, heading for Cerulean City
//         (B2b's end state).
//  End:   Cascade Badge won. Outside the Cerulean Gym.
//
//  Heals at the Route 4 Pokemon Center, walks to Cerulean City and into
//  the Gym, fights through the Gym's trainer(s) and Misty, then mashes
//  through the post-battle Cascade Badge / TM11 (BubbleBeam) hand-off
//  dialog and exits back to the Cerulean City overworld.
//
//  NOTE on scope: the parent ticket's description also lists "Nugget
//  Bridge, Bill, Cut HM01" under this phase. Per the already-committed
//  AutoStory framework (M2, FRO-178) and real game canon, neither is
//  required to win the Cascade Badge:
//    - HM01 Cut is given by the S.S. Anne captain in Vermilion City, which
//      is phase B3b ("Off the S.S. Anne with Cut taught") -- not B2.
//    - Nugget Bridge / Bill's errand (Route 24/25) are optional side
//      content, not gating the Gym challenge.
//  Both are left out of this pass to keep this phase's path to the
//  (hardware-unverified) acceptance criteria -- Cascade Badge, TM11,
//  build passing -- as small and reviewable as possible. Flagged to the
//  ticket author; can be added as a follow-up segment if wanted.
void run_B2c_cerulean_gym(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
