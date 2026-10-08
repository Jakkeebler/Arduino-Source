/*  Pokemon FRLG AutoStory - Segment B5c
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Mr. Fuji rescued -> wake the Route 12 Snorlax with the Poke Flute ->
 *  Routes 12-15 to Fuchsia City -> beat Koga (Soul Badge + TM06 Toxic) ->
 *  Safari Zone warden side-quest for HM03 Surf.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B5c_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B5c_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: Mr. Fuji rescued, standing outside Pokemon Tower with the Poke
//         Flute.
//  End:   Soul Badge won, TM06 (Toxic) obtained. Standing outside the
//         Fuchsia Gym.
//
//  NOTE: best-effort first pass, not hardware-verified.
//   - The Route 12 Snorlax wake-up/catch-or-KO is scripted blind (mash
//     through the flute cutscene, fight/flee the Snorlax encounter);
//     exact Snorlax placement relative to the flute-use spot is estimated.
//   - The long Routes 12-15 (or Cycling Road) hop to Fuchsia is delegated
//     to the overworld A* navigator (navigate_to), which is the part of
//     this segment on the firmest footing.
//   - Koga's gym has an invisible-wall maze in front of his battle tile;
//     that maze is NOT solved here (not captured against real hardware) --
//     see the big caveat comment in the .cpp. This is the segment's
//     highest-risk section.
//   - HM03 Surf / the Safari Zone warden side-quest is intentionally OUT
//     OF SCOPE here: this codebase's existing phase table
//     (PokemonFRLG_AutoStory.cpp) already assigns that to Phase::B6a
//     ("Outside the Fuchsia Gym, Soul Badge in hand." -> "...Surf and
//     Strength obtained."), and this segment's own registered end_text
//     ends at "Outside the Fuchsia Gym" with no HM mentioned. A
//     FuchsiaSafariZoneEntrance goal was added to
//     PokemonFRLG_KantoMapNavigator.h for B6a to use later.
void run_B5c_koga_soul_badge(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
