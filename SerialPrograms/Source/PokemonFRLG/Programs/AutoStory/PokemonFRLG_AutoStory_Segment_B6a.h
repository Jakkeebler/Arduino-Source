/*  Pokemon FRLG AutoStory - Segment B6a
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Outside the Fuchsia Gym (Soul Badge won) -> travel to Saffron City (now
 *  reachable; Team Rocket's occupation of Silph Co. is what gates Saffron
 *  in FRLG) -> enter Silph Co., climb its card-key-gated floors, battle
 *  Giovanni at the top, and receive the free Lapras gift from the grateful
 *  Silph Co. president.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B6a_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B6a_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: standing outside the Fuchsia Gym, Soul Badge in hand.
//  End:   Lapras obtained, standing outside Silph Co. in Saffron City.
//
//  NOTE: best-effort first pass, not hardware-verified.
//   - The long Fuchsia -> Saffron City hop is delegated to the overworld
//     A* navigator (navigate_to), which is the part of this segment on the
//     firmest footing.
//   - Silph Co. is a multi-floor dungeon with a card-key puzzle (locked
//     doors scattered across floors, opened after a Rocket grunt drops the
//     Card Key item, revisited floors as new doors unlock) that is NOT
//     part of the overworld A* navigator's supported interior set (see
//     PokemonFRLG_KantoMapNavigator.h), so its traversal below is
//     hand-authored button choreography, not hardware-captured. This is
//     the segment's highest-risk section -- expect to retune/re-capture it
//     after the first real run, same as the Rocket Hideout in B5a and
//     Pokemon Tower in B5b.
//   - The Silph Co. door tile (KantoGoals::SilphCoEntrance) is itself an
//     estimated position -- see the comment above that constant.
//   - Giovanni's battle at the top floor reuses the same multi-Pokemon gym
//     -battle loop pattern as Koga in B5c (win_gym_battle-style looping),
//     since Giovanni fields multiple Pokemon here too.
void run_B6a_silph_co(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
