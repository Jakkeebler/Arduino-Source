/*  Pokemon FRLG AutoStory - Segment B7a
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Cinnabar Gym (Marsh Badge in hand) -> Pokemon Mansion (Secret Key) ->
 *  back outside the Cinnabar Gym.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B7a_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B7a_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: standing outside the Cinnabar Gym, Marsh Badge in hand.
//  End:   exited Pokemon Mansion, standing outside the Cinnabar Gym again.
//
//  NOTE: this is a best-effort first pass, matching the risk profile already
//  flagged for the Rocket Hideout in Segment_B5a. Pokemon Mansion is a
//  multi-floor dungeon (wild encounters, a Scientist note puzzle in the
//  original games, and the Secret Key needed to open the gym's sliding-floor
//  puzzle) that is not part of the overworld A* navigator's supported
//  interior set (see PokemonFRLG_KantoMapNavigator.h), so its traversal
//  below is hand-authored button choreography, not hardware-captured. The
//  entrance goal (KantoGoals::PokemonMansionEntrance) is itself an estimated
//  position -- see the comment above that constant. Expect to retune/
//  re-capture this whole segment after the first real run.
//
//  Scope note: this segment assumes the player is already on Cinnabar
//  Island (Surf navigation from Fuchsia/Seafoam and the Seafoam Islands
//  dungeon itself are owned by Phase::B6b and Phase::B8a respectively, per
//  the registered phase table in PokemonFRLG_AutoStory.cpp -- not this
//  segment).
void run_B7a_cinnabar_mansion(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
